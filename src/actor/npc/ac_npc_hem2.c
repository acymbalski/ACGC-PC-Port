#include "ac_npc_hem2.h"

#if VERSION >= VER_DELUXE

#include "m_name_table.h"
#include "ac_npc.h"
#include "ac_npc_h.h"
#include "m_common_data.h"
#include "m_msg.h"
#include "m_soncho.h"
#include "ac_shrine.h"

/* ========================================================================== */
/*  Gracie variant 2 (Deluxe-only)                                            */
/*  Simplified reskin of the hem (Gracie) NPC for the Deluxe version.          */
/*  Uses the same basic structure as ac_npc_hem.c with a modified schedule.    */
/* ========================================================================== */

enum {
    aNHM2_ACT_APPEAR_WAIT,
    aNHM2_ACT_DISAPPEAR_WAIT,

    aNHM2_ACT_NUM
};

typedef struct npc_hem2_actor_s NPC_HEM2_ACTOR;

typedef void (*aNHM2_PROC)(NPC_HEM2_ACTOR* actor, GAME_PLAY* play);

typedef struct npc_hem2_actor_s {
    NPC_ACTOR actor;
    int action;
    aNHM2_PROC act_proc;
    u8 disappear_flag;
} NPC_HEM2_ACTOR;

static void aNHM2_actor_ct(ACTOR* actorx, GAME* game);
static void aNHM2_actor_dt(ACTOR* actorx, GAME* game);
static void aNHM2_actor_init(ACTOR* actorx, GAME* game);
static void aNHM2_actor_save(ACTOR* actorx, GAME* game);
static void aNHM2_actor_move(ACTOR* actorx, GAME* game);
static void aNHM2_actor_draw(ACTOR* actorx, GAME* game);
static void aNHM2_schedule_proc(NPC_ACTOR*, GAME_PLAY*, int);

// clang-format off
ACTOR_PROFILE Npc_Hem2_Profile = {
    mAc_PROFILE_NPC_HEM2,
    ACTOR_PART_NPC,
    ACTOR_STATE_NONE,
    SP_NPC_HEM,
    ACTOR_OBJ_BANK_KEEP,
    sizeof(NPC_HEM2_ACTOR),
    aNHM2_actor_ct,
    aNHM2_actor_dt,
    aNHM2_actor_init,
    mActor_NONE_PROC1,
    aNHM2_actor_save
};
// clang-format on

/* ========================================================================== */
/*  Lifecycle                                                                  */
/* ========================================================================== */

static void aNHM2_actor_ct(ACTOR* actorx, GAME* game) {
    // clang-format off
    static aNPC_ct_data_c ct_data = {
        aNHM2_actor_move,
        aNHM2_actor_draw,
        aNPC_CT_SCHED_TYPE_SPECIAL,
        (aNPC_TALK_REQUEST_PROC)none_proc1,
        NULL,
        NULL,
        0
    };
    // clang-format on

    if (NPC_CLIP->birth_check_proc(actorx, game) == TRUE) {
        NPC_HEM2_ACTOR* actor = (NPC_HEM2_ACTOR*)actorx;

        actor->actor.schedule.schedule_proc = aNHM2_schedule_proc;
        NPC_CLIP->ct_proc(actorx, game, &ct_data);
    }
}

static void aNHM2_actor_save(ACTOR* actorx, GAME* game) {
    NPC_CLIP->save_proc(actorx, game);
}

static void aNHM2_actor_dt(ACTOR* actorx, GAME* game) {
    NPC_CLIP->dt_proc(actorx, game);
}

static void aNHM2_actor_init(ACTOR* actorx, GAME* game) {
    NPC_CLIP->init_proc(actorx, game);
}

/* ========================================================================== */
/*  Draw                                                                       */
/* ========================================================================== */

static void aNHM2_actor_draw(ACTOR* actorx, GAME* game) {
    NPC_CLIP->draw_proc(actorx, game);
}

/* ========================================================================== */
/*  Action request                                                             */
/* ========================================================================== */

static void aNHM2_set_request_act(NPC_HEM2_ACTOR* hem) {
    hem->actor.request.act_priority = 4;
    hem->actor.request.act_idx = aNPC_ACT_SPECIAL;
    hem->actor.request.act_type = aNPC_ACT_TYPE_DEFAULT;
}

/* ========================================================================== */
/*  Action state machine                                                       */
/* ========================================================================== */

static void aNHM2_appear_wait(NPC_HEM2_ACTOR* hem, GAME_PLAY* play) {
    if (Common_Get(hem_visible) == TRUE) {
        hem->actor.condition_info.hide_request = FALSE;
    }
}

static void aNHM2_disappear_wait(NPC_HEM2_ACTOR* hem, GAME_PLAY* play) {
    if (Common_Get(hem_visible) == FALSE) {
        hem->disappear_flag = TRUE;
    }
}

static void aNHM2_setupAction(NPC_HEM2_ACTOR* hem, int action) {
    static aNHM2_PROC process[] = { aNHM2_appear_wait, aNHM2_disappear_wait };

    hem->actor.action.step = 0;
    hem->action = action;
    hem->act_proc = process[action];
    NPC_CLIP->animation_init_proc(&hem->actor.actor_class, aNPC_ANIM_WAIT1, FALSE);
}

static void aNHM2_act_chg_data_proc(NPC_ACTOR* actorx, GAME_PLAY* play) {
    NPC_HEM2_ACTOR* hem = (NPC_HEM2_ACTOR*)actorx;
    hem->actor.action.act_obj = aNPC_ACT_OBJ_PLAYER;
}

static void aNHM2_act_init_proc(NPC_ACTOR* actorx, GAME_PLAY* play) {
    NPC_HEM2_ACTOR* hem = (NPC_HEM2_ACTOR*)actorx;
    int act = aNHM2_ACT_APPEAR_WAIT;

    if (hem->disappear_flag == TRUE) {
        act = aNHM2_ACT_DISAPPEAR_WAIT;
    }

    aNHM2_setupAction(hem, act);
}

static void aNHM2_act_main_proc(NPC_ACTOR* actorx, GAME_PLAY* play) {
    NPC_HEM2_ACTOR* hem = (NPC_HEM2_ACTOR*)actorx;

    hem->act_proc(hem, play);
}

static void aNHM2_act_proc(NPC_ACTOR* actorx, GAME_PLAY* play, int action) {
    static aNPC_SUB_PROC act_proc[] = { aNHM2_act_init_proc, aNHM2_act_chg_data_proc, aNHM2_act_main_proc };
    act_proc[action](actorx, play);
}

/* ========================================================================== */
/*  Think state machine                                                        */
/* ========================================================================== */

static void aNHM2_think_main_proc(NPC_ACTOR* actorx, GAME_PLAY* play) {
    NPC_HEM2_ACTOR* hem = (NPC_HEM2_ACTOR*)actorx;
    if (hem->actor.action.step == aNPC_ACTION_END_STEP) {
        hem->actor.condition_info.demo_flg = aNPC_COND_DEMO_SKIP_HEAD_LOOKAT | aNPC_COND_DEMO_SKIP_FORWARD_CHECK |
                                             aNPC_COND_DEMO_SKIP_BGCHECK | aNPC_COND_DEMO_SKIP_MOVE_RANGE_CHECK |
                                             aNPC_COND_DEMO_SKIP_MOVE_CIRCLE_REV | aNPC_COND_DEMO_SKIP_MOVE_Y;
        hem->actor.collision.check_kind = aNPC_BG_CHECK_TYPE_ONLY_GROUND;
        aNHM2_set_request_act(hem);
    }
}

static void aNHM2_think_init_proc(NPC_ACTOR* actorx, GAME_PLAY* play) {
    NPC_HEM2_ACTOR* hem = (NPC_HEM2_ACTOR*)actorx;
    hem->actor.action.act_proc = aNHM2_act_proc;
    aNHM2_set_request_act(hem);
}

static void aNHM2_think_proc(NPC_ACTOR* actorx, GAME_PLAY* play, int action) {
    static aNPC_SUB_PROC think_proc[] = { aNHM2_think_init_proc, aNHM2_think_main_proc };
    think_proc[action](actorx, play);
}

/* ========================================================================== */
/*  Schedule                                                                   */
/* ========================================================================== */

static void aNHM2_schedule_init_proc(NPC_ACTOR* actorx, GAME_PLAY* play) {
    NPC_HEM2_ACTOR* hem = (NPC_HEM2_ACTOR*)actorx;
    hem->actor.think.think_proc = aNHM2_think_proc;
    hem->actor.palActorIgnoreTimer = -1;
    hem->actor.condition_info.demo_flg = aNPC_COND_DEMO_SKIP_HEAD_LOOKAT | aNPC_COND_DEMO_SKIP_FORWARD_CHECK |
                                         aNPC_COND_DEMO_SKIP_BGCHECK | aNPC_COND_DEMO_SKIP_MOVE_RANGE_CHECK |
                                         aNPC_COND_DEMO_SKIP_MOVE_CIRCLE_REV | aNPC_COND_DEMO_SKIP_MOVE_Y;
    hem->actor.collision.check_kind = aNPC_BG_CHECK_TYPE_ONLY_GROUND;
    hem->actor.actor_class.status_data.weight = MASSTYPE_HEAVY;
    hem->actor.actor_class.world.position.x += 20.f;
    hem->actor.actor_class.world.position.z -= 20.f;
    NPC_CLIP->think_proc(&hem->actor, play, aNPC_THINK_SPECIAL, aNPC_THINK_TYPE_INIT);
}

static void aNHM2_schedule_main_proc(NPC_ACTOR* actorx, GAME_PLAY* play) {
    if (NPC_CLIP->think_proc(actorx, play, -1, aNPC_THINK_TYPE_CHK_INTERRUPT) == FALSE) {
        NPC_CLIP->think_proc(actorx, play, -1, aNPC_THINK_TYPE_MAIN);
    }
}

static void aNHM2_schedule_proc(NPC_ACTOR* actorx, GAME_PLAY* play, int proc) {
    static aNPC_SUB_PROC sche_proc[] = { aNHM2_schedule_init_proc, aNHM2_schedule_main_proc };
    sche_proc[proc](actorx, play);
}

/* ========================================================================== */
/*  Move                                                                       */
/* ========================================================================== */

static void aNHM2_actor_move(ACTOR* actorx, GAME* game) {
    NPC_HEM2_ACTOR* hem = (NPC_HEM2_ACTOR*)actorx;

    NPC_CLIP->move_proc(actorx, game);
    if (hem->disappear_flag == TRUE) {
        Actor_delete(actorx);
    }
}

#endif /* VERSION >= VER_DELUXE */
