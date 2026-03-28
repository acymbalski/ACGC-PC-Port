#include "ac_npc_reset.h"

#if VERSION >= VER_DELUXE

#include "m_common_data.h"
#include "m_player_lib.h"
#include "m_msg.h"
#include "m_bgm.h"
#include "ac_reset_demo.h"
#include "ac_resetcenter_indoor.h"
#include "ac_reset_chair.h"

/* ========================================================================== */
/*  Forward declarations                                                      */
/* ========================================================================== */

static void aRST_actor_ct(ACTOR* actorx, GAME* game);
static void aRST_actor_save(ACTOR* actorx, GAME* game);
static void aRST_actor_dt(ACTOR* actorx, GAME* game);
static void aRST_actor_init(ACTOR* actorx, GAME* game);
static void aRST_actor_move(ACTOR* actorx, GAME* game);
static void aRST_actor_draw(ACTOR* actorx, GAME* game);
static BOOL aRST_talk_init(ACTOR* actorx, GAME* game);
static BOOL aRST_talk_end_chk(ACTOR* actorx, GAME* game);
static void aRST_schedule_proc(NPC_ACTOR* nactorx, GAME_PLAY* play, int type);
static void aRST_setup_think_proc(NPC_RESET_ACTOR* actor, GAME_PLAY* play, u8 think_idx);
static void aRST_change_talk_proc(NPC_RESET_ACTOR* actor, u8 talk_idx);
static void aRST_sailing_process(NPC_RESET_ACTOR* actor);
static void aRST_set_position(ACTOR* actorx, GAME* game);
static BOOL aRST_check_look_range(ACTOR* actorx, GAME* game);
static void aRST_head_proc(ACTOR* actorx, GAME* game);
static void aRST_force_talk_request(ACTOR* actorx, GAME* game);
static void aRST_norm_talk_request(ACTOR* actorx, GAME* game);

/* ========================================================================== */
/*  Actor Profile                                                             */
/* ========================================================================== */

// clang-format off
ACTOR_PROFILE Npc_Reset_Profile = {
    mAc_PROFILE_NPC_RESET,
    ACTOR_PART_NPC,
    ACTOR_STATE_NONE,
    SP_NPC_MAJIN,
    ACTOR_OBJ_BANK_KEEP,
    sizeof(NPC_RESET_ACTOR),
    aRST_actor_ct,
    aRST_actor_dt,
    aRST_actor_init,
    mActor_NONE_PROC1,
    aRST_actor_save,
};
// clang-format on

/* ========================================================================== */
/*  actor_ct (388 bytes)                                                      */
/*  Construct Resetti NPC actor. Set up schedule, talk callbacks, and          */
/*  initial sailing/think state.                                              */
/* ========================================================================== */

static void aRST_actor_ct(ACTOR* actorx, GAME* game) {
    static aNPC_ct_data_c ct_data = {
        aRST_actor_move,
        aRST_actor_draw,
        aNPC_CT_SCHED_TYPE_SPECIAL,
        (aNPC_TALK_REQUEST_PROC)none_proc1,
        aRST_talk_init,
        aRST_talk_end_chk,
        0,
    };

    if (NPC_CLIP->birth_check_proc(actorx, game) == TRUE) {
        NPC_RESET_ACTOR* actor = (NPC_RESET_ACTOR*)actorx;

        actor->npc_class.schedule.schedule_proc = aRST_schedule_proc;
        NPC_CLIP->ct_proc(actorx, game, &ct_data);
        actorx->status_data.weight = MASSTYPE_HEAVY;

        /* Initialize sailing data */
        actor->sailing_state = aRST_SAILING_IDLE;
        actor->sailing_timer = 0;
        actor->sailing_t = 0.0f;
        actor->sailing_done = FALSE;

        /* Initialize state */
        actor->think_idx = aRST_THINK_NORMAL_WAIT;
        actor->think_proc = NULL;
        actor->talk_idx = 0;
        actor->talk_proc = NULL;
        actor->talk_end = FALSE;
        actor->timer = 0;
        actor->timer2 = 0;
        actor->look_range = 200.0f;
        actor->force_talk_flag = FALSE;
        actor->norm_talk_flag = FALSE;
        actor->head_request_set = FALSE;

        /* Position relative to spawn */
        actor->sailing_start = actorx->world.position;
        actor->sailing_target.x = actorx->world.position.x;
        actor->sailing_target.y = actorx->world.position.y;
        actor->sailing_target.z = actorx->world.position.z;

        /* Set initial condition flags */
        actor->npc_class.condition_info.hide_request = FALSE;
        actor->npc_class.condition_info.demo_flg = aNPC_COND_DEMO_SKIP_FEEL_CHECK;
    }
}

/* ========================================================================== */
/*  actor_save (32 bytes)                                                     */
/* ========================================================================== */

static void aRST_actor_save(ACTOR* actorx, GAME* game) {
    NPC_CLIP->save_proc(actorx, game);
}

/* ========================================================================== */
/*  actor_dt (56 bytes)                                                       */
/* ========================================================================== */

static void aRST_actor_dt(ACTOR* actorx, GAME* game) {
    NPC_CLIP->dt_proc(actorx, game);

    /* Clear reset demo reference to this actor */
    if (CLIP(demo_clip2) != NULL && CLIP(demo_clip2)->type == mDemo_CLIP_TYPE_RESET_DEMO) {
        ACTOR* demox = (ACTOR*)CLIP(demo_clip2)->demo_class;

        if (demox != NULL) {
            RESET_DEMO_ACTOR* reset_demo = (RESET_DEMO_ACTOR*)demox;

            if (reset_demo->reset_actor == (ACTOR*)actorx) {
                reset_demo->reset_actor = NULL;
            }
        }
    }
}

/* ========================================================================== */
/*  actor_init (56 bytes)                                                     */
/* ========================================================================== */

static void aRST_actor_init(ACTOR* actorx, GAME* game) {
    NPC_CLIP->init_proc(actorx, game);
}

/* ========================================================================== */
/*  set_request_act (148 bytes)                                               */
/*  Set up action request with priority, index, type, and position args.      */
/*  Signature: (NPC_RESET_ACTOR*, u8 priority, u8 act_idx, u8 act_type,      */
/*              u16 arg, s16 move_x, s16 move_z)                              */
/* ========================================================================== */

static void aRST_set_request_act(NPC_RESET_ACTOR* actor, u8 priority, u8 act_idx, u8 act_type, u16 arg, s16 move_x, s16 move_z) {
    actor->npc_class.request.act_priority = priority;
    actor->npc_class.request.act_idx = act_idx;
    actor->npc_class.request.act_type = act_type;
    actor->npc_class.request.act_args[0] = arg;
    actor->npc_class.action.move_x = move_x;
    actor->npc_class.action.move_z = move_z;
}

/* ========================================================================== */
/*  set_position (236 bytes)                                                  */
/*  Update Resetti's world position based on sailing interpolation and        */
/*  background collision.                                                     */
/* ========================================================================== */

static void aRST_set_position(ACTOR* actorx, GAME* game) {
    NPC_RESET_ACTOR* actor = (NPC_RESET_ACTOR*)actorx;

    if (actor->sailing_state == aRST_SAILING_APPROACH || actor->sailing_state == aRST_SAILING_DEPART) {
        f32 t = actor->sailing_t;

        if (actor->sailing_state == aRST_SAILING_APPROACH) {
            actorx->world.position.x = actor->sailing_start.x + (actor->sailing_target.x - actor->sailing_start.x) * t;
            actorx->world.position.y = actor->sailing_start.y + (actor->sailing_target.y - actor->sailing_start.y) * t;
            actorx->world.position.z = actor->sailing_start.z + (actor->sailing_target.z - actor->sailing_start.z) * t;
        } else {
            actorx->world.position.x = actor->sailing_target.x + (actor->sailing_start.x - actor->sailing_target.x) * t;
            actorx->world.position.y = actor->sailing_target.y + (actor->sailing_start.y - actor->sailing_target.y) * t;
            actorx->world.position.z = actor->sailing_target.z + (actor->sailing_start.z - actor->sailing_target.z) * t;
        }
    }
}

/* ========================================================================== */
/*  check_look_range (180 bytes)                                              */
/*  Check if the player is within Resetti's look/interaction range.           */
/* ========================================================================== */

static BOOL aRST_check_look_range(ACTOR* actorx, GAME* game) {
    NPC_RESET_ACTOR* actor = (NPC_RESET_ACTOR*)actorx;
    ACTOR* playerx = GET_PLAYER_ACTOR_GAME_ACTOR(game);

    if (playerx != NULL) {
        f32 dx = playerx->world.position.x - actorx->world.position.x;
        f32 dz = playerx->world.position.z - actorx->world.position.z;
        f32 dist_sq = dx * dx + dz * dz;

        if (dist_sq < actor->look_range * actor->look_range) {
            return TRUE;
        }
    }

    return FALSE;
}

/* ========================================================================== */
/*  head_proc (436 bytes)                                                     */
/*  Process Resetti's head tracking — look at player or set fixed angle.      */
/* ========================================================================== */

static void aRST_head_proc(ACTOR* actorx, GAME* game) {
    NPC_RESET_ACTOR* actor = (NPC_RESET_ACTOR*)actorx;
    GAME_PLAY* play = (GAME_PLAY*)game;
    ACTOR* playerx = GET_PLAYER_ACTOR_GAME_ACTOR(game);

    if (playerx != NULL && aRST_check_look_range(actorx, game)) {
        /* Track player head */
        xyz_t target_pos;

        target_pos.x = playerx->world.position.x;
        target_pos.y = playerx->world.position.y + 40.0f;
        target_pos.z = playerx->world.position.z;

        NPC_CLIP->set_head_request_act_proc(&actor->npc_class, 4, aNPC_HEAD_TARGET_POS, NULL, &target_pos);
        actor->head_request_set = TRUE;
    } else if (actor->head_request_set) {
        /* Reset head to forward */
        NPC_CLIP->set_head_request_act_proc(&actor->npc_class, 4, aNPC_HEAD_TARGET_NONE, NULL, NULL);
        actor->head_request_set = FALSE;
    }

    /* Process demo order head commands if in talk */
    if (mDemo_Check(mDemo_TYPE_SPEAK, actorx)) {
        int demo_order = mDemo_Get_OrderValue(mDemo_ORDER_NPC0, 3);

        switch (demo_order) {
            case 1: {
                /* Look at camera */
                xyz_t cam_pos = *Camera2_getEyePos_p();
                cam_pos.y += -150.0f;
                NPC_CLIP->set_head_request_act_proc(&actor->npc_class, 4, aNPC_HEAD_TARGET_POS, NULL, &cam_pos);
                actor->npc_class.movement.mv_angl = 0;
                actor->npc_class.movement.mv_add_angl = DEG2SHORT_ANGLE2(11.25f);
                break;
            }
            case 0xFF:
                /* Look at player */
                actor->npc_class.movement.mv_angl = actorx->player_angle_y;
                actor->npc_class.movement.mv_add_angl = DEG2SHORT_ANGLE2(11.25f);
                break;
            default:
                break;
        }
        mDemo_Set_OrderValue(mDemo_ORDER_NPC0, 3, 0);
    }
}

/* ========================================================================== */
/*  actor_move (112 bytes)                                                    */
/*  Main move callback — process sailing, position, head, then NPC move.      */
/* ========================================================================== */

static void aRST_actor_move(ACTOR* actorx, GAME* game) {
    NPC_RESET_ACTOR* actor = (NPC_RESET_ACTOR*)actorx;

    aRST_sailing_process(actor);
    aRST_set_position(actorx, game);
    aRST_head_proc(actorx, game);
    NPC_CLIP->move_proc(actorx, game);

    actorx->shape_info.draw_shadow = FALSE;
}

/* ========================================================================== */
/*  actor_draw (56 bytes)                                                     */
/* ========================================================================== */

static void aRST_actor_draw(ACTOR* actorx, GAME* game) {
    NPC_CLIP->draw_proc(actorx, game);
}

/* ========================================================================== */
/*  sailing_process (208 bytes)                                               */
/*  Update sailing interpolation each frame. Only takes actor (no play).      */
/* ========================================================================== */

static void aRST_sailing_process(NPC_RESET_ACTOR* actor) {
    if (actor->sailing_state == aRST_SAILING_APPROACH) {
        actor->sailing_timer++;

        actor->sailing_t = (f32)actor->sailing_timer / 120.0f;
        if (actor->sailing_t > 1.0f) {
            actor->sailing_t = 1.0f;
        }

        if (actor->sailing_t >= 1.0f) {
            actor->sailing_state = aRST_SAILING_ARRIVE;
            actor->sailing_done = TRUE;
        }
    } else if (actor->sailing_state == aRST_SAILING_DEPART) {
        actor->sailing_timer++;

        actor->sailing_t = (f32)actor->sailing_timer / 120.0f;
        if (actor->sailing_t > 1.0f) {
            actor->sailing_t = 1.0f;
        }

        if (actor->sailing_t >= 1.0f) {
            actor->sailing_state = aRST_SAILING_IDLE;
        }
    }
}

/* ========================================================================== */
/*  Talk state functions — called from talk_proc dispatch                     */
/* ========================================================================== */

/* talk_byebye (80 bytes) — Resetti says farewell */
static void aRST_talk_byebye(NPC_RESET_ACTOR* actor, GAME_PLAY* play) {
    int order = mDemo_Get_OrderValue(mDemo_ORDER_NPC0, 9);

    if (order != 0 && mMsg_CHECK_MAINNORMALCONTINUE() == TRUE) {
        mDemo_Set_OrderValue(mDemo_ORDER_NPC0, 9, 0);
        actor->talk_end = TRUE;
    }
}

/* talk_exit (116 bytes) — Resetti exits after talk completes */
static void aRST_talk_exit(NPC_RESET_ACTOR* actor, GAME_PLAY* play) {
    int order = mDemo_Get_OrderValue(mDemo_ORDER_NPC0, 9);

    if (order != 0 && mMsg_CHECK_MAINNORMALCONTINUE() == TRUE) {
        mDemo_Set_OrderValue(mDemo_ORDER_NPC0, 9, 0);
        aRST_setup_think_proc(actor, play, aRST_THINK_HIDE);
        actor->talk_end = TRUE;
    }
}

/* talk_stop_player (60 bytes) — Stop player movement during talk */
static void aRST_talk_stop_player(NPC_RESET_ACTOR* actor, GAME_PLAY* play) {
    mPlib_request_main_demo_wait_type1((GAME*)play, FALSE, NULL);
    aRST_change_talk_proc(actor, aRST_TALK_AINOTE_0);
}

/* talk_ainote_0 (128 bytes) — First anger note talk segment */
static void aRST_talk_ainote_0(NPC_RESET_ACTOR* actor, GAME_PLAY* play) {
    int order = mDemo_Get_OrderValue(mDemo_ORDER_NPC0, 9);

    if (order != 0 && mMsg_CHECK_MAINNORMALCONTINUE() == TRUE) {
        mDemo_Set_OrderValue(mDemo_ORDER_NPC0, 9, 0);
        aRST_change_talk_proc(actor, aRST_TALK_AINOTE_1);
    }
}

/* talk_ainote_1 (156 bytes) — Second anger note talk segment */
static void aRST_talk_ainote_1(NPC_RESET_ACTOR* actor, GAME_PLAY* play) {
    int order = mDemo_Get_OrderValue(mDemo_ORDER_NPC0, 9);

    if (order != 0 && mMsg_CHECK_MAINNORMALCONTINUE() == TRUE) {
        mDemo_Set_OrderValue(mDemo_ORDER_NPC0, 9, 0);
        aRST_change_talk_proc(actor, aRST_TALK_END_WAIT);
    }
}

/* talk_end_wait (88 bytes) — Wait for message to finish */
static void aRST_talk_end_wait(NPC_RESET_ACTOR* actor, GAME_PLAY* play) {
    int order = mDemo_Get_OrderValue(mDemo_ORDER_NPC0, 9);

    if (order != 0 && mMsg_CHECK_MAINNORMALCONTINUE() == TRUE) {
        mDemo_Set_OrderValue(mDemo_ORDER_NPC0, 9, 0);
        actor->talk_end = TRUE;
    }
}

/* talk_end_wait3 (152 bytes) — Extended wait with timer for final talk */
static void aRST_talk_end_wait3(NPC_RESET_ACTOR* actor, GAME_PLAY* play) {
    int order = mDemo_Get_OrderValue(mDemo_ORDER_NPC0, 9);

    if (order != 0 && mMsg_CHECK_MAINNORMALCONTINUE() == TRUE) {
        mDemo_Set_OrderValue(mDemo_ORDER_NPC0, 9, 0);

        actor->timer2++;
        if (actor->timer2 >= 3) {
            actor->talk_end = TRUE;
        } else {
            aRST_change_talk_proc(actor, aRST_TALK_4_1);
        }
    }
}

/* talk_4_1 (100 bytes) — Progressive scolding talk part 1 */
static void aRST_talk_4_1(NPC_RESET_ACTOR* actor, GAME_PLAY* play) {
    int order = mDemo_Get_OrderValue(mDemo_ORDER_NPC0, 9);

    if (order != 0 && mMsg_CHECK_MAINNORMALCONTINUE() == TRUE) {
        mDemo_Set_OrderValue(mDemo_ORDER_NPC0, 9, 0);
        aRST_change_talk_proc(actor, aRST_TALK_4_2);
    }
}

/* talk_4_2 (124 bytes) — Progressive scolding talk part 2 */
static void aRST_talk_4_2(NPC_RESET_ACTOR* actor, GAME_PLAY* play) {
    int order = mDemo_Get_OrderValue(mDemo_ORDER_NPC0, 9);

    if (order != 0 && mMsg_CHECK_MAINNORMALCONTINUE() == TRUE) {
        mDemo_Set_OrderValue(mDemo_ORDER_NPC0, 9, 0);
        aRST_change_talk_proc(actor, aRST_TALK_4_3);
    }
}

/* talk_4_3 (100 bytes) — Progressive scolding talk part 3 */
static void aRST_talk_4_3(NPC_RESET_ACTOR* actor, GAME_PLAY* play) {
    int order = mDemo_Get_OrderValue(mDemo_ORDER_NPC0, 9);

    if (order != 0 && mMsg_CHECK_MAINNORMALCONTINUE() == TRUE) {
        mDemo_Set_OrderValue(mDemo_ORDER_NPC0, 9, 0);
        aRST_change_talk_proc(actor, aRST_TALK_4_4);
    }
}

/* talk_4_4 (120 bytes) — Progressive scolding talk part 4 */
static void aRST_talk_4_4(NPC_RESET_ACTOR* actor, GAME_PLAY* play) {
    int order = mDemo_Get_OrderValue(mDemo_ORDER_NPC0, 9);

    if (order != 0 && mMsg_CHECK_MAINNORMALCONTINUE() == TRUE) {
        mDemo_Set_OrderValue(mDemo_ORDER_NPC0, 9, 0);
        aRST_change_talk_proc(actor, aRST_TALK_4_5);
    }
}

/* talk_4_5 (96 bytes) — Progressive scolding talk part 5, final */
static void aRST_talk_4_5(NPC_RESET_ACTOR* actor, GAME_PLAY* play) {
    int order = mDemo_Get_OrderValue(mDemo_ORDER_NPC0, 9);

    if (order != 0 && mMsg_CHECK_MAINNORMALCONTINUE() == TRUE) {
        mDemo_Set_OrderValue(mDemo_ORDER_NPC0, 9, 0);
        actor->talk_end = TRUE;
    }
}

/* ========================================================================== */
/*  Talk proc dispatch                                                        */
/* ========================================================================== */

/* change_talk_proc (12 bytes) — Switch talk state by index */
static void aRST_change_talk_proc(NPC_RESET_ACTOR* actor, u8 talk_idx) {
    // clang-format off
    static aRST_TALK_PROC talk_proc_table[] = {
        aRST_talk_byebye,
        aRST_talk_exit,
        aRST_talk_stop_player,
        aRST_talk_ainote_0,
        aRST_talk_ainote_1,
        aRST_talk_end_wait,
        aRST_talk_end_wait3,
        aRST_talk_4_1,
        aRST_talk_4_2,
        aRST_talk_4_3,
        aRST_talk_4_4,
        aRST_talk_4_5,
    };
    // clang-format on

    actor->talk_idx = talk_idx;
    actor->talk_proc = talk_proc_table[talk_idx];
}

/* change_talk_proc_next (48 bytes) — Advance to next talk state */
static void aRST_change_talk_proc_next(NPC_RESET_ACTOR* actor) {
    u8 next_idx = actor->talk_idx + 1;

    if (next_idx >= aRST_TALK_NUM) {
        next_idx = aRST_TALK_END_WAIT;
    }

    aRST_change_talk_proc(actor, next_idx);
}

/* ========================================================================== */
/*  Talk request functions                                                    */
/* ========================================================================== */

/* set_force_talk_request (636 bytes) — Build force-talk demo info */
static void aRST_set_force_talk_request(ACTOR* actorx) {
    NPC_RESET_ACTOR* actor = (NPC_RESET_ACTOR*)actorx;
    int reset_count;

    /* Select message based on reset count */
    reset_count = Now_Private->reset_count;
    if (reset_count > 7) {
        reset_count = 7;
    }

    mDemo_Set_msg_num(0x1B3B + reset_count);
    mDemo_Set_talk_turn(TRUE);
    mDemo_Set_camera(CAMERA2_PROCESS_NORMAL);
    mPlib_Set_able_hand_all_item_in_demo(TRUE);
    mBGMPsComp_make_ps_quiet(0);
}

/* force_talk_request (72 bytes) — Request forced talk with player */
static void aRST_force_talk_request(ACTOR* actorx, GAME* game) {
    mDemo_Request(mDemo_TYPE_SPEAK, actorx, aRST_set_force_talk_request);
}

/* set_norm_talk_request (468 bytes) — Build normal talk demo info */
static void aRST_set_norm_talk_request(ACTOR* actorx) {
    NPC_RESET_ACTOR* actor = (NPC_RESET_ACTOR*)actorx;
    int reset_count;

    /* Select message based on reset count */
    reset_count = Now_Private->reset_count;
    if (reset_count > 7) {
        reset_count = 7;
    }

    mDemo_Set_msg_num(0x1B3B + reset_count);
    mDemo_Set_camera(CAMERA2_PROCESS_NORMAL);
    mDemo_Set_talk_turn(TRUE);
}

/* norm_talk_request (72 bytes) — Request normal talk with player */
static void aRST_norm_talk_request(ACTOR* actorx, GAME* game) {
    mDemo_Request(mDemo_TYPE_SPEAK, actorx, aRST_set_norm_talk_request);
}

/* ========================================================================== */
/*  talk_init (100 bytes) — Initialize talk when demo starts                  */
/* ========================================================================== */

static BOOL aRST_talk_init(ACTOR* actorx, GAME* game) {
    NPC_RESET_ACTOR* actor = (NPC_RESET_ACTOR*)actorx;

    mDemo_Set_ListenAble();
    aRST_change_talk_proc(actor, aRST_TALK_STOP_PLAYER);
    actor->talk_end = FALSE;
    actor->timer2 = 0;
    actor->npc_class.talk_info.talk_request_proc = (aNPC_TALK_REQUEST_PROC)none_proc1;

    return TRUE;
}

/* ========================================================================== */
/*  talk_end_chk (296 bytes) — Check if talk has finished                     */
/* ========================================================================== */

static BOOL aRST_talk_end_chk(ACTOR* actorx, GAME* game) {
    NPC_RESET_ACTOR* actor = (NPC_RESET_ACTOR*)actorx;
    GAME_PLAY* play = (GAME_PLAY*)game;
    int ret = FALSE;

    /* Process current talk state */
    if (actor->talk_proc != NULL) {
        actor->talk_proc(actor, play);
    }

    /* Check if talk sequence is done */
    if (!mDemo_Check(mDemo_TYPE_SPEAK, actorx)) {
        /* Talk demo ended */
        aRST_setup_think_proc(actor, play, aRST_THINK_HIDE);
        actor->talk_end = TRUE;
        ret = TRUE;
    } else if (actor->talk_end) {
        ret = TRUE;
    }

    return ret;
}

/* ========================================================================== */
/*  Think state functions                                                     */
/* ========================================================================== */

/* ready_sailing (112 bytes) — Prepare to begin sailing approach */
static void aRST_ready_sailing(NPC_RESET_ACTOR* actor, GAME_PLAY* play) {
    if (actor->timer > 0) {
        actor->timer--;
    } else {
        aRST_setup_think_proc(actor, play, aRST_THINK_SAILING);
    }
}

/* sailing (84 bytes) — Active sailing state, wait for arrival */
static void aRST_sailing(NPC_RESET_ACTOR* actor, GAME_PLAY* play) {
    if (actor->sailing_done) {
        aRST_setup_think_proc(actor, play, aRST_THINK_SETTLEMENT);
    }
}

/* settlement (96 bytes) — Resetti has arrived, begin interaction */
static void aRST_settlement(NPC_RESET_ACTOR* actor, GAME_PLAY* play) {
    actor->npc_class.condition_info.hide_request = FALSE;
    actor->npc_class.talk_info.talk_request_proc = aRST_force_talk_request;
    aRST_setup_think_proc(actor, play, aRST_THINK_HUNT);
}

/* hide (144 bytes) — Resetti is hidden/departing */
static void aRST_hide(NPC_RESET_ACTOR* actor, GAME_PLAY* play) {
    /* Start departure sailing */
    actor->sailing_state = aRST_SAILING_DEPART;
    actor->sailing_timer = 0;
    actor->sailing_t = 0.0f;

    actor->timer++;
    if (actor->timer > 120) {
        Actor_delete((ACTOR*)actor);
    }
}

/* return (84 bytes) — Resetti returns to original position */
static void aRST_return(NPC_RESET_ACTOR* actor, GAME_PLAY* play) {
    if (actor->sailing_state == aRST_SAILING_IDLE) {
        aRST_setup_think_proc(actor, play, aRST_THINK_NORMAL_WAIT);
    }
}

/* reset_ainote (184 bytes) — Play reset anger emote/note */
static void aRST_reset_ainote(NPC_RESET_ACTOR* actor, GAME_PLAY* play) {
    int order = mDemo_Get_OrderValue(mDemo_ORDER_NPC0, 9);

    if (order != 0 && mMsg_CHECK_MAINNORMALCONTINUE() == TRUE) {
        mDemo_Set_OrderValue(mDemo_ORDER_NPC0, 9, 0);
        NPC_CLIP->animation_init_proc((ACTOR*)actor, aNPC_ANIM_WAIT_R1, FALSE);
        aRST_setup_think_proc(actor, play, aRST_THINK_HUNT);
    }
}

/* racket_ainote (184 bytes) — Play racket/brother anger emote */
static void aRST_racket_ainote(NPC_RESET_ACTOR* actor, GAME_PLAY* play) {
    int order = mDemo_Get_OrderValue(mDemo_ORDER_NPC0, 9);

    if (order != 0 && mMsg_CHECK_MAINNORMALCONTINUE() == TRUE) {
        mDemo_Set_OrderValue(mDemo_ORDER_NPC0, 9, 0);
        NPC_CLIP->animation_init_proc((ACTOR*)actor, aNPC_ANIM_APPEAR1, FALSE);
        aRST_setup_think_proc(actor, play, aRST_THINK_REMAIN);
    }
}

/* hunt (56 bytes) — Resetti hunts/chases toward player */
static void aRST_hunt(NPC_RESET_ACTOR* actor, GAME_PLAY* play) {
    ACTOR* playerx = GET_PLAYER_ACTOR_ACTOR(play);

    if (playerx != NULL) {
        actor->npc_class.movement.mv_angl = actor->npc_class.actor_class.player_angle_y;
    }
}

/* remain (80 bytes) — Resetti remains in place, waiting */
static void aRST_remain(NPC_RESET_ACTOR* actor, GAME_PLAY* play) {
    actor->timer++;
    if (actor->timer > 300) {
        aRST_setup_think_proc(actor, play, aRST_THINK_HIDE);
    }
}

/* timer (68 bytes) — Generic timer-based state transition */
static void aRST_timer(NPC_RESET_ACTOR* actor, GAME_PLAY* play) {
    actor->timer++;
    if (actor->timer > 60) {
        aRST_setup_think_proc(actor, play, aRST_THINK_END);
    }
}

/* ========================================================================== */
/*  Think proc dispatch                                                       */
/* ========================================================================== */

/* think_main_proc (40 bytes) — Main think processing */
static void aRST_think_main_proc(NPC_ACTOR* nactorx, GAME_PLAY* play) {
    NPC_RESET_ACTOR* actor = (NPC_RESET_ACTOR*)nactorx;

    if (nactorx->action.step == aNPC_ACTION_END_STEP && actor->think_proc != NULL) {
        actor->think_proc(actor, play);
    }
}

/* think_init_proc (292 bytes) — Initialize think state with defaults */
static void aRST_think_init_proc(NPC_ACTOR* nactorx, GAME_PLAY* play) {
    NPC_RESET_ACTOR* actor = (NPC_RESET_ACTOR*)nactorx;
    ACTOR* actorx = (ACTOR*)nactorx;

    /* Set default action request */
    aRST_set_request_act(actor, 1, aNPC_ACT_SPECIAL, aNPC_ACT_TYPE_SEARCH, 0, 0, 0);
    actorx->status_data.weight = MASSTYPE_HEAVY;

    /* Default position and animation */
    NPC_CLIP->animation_init_proc(actorx, aNPC_ANIM_WAIT1, FALSE);

    /* Set up initial think */
    actor->npc_class.condition_info.hide_request = TRUE;
    actor->npc_class.talk_info.talk_request_proc = (aNPC_TALK_REQUEST_PROC)none_proc1;
    aRST_setup_think_proc(actor, play, aRST_THINK_START);
}

/* ========================================================================== */
/*  Think init functions — called when entering a think state                 */
/* ========================================================================== */

/* normal_wait_init (156 bytes) — Initialize normal wait state */
static void aRST_normal_wait_init(NPC_RESET_ACTOR* actor, GAME_PLAY* play) {
    actor->npc_class.condition_info.hide_request = FALSE;
    actor->timer = 0;
    NPC_CLIP->animation_init_proc((ACTOR*)actor, aNPC_ANIM_WAIT1, FALSE);

    actor->npc_class.talk_info.talk_request_proc = aRST_norm_talk_request;
    actor->npc_class.talk_info.default_animation = aNPC_ANIM_WAIT1;
    actor->npc_class.talk_info.default_turn_animation = aNPC_ANIM_WAIT1;
}

/* ready_sailing_init (124 bytes) — Setup approach sailing parameters */
static void aRST_ready_sailing_init(NPC_RESET_ACTOR* actor, GAME_PLAY* play) {
    ACTOR* actorx = (ACTOR*)actor;

    actor->sailing_start.x = actorx->world.position.x - 400.0f;
    actor->sailing_start.y = actorx->world.position.y;
    actor->sailing_start.z = actorx->world.position.z;
    actor->sailing_target = actorx->world.position;

    actor->sailing_state = aRST_SAILING_APPROACH;
    actor->sailing_timer = 0;
    actor->sailing_t = 0.0f;
    actor->sailing_done = FALSE;
    actor->timer = 30;
}

/* sailing_init (72 bytes) — Begin active sailing */
static void aRST_sailing_init(NPC_RESET_ACTOR* actor, GAME_PLAY* play) {
    actor->npc_class.condition_info.hide_request = TRUE;
    NPC_CLIP->animation_init_proc((ACTOR*)actor, aNPC_ANIM_WAIT1, FALSE);
}

/* settlement_init (36 bytes) — Settlement state init */
static void aRST_settlement_init(NPC_RESET_ACTOR* actor, GAME_PLAY* play) {
    actor->npc_class.condition_info.hide_request = FALSE;
}

/* hide_init (12 bytes) — Hide state init */
static void aRST_hide_init(NPC_RESET_ACTOR* actor, GAME_PLAY* play) {
    actor->timer = 0;
}

/* return_init (72 bytes) — Return state init */
static void aRST_return_init(NPC_RESET_ACTOR* actor, GAME_PLAY* play) {
    actor->sailing_state = aRST_SAILING_DEPART;
    actor->sailing_timer = 0;
    actor->sailing_t = 0.0f;
}

/* end_init (56 bytes) — End state init */
static void aRST_end_init(NPC_RESET_ACTOR* actor, GAME_PLAY* play) {
    actor->npc_class.condition_info.hide_request = TRUE;
    actor->npc_class.talk_info.talk_request_proc = (aNPC_TALK_REQUEST_PROC)none_proc1;
}

/* start_init (108 bytes) — First start state init */
static void aRST_start_init(NPC_RESET_ACTOR* actor, GAME_PLAY* play) {
    actor->npc_class.condition_info.hide_request = TRUE;
    actor->npc_class.talk_info.talk_request_proc = (aNPC_TALK_REQUEST_PROC)none_proc1;
    NPC_CLIP->animation_init_proc((ACTOR*)actor, aNPC_ANIM_WAIT1, FALSE);
    aRST_setup_think_proc(actor, play, aRST_THINK_READY_SAILING);
}

/* start2_init (108 bytes) — Second start state init */
static void aRST_start2_init(NPC_RESET_ACTOR* actor, GAME_PLAY* play) {
    actor->npc_class.condition_info.hide_request = TRUE;
    actor->npc_class.talk_info.talk_request_proc = aRST_force_talk_request;
    NPC_CLIP->animation_init_proc((ACTOR*)actor, aNPC_ANIM_APPEAR1, FALSE);
    actor->npc_class.talk_info.default_animation = aNPC_ANIM_APPEAR1;
    actor->npc_class.talk_info.default_turn_animation = aNPC_ANIM_APPEAR1;
}

/* start3_init (108 bytes) — Third start state init */
static void aRST_start3_init(NPC_RESET_ACTOR* actor, GAME_PLAY* play) {
    actor->npc_class.condition_info.hide_request = FALSE;
    actor->npc_class.talk_info.talk_request_proc = aRST_force_talk_request;
    NPC_CLIP->animation_init_proc((ACTOR*)actor, aNPC_ANIM_WAIT_R1, FALSE);
    actor->npc_class.talk_info.default_animation = aNPC_ANIM_WAIT_R1;
    actor->npc_class.talk_info.default_turn_animation = aNPC_ANIM_WAIT_R1;
}

/* timer_sailing_init (12 bytes) — Timer sailing state init (minimal) */
static void aRST_timer_sailing_init(NPC_RESET_ACTOR* actor, GAME_PLAY* play) {
    actor->timer = 0;
}

/* ========================================================================== */
/*  setup_think_proc (156 bytes)                                              */
/*  Set up the think state by index, calling the appropriate init function.   */
/* ========================================================================== */

typedef void (*aRST_THINK_INIT_PROC)(NPC_RESET_ACTOR* actor, GAME_PLAY* play);

typedef struct {
    aRST_THINK_PROC think_proc;
    aRST_THINK_INIT_PROC think_init_proc;
} aRST_think_data_c;

static void aRST_setup_think_proc(NPC_RESET_ACTOR* actor, GAME_PLAY* play, u8 think_idx) {
    // clang-format off
    static aRST_think_data_c think_data_table[] = {
        { (aRST_THINK_PROC)none_proc1,           aRST_normal_wait_init     }, /* NORMAL_WAIT     */
        { aRST_ready_sailing,                     aRST_ready_sailing_init   }, /* READY_SAILING   */
        { aRST_sailing,                           aRST_sailing_init         }, /* SAILING         */
        { aRST_settlement,                        aRST_settlement_init      }, /* SETTLEMENT      */
        { aRST_hide,                              aRST_hide_init            }, /* HIDE            */
        { aRST_return,                            aRST_return_init          }, /* RETURN          */
        { aRST_reset_ainote,                      (aRST_THINK_INIT_PROC)none_proc1 }, /* RESET_AINOTE   */
        { aRST_racket_ainote,                     (aRST_THINK_INIT_PROC)none_proc1 }, /* RACKET_AINOTE  */
        { aRST_hunt,                              (aRST_THINK_INIT_PROC)none_proc1 }, /* HUNT           */
        { aRST_remain,                            (aRST_THINK_INIT_PROC)none_proc1 }, /* REMAIN         */
        { aRST_timer,                             (aRST_THINK_INIT_PROC)none_proc1 }, /* TIMER          */
        { (aRST_THINK_PROC)none_proc1,           aRST_start_init           }, /* START           */
        { (aRST_THINK_PROC)none_proc1,           aRST_start2_init          }, /* START2          */
        { (aRST_THINK_PROC)none_proc1,           aRST_start3_init          }, /* START3          */
        { (aRST_THINK_PROC)none_proc1,           aRST_end_init             }, /* END             */
        { (aRST_THINK_PROC)none_proc1,           aRST_timer_sailing_init   }, /* TIMER_SAILING   */
    };
    // clang-format on

    aRST_think_data_c* data = &think_data_table[think_idx];

    actor->think_idx = think_idx;
    actor->think_proc = data->think_proc;
    actor->timer = 0;
    (*data->think_init_proc)(actor, play);
}

/* ========================================================================== */
/*  think_proc (64 bytes) — Top-level think dispatch                          */
/* ========================================================================== */

static void aRST_think_proc(NPC_ACTOR* nactorx, GAME_PLAY* play, int type) {
    static aNPC_SUB_PROC think_proc_table[] = { aRST_think_init_proc, aRST_think_main_proc };

    (*think_proc_table[type])(nactorx, play);
}

/* ========================================================================== */
/*  Schedule functions                                                        */
/* ========================================================================== */

/* schedule_init_proc (76 bytes) — Initialize schedule with think proc */
static void aRST_schedule_init_proc(NPC_ACTOR* nactorx, GAME_PLAY* play) {
    nactorx->think.think_proc = aRST_think_proc;
    NPC_CLIP->think_proc(nactorx, play, aNPC_THINK_SPECIAL, aNPC_THINK_TYPE_INIT);
}

/* schedule_main_proc (120 bytes) — Main schedule processing */
static void aRST_schedule_main_proc(NPC_ACTOR* nactorx, GAME_PLAY* play) {
    if (!NPC_CLIP->think_proc(nactorx, play, -1, aNPC_THINK_TYPE_CHK_INTERRUPT)) {
        NPC_CLIP->think_proc(nactorx, play, -1, aNPC_THINK_TYPE_MAIN);
    }
}

/* schedule_proc (64 bytes) — Top-level schedule dispatch */
static void aRST_schedule_proc(NPC_ACTOR* nactorx, GAME_PLAY* play, int type) {
    static aNPC_SUB_PROC sche_proc[] = { aRST_schedule_init_proc, aRST_schedule_main_proc };

    (*sche_proc[type])(nactorx, play);
}

#endif /* VERSION >= VER_DELUXE */
