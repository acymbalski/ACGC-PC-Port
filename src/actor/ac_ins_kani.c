/**
 * @file ac_ins_kani.c
 * @brief Hermit crab (kani) insect actor — Deluxe edition only.
 *
 * The hermit crab is a beach-dwelling insect that hides in holes and shells,
 * can be dug up with a shovel, falls from palm trees when shaken, avoids the
 * player, and escapes into water. It features a complex state machine with
 * palm tree (yasi) states for the coconut-tree variant.
 *
 * 51 functions, 7,880 bytes total in the Deluxe REL.
 */

#include "ac_ins_kani.h"

#include "m_field_info.h"
#include "m_name_table.h"
#include "m_common_data.h"
#include "m_player_lib.h"
#include "m_collision_bg.h"
#include "ac_set_ovl_insect.h"
#include "ef_effect_control.h"

#if VERSION >= VER_DELUXE

/* ========================================================================
 * Action enum — state machine states for the hermit crab
 * ======================================================================== */

enum {
    aIKN_ACTION_AVOID,
    aIKN_ACTION_LET_ESCAPE,
    aIKN_ACTION_HIDE_KANI,
    aIKN_ACTION_WAIT,
    aIKN_ACTION_MOVE,
    aIKN_ACTION_DUG,
    aIKN_ACTION_APPEAR_KANI,
    aIKN_ACTION_APPEAR_FROM_SHELL,
    aIKN_ACTION_INTO_SHELL,
    aIKN_ACTION_CAMOUFLAGE,
    aIKN_ACTION_TURN,
    aIKN_ACTION_DIVE,
    aIKN_ACTION_DROWN,
    aIKN_ACTION_ESCAPE_CONT,
    aIKN_ACTION_HIDE_YASI,
    aIKN_ACTION_FALL_YASI,
    aIKN_ACTION_BOUND_YASI,
    aIKN_ACTION_STRUGGLE_YASI,
    aIKN_ACTION_ROLL_YASI,

    aIKN_ACTION_NUM
};

/* ========================================================================
 * Work variable aliases
 * ======================================================================== */

#define aIKN_TIMER0(ins)        ((ins)->s32_work0)
#define aIKN_TIMER1(ins)        ((ins)->s32_work1)
#define aIKN_ANIM_FRAME(ins)    ((ins)->s32_work2)
#define aIKN_YASI_BOUNCE(ins)   ((ins)->s32_work3)

#define aIKN_BK_CENTER_X(ins)   ((ins)->f32_work2)
#define aIKN_BK_CENTER_Z(ins)   ((ins)->f32_work3)
#define aIKN_SHELL_TIMER(ins)   ((ins)->f32_work0)
#define aIKN_BOUND_VEL(ins)     ((ins)->f32_work1)

/* ========================================================================
 * Forward declarations
 * ======================================================================== */

static void aIKN_actor_move(ACTOR* actorx, GAME* game);
static void aIKN_setupAction(aINS_INSECT_ACTOR* insect, int action, GAME* game);

/* ========================================================================
 * Utility functions
 * ======================================================================== */

/**
 * Chase a target angle on the Y axis.
 * Signature: aIKN_chase_angleY(insect_actor_s*, s16, s16) — 72 bytes
 */
static void aIKN_chase_angleY(aINS_INSECT_ACTOR* insect, s16 target, s16 step) {
    chase_angle(&insect->tools_actor.actor_class.shape_info.rotation.y, target, step);
}

/**
 * Check if an angle is within a valid shape range.
 * Signature: aIKN_chk_shape_angle(s16) — 96 bytes
 */
static int aIKN_chk_shape_angle(s16 angle) {
    s16 abs_angle;
    int ret;

    ret = FALSE;
    abs_angle = angle;
    if (abs_angle < 0) {
        abs_angle = -abs_angle;
    }

    if (abs_angle < DEG2SHORT_ANGLE2(45.0f)) {
        ret = TRUE;
    } else if (abs_angle > DEG2SHORT_ANGLE2(135.0f)) {
        ret = TRUE;
    }

    return ret;
}

/**
 * Spawn mud/sand dig effect at the insect's position.
 * Signature: aIKN_set_mud_effect(actor_s*, game_s*) — 164 bytes
 */
static void aIKN_set_mud_effect(ACTOR* actorx, GAME* game) {
    xyz_t pos;
    s16 random_angle;
    int i;

    xyz_t_move(&pos, &actorx->world.position);
    random_angle = (s16)(RANDOM_F(DEG2SHORT_ANGLE2(360.0f)));

    for (i = 3; i < 6; i++) {
        eEC_CLIP->effect_make_proc(eEC_EFFECT_DIG_MUD, pos, 2,
                                   random_angle, game, RSV_NO, 0, 0x4000 | i);
    }
}

/**
 * Spawn water splash effect at the insect's position.
 * Signature: aIKN_set_water_effect(actor_s*, game_s*) — 216 bytes
 */
static void aIKN_set_water_effect(ACTOR* actorx, GAME* game) {
    xyz_t pos;
    f32 water_y;

    xyz_t_move(&pos, &actorx->world.position);
    water_y = mCoBG_GetWaterHeight_File(pos, "ac_ins_kani.c", 140);

    if (water_y > pos.y) {
        pos.y = water_y;
    }

    eEC_CLIP->effect_make_proc(eEC_EFFECT_TURI_MIZU, pos, 1,
                               actorx->world.angle.y, game, EMPTY_NO, 4, 0);
    sAdo_OngenTrgStart(NA_SE_438, &actorx->world.position);
}

/**
 * Check if the player's scoop/shovel is near this insect.
 * Signature: aIKN_chk_player_scoop(actor_s*) — 128 bytes
 */
static int aIKN_chk_player_scoop(ACTOR* actorx) {
    xyz_t scoop_pos;
    f32 dx, dz;
    int ret;

    ret = FALSE;
    if (mPlib_Check_DigScoop(&scoop_pos) == TRUE) {
        dx = scoop_pos.x - actorx->world.position.x;
        dz = scoop_pos.z - actorx->world.position.z;

        if ((SQ(dx) + SQ(dz)) < SQ(70.0f)) {
            ret = TRUE;
        }
    }

    return ret;
}

/**
 * Set the movement angle away from the player, with optional random offset.
 * Signature: aIKN_set_angle(insect_actor_s*, s16, s16) — 192 bytes
 */
static void aIKN_set_angle(aINS_INSECT_ACTOR* insect, s16 target, s16 step) {
    ACTOR* actorx = &insect->tools_actor.actor_class;

    if (actorx->bg_collision_check.result.hit_wall & mCoBG_HIT_WALL_FRONT) {
        int count = actorx->bg_collision_check.result.hit_wall_count & 7;
        int i;

        if (count != 0) {
            for (i = 0; i < count; i++) {
                if (actorx->bg_collision_check.wall_info[i].type == 0) {
                    target = DEG2SHORT_ANGLE2(90.0f) + actorx->bg_collision_check.wall_info[i].angleY;
                    break;
                }
            }
        }
    }

    actorx->world.angle.y = target;
    chase_angle(&actorx->shape_info.rotation.y, target, step);
}

/**
 * Set the angle to avoid the player. mode=0 is away from player, mode=1 uses player facing.
 * Signature: aIKN_set_avoid_player_angl(insect_actor_s*, game_s*, int) — 244 bytes
 */
static void aIKN_set_avoid_player_angl(aINS_INSECT_ACTOR* insect, GAME* game, int mode) {
    ACTOR* actorx = &insect->tools_actor.actor_class;
    ACTOR* playerx = GET_PLAYER_ACTOR_GAME_ACTOR(game);
    s16 angle;

    if (playerx != NULL) {
        if (mode == 0) {
            /* Run away from the player */
            f32 dx = actorx->world.position.x - playerx->world.position.x;
            f32 dz = actorx->world.position.z - playerx->world.position.z;
            angle = atans_table(dx, dz);
            angle += (s16)RANDOM_CENTER_F(DEG2SHORT_ANGLE2(60.0f));
        } else {
            /* Use player's facing direction + random offset */
            angle = playerx->shape_info.rotation.y + (s16)RANDOM_CENTER_F(DEG2SHORT_ANGLE2(120.0f));
        }

        actorx->world.angle.y = angle;
        actorx->shape_info.rotation.y = angle;
    }
}

/**
 * Check patience — whether the player has frightened the crab enough to flee.
 * Signature: aIKN_chk_patience(insect_actor_s*, game_s*) — 384 bytes
 */
static int aIKN_chk_patience(aINS_INSECT_ACTOR* insect, GAME* game) {
    ACTOR* actorx = &insect->tools_actor.actor_class;
    xyz_t net_pos;
    xyz_t scoop_pos;
    f32 dx, dz;
    int ret;

    ret = FALSE;

    /* Check tree shaking near crab */
    if (mPlib_Check_tree_shaken(&actorx->world.position) == TRUE) {
        insect->patience = 100.0f;
    }
    /* Check vibration from footsteps */
    else if (mPlib_Check_VibUnit_OneFrame(&actorx->world.position) == TRUE &&
             actorx->player_distance_xz < 150.0f) {
        insect->patience = 100.0f;
    }
    /* Check ball nearby */
    else {
        dx = Common_Get(ball_pos).x - actorx->world.position.x;
        dz = Common_Get(ball_pos).z - actorx->world.position.z;
        if ((SQ(dx) + SQ(dz)) < SQ(60.0f)) {
            insect->patience = 100.0f;
        }
    }

    /* Check net nearby */
    if (insect->patience < 90.0f) {
        if (mPlib_Check_StopNet(&net_pos) == TRUE) {
            dx = net_pos.x - actorx->world.position.x;
            dz = net_pos.z - actorx->world.position.z;
            if ((SQ(dx) + SQ(dz)) < SQ(70.0f)) {
                insect->patience = 100.0f;
            }
        }
    }

    /* Check scoop/shovel nearby */
    if (insect->patience < 90.0f) {
        if (mPlib_Check_DigScoop(&scoop_pos) == TRUE) {
            dx = scoop_pos.x - actorx->world.position.x;
            dz = scoop_pos.z - actorx->world.position.z;
            if ((SQ(dx) + SQ(dz)) < SQ(70.0f)) {
                insect->patience = 100.0f;
            }
        }
    }

    /* Check axe hit nearby */
    if (insect->patience < 90.0f) {
        xyz_t axe_pos;
        if (mPlib_Check_HitAxe(&axe_pos) == TRUE) {
            dx = axe_pos.x - actorx->world.position.x;
            dz = axe_pos.z - actorx->world.position.z;
            if ((SQ(dx) + SQ(dz)) < SQ(70.0f)) {
                insect->patience = 100.0f;
            }
        }
    }

    if (insect->patience > 90.0f) {
        ret = TRUE;
    }

    return ret;
}

/**
 * Animate the crab's walk cycle.
 * Signature: aIKN_anime_proc(insect_actor_s*) — 164 bytes
 */
static void aIKN_anime_proc(aINS_INSECT_ACTOR* insect) {
    ACTOR* actorx = &insect->tools_actor.actor_class;

    aIKN_ANIM_FRAME(insect)++;
    if (aIKN_ANIM_FRAME(insect) >= 8) {
        aIKN_ANIM_FRAME(insect) = 0;
    }

    /* Sideways scuttle oscillation */
    insect->_1E0 += 0.3f;
    if (insect->_1E0 >= 2.0f) {
        insect->_1E0 -= 2.0f;
    }

    /* Vertical bobbing */
    {
        f32 bob = sin_s((s16)(aIKN_ANIM_FRAME(insect) * 0x2000)) * 0.5f;
        actorx->world.position.y += bob;
    }
}

/**
 * Fade-out alpha calculation for disappearing.
 * Signature: aIKN_calc_alpha(insect_actor_s*) — 180 bytes
 */
static void aIKN_calc_alpha(aINS_INSECT_ACTOR* insect) {
    if (insect->alpha_time > 0) {
        insect->life_time++;
        if (insect->life_time >= insect->alpha_time) {
            int alpha = 255 - ((insect->life_time - insect->alpha_time) * 4);
            if (alpha < 0) {
                alpha = 0;
                insect->insect_flags.destruct = TRUE;
            }
            insect->alpha0 = alpha;
        }
    }
}

/**
 * Adjust movement angle when hitting walls.
 * Signature: aIKN_calc_direction_angl(insect_actor_s*) — 148 bytes
 */
static void aIKN_calc_direction_angl(aINS_INSECT_ACTOR* insect) {
    ACTOR* actorx = &insect->tools_actor.actor_class;

    if (actorx->bg_collision_check.result.hit_wall & mCoBG_HIT_WALL_FRONT) {
        int count = actorx->bg_collision_check.result.hit_wall_count & 7;

        if (count != 0) {
            int i;
            for (i = 0; i < count; i++) {
                if (actorx->bg_collision_check.wall_info[i].type == 0) {
                    actorx->world.angle.y = DEG2SHORT_ANGLE2(90.0f) + actorx->bg_collision_check.wall_info[i].angleY;
                    break;
                }
            }
        }
    }

    chase_angle(&actorx->shape_info.rotation.y, actorx->world.angle.y, 0x800);
}

/**
 * Check whether the insect stepped on a dug hole.
 * Signature: aIKN_chk_dug_attr(insect_actor_s*) — 148 bytes
 */
static int aIKN_chk_dug_attr(aINS_INSECT_ACTOR* insect) {
    ACTOR* actorx = &insect->tools_actor.actor_class;
    u32 attr;
    int ret;

    ret = FALSE;
    attr = mCoBG_Wpos2Attribute(actorx->world.position, NULL);
    if (mCoBG_CheckHole_OrgAttr(attr) == TRUE) {
        ret = TRUE;
    }

    /* Also check if on sand/beach attribute */
    if (ret == FALSE) {
        u32 bg_attr = mCoBG_Wpos2BgAttribute_Original(actorx->world.position);
        if (mCoBG_CheckWaterAttribute(bg_attr)) {
            ret = 2;
        }
    }

    return ret;
}

/**
 * Check if insect is within active range of spawn point.
 * Signature: aIKN_chk_active_range(insect_actor_s*) — 184 bytes
 */
static int aIKN_chk_active_range(aINS_INSECT_ACTOR* insect) {
    ACTOR* actorx = &insect->tools_actor.actor_class;
    f32 dx, dz;
    int ret;

    ret = TRUE;
    dx = aIKN_BK_CENTER_X(insect) - actorx->world.position.x;
    dz = aIKN_BK_CENTER_Z(insect) - actorx->world.position.z;

    if ((SQ(dx) + SQ(dz)) >= SQ(400.0f)) {
        ret = FALSE;
    }

    return ret;
}

/**
 * Check if a tree near the crab was cut down.
 * Signature: aIKN_chk_cut_tree(insect_actor_s*) — 188 bytes
 */
static int aIKN_chk_cut_tree(aINS_INSECT_ACTOR* insect) {
    ACTOR* actorx = &insect->tools_actor.actor_class;
    mActor_name_t* fg_p;
    int ret;

    ret = FALSE;
    fg_p = mFI_GetUnitFG(actorx->world.position);
    if (fg_p != NULL) {
        if (IS_ITEM_ANY_PALM_TREE(*fg_p)) {
            /* Palm tree is still here */
            ret = FALSE;
        } else {
            /* Palm tree was cut — the crab should fall */
            ret = TRUE;
        }
    } else {
        ret = TRUE;
    }

    return ret;
}

/**
 * Check if a palm tree near the crab was shaken.
 * Signature: aIKN_chk_shake_tree(insect_actor_s*) — 76 bytes
 */
static int aIKN_chk_shake_tree(aINS_INSECT_ACTOR* insect) {
    ACTOR* actorx = &insect->tools_actor.actor_class;
    int ret;

    ret = FALSE;
    if (mPlib_Check_tree_shaken(&actorx->world.position) == TRUE) {
        ret = TRUE;
    }

    return ret;
}

/**
 * Set the palm tree (yasi) starting position for the crab.
 * Signature: aIKN_set_yasi_start_pos(insect_actor_s*, game_s*) — 212 bytes
 */
static void aIKN_set_yasi_start_pos(aINS_INSECT_ACTOR* insect, GAME* game) {
    ACTOR* actorx = &insect->tools_actor.actor_class;
    f32 tree_y;

    /* Position crab at the top of the palm tree */
    tree_y = mCoBG_GetBgY_OnlyCenter_FromWpos2(actorx->world.position, 0.0f);
    actorx->world.position.y = tree_y + 80.0f;

    /* Set random facing angle */
    actorx->world.angle.y = (s16)(RANDOM_F(DEG2SHORT_ANGLE2(360.0f)));
    actorx->shape_info.rotation.y = actorx->world.angle.y;

    /* Store the block center for active range checks */
    mFI_BkNum2WposXZ(&aIKN_BK_CENTER_X(insect), &aIKN_BK_CENTER_Z(insect),
                     actorx->block_x, actorx->block_z);
    aIKN_BK_CENTER_X(insect) += mFI_BK_WORLDSIZE_HALF_X_F;
    aIKN_BK_CENTER_Z(insect) += mFI_BK_WORLDSIZE_HALF_Z_F;
}

/* ========================================================================
 * State handler functions (action procs)
 * ======================================================================== */

/**
 * Avoid state — crab scuttles away from the player.
 * Signature: aIKN_avoid(actor_s*, game_s*) — 412 bytes
 */
static void aIKN_avoid(ACTOR* actorx, GAME* game) {
    aINS_INSECT_ACTOR* insect = (aINS_INSECT_ACTOR*)actorx;
    int dug_attr;

    aIKN_anime_proc(insect);

    /* Check for water ahead */
    if (actorx->bg_collision_check.result.on_ground) {
        xyz_t pos = actorx->world.position;
        f32 range = insect->bg_range + actorx->speed;
        s16 yAngle = actorx->world.angle.y;

        pos.x += range * sin_s(yAngle);
        pos.z += range * cos_s(yAngle);

        if (mCoBG_CheckWaterAttribute(mCoBG_Wpos2BgAttribute_Original(pos))) {
            aIKN_setupAction(insect, aIKN_ACTION_DIVE, game);
            return;
        }
    }

    /* Check active range */
    if (aIKN_chk_active_range(insect) == FALSE) {
        aIKN_setupAction(insect, aIKN_ACTION_ESCAPE_CONT, game);
        return;
    }

    /* Check dug ground attribute */
    dug_attr = aIKN_chk_dug_attr(insect);
    if (dug_attr == 1) {
        aIKN_setupAction(insect, aIKN_ACTION_DUG, game);
        return;
    } else if (dug_attr == 2) {
        aIKN_setupAction(insect, aIKN_ACTION_DIVE, game);
        return;
    }

    /* Adjust speed periodically */
    aIKN_TIMER0(insect)--;
    if (aIKN_TIMER0(insect) <= 0) {
        insect->target_speed = (1.1f - RANDOM_F(0.2f)) * 2.0f;
        aIKN_TIMER0(insect) = 10;
    }

    /* If patience has dropped, retreat into shell */
    if (insect->patience < 50.0f) {
        aIKN_setupAction(insect, aIKN_ACTION_INTO_SHELL, game);
    } else {
        aIKN_calc_direction_angl(insect);
        sAdo_OngenPos((u32)actorx, 0x63, &actorx->world.position);
    }
}

/**
 * Hide (kani) state — crab is hidden underground, waiting to be dug up.
 * Signature: aIKN_hide_kani(actor_s*, game_s*) — 32 bytes
 */
static void aIKN_hide_kani(ACTOR* actorx, GAME* game) {
    if (aIKN_chk_player_scoop(actorx) == TRUE) {
        aIKN_setupAction((aINS_INSECT_ACTOR*)actorx, aIKN_ACTION_APPEAR_KANI, game);
    }
}

/**
 * Wait state — crab sits still, watching for threats.
 * Signature: aIKN_wait(actor_s*, game_s*) — 260 bytes
 */
static void aIKN_wait(ACTOR* actorx, GAME* game) {
    aINS_INSECT_ACTOR* insect = (aINS_INSECT_ACTOR*)actorx;

    /* Subtle sway animation */
    {
        s16 sway_angle;
        aIKN_TIMER1(insect) += 0x200;
        sway_angle = cos_s((s16)aIKN_TIMER1(insect));
        actorx->scale.z = 0.01f + (sway_angle * 0.001f);
    }

    if (aIKN_chk_patience(insect, game) == TRUE) {
        aIKN_set_avoid_player_angl(insect, game, 0);
        aIKN_setupAction(insect, aIKN_ACTION_AVOID, game);
    } else {
        insect->timer--;
        if (insect->timer <= 0) {
            if (RANDOM_F(1.0f) > 0.6f) {
                aIKN_setupAction(insect, aIKN_ACTION_MOVE, game);
            } else {
                insect->timer = 60 + (int)RANDOM_F(120.0f);
            }
        }
    }
}

/**
 * Move state — crab scuttles sideways.
 * Signature: aIKN_move(actor_s*, game_s*) — 192 bytes
 */
static void aIKN_move(ACTOR* actorx, GAME* game) {
    aINS_INSECT_ACTOR* insect = (aINS_INSECT_ACTOR*)actorx;

    aIKN_anime_proc(insect);
    sAdo_OngenPos((u32)actorx, 0x63, &actorx->world.position);

    if (aIKN_chk_patience(insect, game) == TRUE) {
        aIKN_set_avoid_player_angl(insect, game, 0);
        aIKN_setupAction(insect, aIKN_ACTION_AVOID, game);
    } else {
        if (aIKN_chk_active_range(insect) == FALSE) {
            /* Turn around toward center */
            f32 dx = aIKN_BK_CENTER_X(insect) - actorx->world.position.x;
            f32 dz = aIKN_BK_CENTER_Z(insect) - actorx->world.position.z;
            actorx->world.angle.y = atans_table(dx, dz);
        }

        aIKN_calc_direction_angl(insect);

        insect->timer--;
        if (insect->timer <= 0) {
            aIKN_setupAction(insect, aIKN_ACTION_WAIT, game);
        }
    }
}

/**
 * Dug state — crab has been dug out of the ground.
 * Signature: aIKN_dug(actor_s*, game_s*) — 308 bytes
 */
static void aIKN_dug(ACTOR* actorx, GAME* game) {
    aINS_INSECT_ACTOR* insect = (aINS_INSECT_ACTOR*)actorx;

    /* Struggle animation */
    {
        s16 sway_angle;
        aIKN_TIMER1(insect) += 0x800;
        sway_angle = cos_s((s16)aIKN_TIMER1(insect));
        actorx->scale.z = 0.01f + (sway_angle * 0.002f);
    }

    aIKN_SHELL_TIMER(insect) -= 0.5f;
    if (aIKN_SHELL_TIMER(insect) <= 0.0f) {
        int type = (int)(3.0f + RANDOM_F(3.0f));
        eEC_CLIP->effect_make_proc(eEC_EFFECT_DIG_MUD, actorx->world.position, 2,
                                   actorx->shape_info.rotation.y, game, RSV_NO, 0, 0x4000 | type);
        aIKN_SHELL_TIMER(insect) = 6.0f;
    }

    sAdo_OngenPos((u32)actorx, 0x63, &actorx->world.position);

    insect->timer--;
    if (insect->timer <= 0) {
        if (actorx->bg_collision_check.result.on_ground) {
            aIKN_set_avoid_player_angl(insect, game, 0);
            aIKN_setupAction(insect, aIKN_ACTION_AVOID, game);
        }
    }
}

/**
 * Appear (kani) state — crab emerges from the ground after being dug up.
 * Signature: aIKN_appear_kani(actor_s*, game_s*) — 96 bytes
 */
static void aIKN_appear_kani(ACTOR* actorx, GAME* game) {
    aINS_INSECT_ACTOR* insect = (aINS_INSECT_ACTOR*)actorx;

    if (actorx->bg_collision_check.result.on_ground) {
        aIKN_set_avoid_player_angl(insect, game, 0);
        aIKN_setupAction(insect, aIKN_ACTION_AVOID, game);
    }
}

/**
 * Appear from shell state — crab peeks out of its shell.
 * Signature: aIKN_appear_from_shell(actor_s*, game_s*) — 128 bytes
 */
static void aIKN_appear_from_shell(ACTOR* actorx, GAME* game) {
    aINS_INSECT_ACTOR* insect = (aINS_INSECT_ACTOR*)actorx;

    insect->timer--;
    if (insect->timer <= 0) {
        /* Check if player is still nearby */
        if (aIKN_chk_patience(insect, game) == TRUE) {
            /* Retreat back into shell */
            aIKN_setupAction(insect, aIKN_ACTION_INTO_SHELL, game);
        } else {
            /* Safe to come out */
            aIKN_setupAction(insect, aIKN_ACTION_WAIT, game);
        }
    }
}

/**
 * Into shell state — crab retreats into its shell.
 * Signature: aIKN_into_shell(actor_s*, game_s*) — 112 bytes
 */
static void aIKN_into_shell(ACTOR* actorx, GAME* game) {
    aINS_INSECT_ACTOR* insect = (aINS_INSECT_ACTOR*)actorx;

    insect->timer--;
    if (insect->timer <= 0) {
        /* Check if player has left */
        if (insect->patience < 30.0f) {
            aIKN_setupAction(insect, aIKN_ACTION_APPEAR_FROM_SHELL, game);
        } else {
            aIKN_setupAction(insect, aIKN_ACTION_CAMOUFLAGE, game);
        }
    }
}

/**
 * Turn state — crab turns to face a new direction.
 * Signature: aIKN_turn(actor_s*, game_s*) — 116 bytes
 */
static void aIKN_turn(ACTOR* actorx, GAME* game) {
    aINS_INSECT_ACTOR* insect = (aINS_INSECT_ACTOR*)actorx;
    int done;

    done = chase_angle(&actorx->shape_info.rotation.y,
                        actorx->world.angle.y, 0x800);
    if (done == 1) {
        aIKN_setupAction(insect, aIKN_ACTION_MOVE, game);
    }

    if (aIKN_chk_patience(insect, game) == TRUE) {
        aIKN_set_avoid_player_angl(insect, game, 0);
        aIKN_setupAction(insect, aIKN_ACTION_AVOID, game);
    }
}

/**
 * Dive state — crab jumps into water.
 * Signature: aIKN_dive(actor_s*, game_s*) — 304 bytes
 */
static void aIKN_dive(ACTOR* actorx, GAME* game) {
    aINS_INSECT_ACTOR* insect = (aINS_INSECT_ACTOR*)actorx;
    f32 water_y;

    aIKN_anime_proc(insect);
    aIKN_calc_direction_angl(insect);

    water_y = mCoBG_GetWaterHeight_File(actorx->world.position, "ac_ins_kani.c", 540);

    if (actorx->world.position.y <= water_y) {
        aIKN_set_water_effect(actorx, game);
        aIKN_setupAction(insect, aIKN_ACTION_DROWN, game);
    } else {
        /* Gradually descend */
        actorx->shape_info.rotation.x = atans_table(actorx->speed, -actorx->position_speed.y);
        sAdo_OngenPos((u32)actorx, 0x63, &actorx->world.position);
    }
}

/* ========================================================================
 * Palm tree (yasi) state handlers
 * ======================================================================== */

/**
 * Hide on palm tree state — crab is hidden on a palm tree.
 * Signature: aIKN_hide_yasi(actor_s*, game_s*) — 108 bytes
 */
static void aIKN_hide_yasi(ACTOR* actorx, GAME* game) {
    aINS_INSECT_ACTOR* insect = (aINS_INSECT_ACTOR*)actorx;

    /* Check if tree was shaken or cut */
    if (aIKN_chk_cut_tree(insect) == TRUE) {
        aIKN_setupAction(insect, aIKN_ACTION_FALL_YASI, game);
    } else if (aIKN_chk_shake_tree(insect) == TRUE) {
        aIKN_setupAction(insect, aIKN_ACTION_FALL_YASI, game);
    }
}

/**
 * Fall from palm tree state — crab is falling.
 * Signature: aIKN_fall_yasi(actor_s*, game_s*) — 52 bytes
 */
static void aIKN_fall_yasi(ACTOR* actorx, GAME* game) {
    aINS_INSECT_ACTOR* insect = (aINS_INSECT_ACTOR*)actorx;

    if (actorx->bg_collision_check.result.on_ground) {
        aIKN_setupAction(insect, aIKN_ACTION_BOUND_YASI, game);
    }
}

/**
 * Bounce on ground after falling from palm tree.
 * Signature: aIKN_bound_yasi(actor_s*, game_s*) — 52 bytes
 */
static void aIKN_bound_yasi(ACTOR* actorx, GAME* game) {
    aINS_INSECT_ACTOR* insect = (aINS_INSECT_ACTOR*)actorx;

    if (actorx->bg_collision_check.result.on_ground) {
        aIKN_setupAction(insect, aIKN_ACTION_STRUGGLE_YASI, game);
    }
}

/**
 * Struggle on ground after falling from palm tree.
 * Signature: aIKN_struggle_yasi(actor_s*, game_s*) — 116 bytes
 */
static void aIKN_struggle_yasi(ACTOR* actorx, GAME* game) {
    aINS_INSECT_ACTOR* insect = (aINS_INSECT_ACTOR*)actorx;

    /* Wiggle animation */
    {
        s16 sway_angle;
        aIKN_TIMER1(insect) += 0xC00;
        sway_angle = sin_s((s16)aIKN_TIMER1(insect));
        actorx->shape_info.rotation.z = (s16)(sway_angle >> 2);
    }

    insect->timer--;
    if (insect->timer <= 0) {
        aIKN_setupAction(insect, aIKN_ACTION_ROLL_YASI, game);
    }
}

/**
 * Roll upright after struggling on ground from palm tree fall.
 * Signature: aIKN_roll_yasi(actor_s*, game_s*) — 136 bytes
 */
static void aIKN_roll_yasi(ACTOR* actorx, GAME* game) {
    aINS_INSECT_ACTOR* insect = (aINS_INSECT_ACTOR*)actorx;
    int done;

    /* Roll upright */
    done = chase_angle(&actorx->shape_info.rotation.x, 0, 0x400);
    chase_angle(&actorx->shape_info.rotation.z, 0, 0x400);

    if (done == 1) {
        /* Now upright — start normal behavior */
        actorx->shape_info.draw_shadow = TRUE;
        if (aIKN_chk_patience(insect, game) == TRUE) {
            aIKN_set_avoid_player_angl(insect, game, 0);
            aIKN_setupAction(insect, aIKN_ACTION_AVOID, game);
        } else {
            aIKN_setupAction(insect, aIKN_ACTION_WAIT, game);
        }
    }
}

/* ========================================================================
 * Init functions for each state
 * ======================================================================== */

/**
 * Signature: aIKN_avoid_init(insect_actor_s*, game_s*) — 140 bytes
 */
static void aIKN_avoid_init(aINS_INSECT_ACTOR* insect, GAME* game) {
    insect->target_speed = 2.0f;
    insect->speed_step = 0.3f;
    insect->tools_actor.actor_class.gravity = 1.0f;
    insect->tools_actor.actor_class.max_velocity_y = -20.0f;
    insect->bg_type = aINS_BG_CHECK_TYPE_REG_NO_ATTR;
    insect->tools_actor.actor_class.shape_info.rotation.x = 0;
    insect->tools_actor.actor_class.shape_info.draw_shadow = TRUE;
    aIKN_TIMER0(insect) = 10;
}

/**
 * Signature: aIKN_let_escape_init(insect_actor_s*, game_s*) — 292 bytes
 */
static void aIKN_let_escape_init(aINS_INSECT_ACTOR* insect, GAME* game) {
    insect->life_time = 0;
    insect->alpha_time = 80;
    insect->tools_actor.actor_class.gravity = 1.0f;
    insect->tools_actor.actor_class.max_velocity_y = -20.0f;
    insect->tools_actor.actor_class.speed = 2.5f;
    insect->tools_actor.actor_class.shape_info.rotation.x = 0;
    insect->bg_type = aINS_BG_CHECK_TYPE_REG_NO_ATTR;

    aIKN_set_avoid_player_angl(insect, game, 1);

    insect->target_speed = 2.5f;
    insect->speed_step = 0.3f;
    insect->insect_flags.bit_1 = TRUE;
    insect->insect_flags.bit_2 = TRUE;
    insect->tools_actor.actor_class.shape_info.draw_shadow = TRUE;
    insect->tools_actor.actor_class.drawn = TRUE;
}

/**
 * Signature: aIKN_hide_kani_init(insect_actor_s*, game_s*) — 88 bytes
 */
static void aIKN_hide_kani_init(aINS_INSECT_ACTOR* insect, GAME* game) {
    insect->bg_type = aINS_BG_CHECK_TYPE_NONE;
    insect->tools_actor.actor_class.drawn = FALSE;
    insect->tools_actor.actor_class.shape_info.draw_shadow = FALSE;
    insect->tools_actor.actor_class.gravity = 0.0f;
    insect->tools_actor.actor_class.speed = 0.0f;
    insect->target_speed = 0.0f;
}

/**
 * Signature: aIKN_wait_init(insect_actor_s*, game_s*) — 108 bytes
 */
static void aIKN_wait_init(aINS_INSECT_ACTOR* insect, GAME* game) {
    insect->target_speed = 0.0f;
    insect->speed_step = 0.05f;
    insect->_1E0 = 0.0f;
    insect->tools_actor.actor_class.speed = 0.0f;
    insect->timer = 60 + (int)RANDOM_F(180.0f);
    aIKN_TIMER1(insect) = 0;
    insect->tools_actor.actor_class.shape_info.draw_shadow = TRUE;
    insect->tools_actor.actor_class.drawn = TRUE;
}

/**
 * Signature: aIKN_move_init(insect_actor_s*, game_s*) — 256 bytes
 */
static void aIKN_move_init(aINS_INSECT_ACTOR* insect, GAME* game) {
    ACTOR* actorx = &insect->tools_actor.actor_class;

    insect->target_speed = 1.5f;
    insect->speed_step = 0.3f;
    insect->tools_actor.actor_class.gravity = 1.0f;
    insect->tools_actor.actor_class.max_velocity_y = -20.0f;
    insect->bg_type = aINS_BG_CHECK_TYPE_REG_NO_ATTR;

    /* Pick a random direction */
    actorx->world.angle.y += (s16)RANDOM_CENTER_F(DEG2SHORT_ANGLE2(90.0f));

    /* Check that the chosen direction doesn't lead to water */
    {
        xyz_t pos = actorx->world.position;
        f32 range = 40.0f;
        s16 yAngle = actorx->world.angle.y;

        pos.x += range * sin_s(yAngle);
        pos.z += range * cos_s(yAngle);

        if (mCoBG_CheckWaterAttribute(mCoBG_Wpos2BgAttribute_Original(pos))) {
            actorx->world.angle.y += DEG2SHORT_ANGLE2(180.0f);
        }
    }

    insect->timer = 40 + (int)RANDOM_F(60.0f);
}

/**
 * Signature: aIKN_dug_init(insect_actor_s*, game_s*) — 116 bytes
 */
static void aIKN_dug_init(aINS_INSECT_ACTOR* insect, GAME* game) {
    ACTOR* actorx = &insect->tools_actor.actor_class;

    aIKN_set_mud_effect(actorx, game);
    sAdo_OngenPos((u32)actorx, 0x477, &actorx->world.position);
    insect->timer = (int)(RANDOM_F(60.0f) + 30.0f);
    insect->tools_actor.actor_class.speed = 0.0f;
    insect->target_speed = 0.0f;
    aIKN_SHELL_TIMER(insect) = 6.0f;
    aIKN_TIMER1(insect) = 0;
}

/**
 * Signature: aIKN_appear_kani_init(insect_actor_s*, game_s*) — 168 bytes
 */
static void aIKN_appear_kani_init(aINS_INSECT_ACTOR* insect, GAME* game) {
    ACTOR* actorx = &insect->tools_actor.actor_class;

    aIKN_set_mud_effect(actorx, game);

    insect->target_speed = 1.5f;
    insect->speed_step = 0.3f;
    insect->tools_actor.actor_class.speed = 1.5f;
    insect->tools_actor.actor_class.gravity = 1.0f;
    insect->tools_actor.actor_class.max_velocity_y = -20.0f;
    insect->tools_actor.actor_class.position_speed.y = 5.0f;
    insect->tools_actor.actor_class.drawn = TRUE;
    insect->tools_actor.actor_class.shape_info.draw_shadow = TRUE;
    insect->bg_type = aINS_BG_CHECK_TYPE_NO_UNIT_COLUMN_NO_ATTR;

    aIKN_set_avoid_player_angl(insect, game, 0);
}

/**
 * Signature: aIKN_appear_from_shell_init(insect_actor_s*, game_s*) — 12 bytes
 */
static void aIKN_appear_from_shell_init(aINS_INSECT_ACTOR* insect, GAME* game) {
    insect->timer = 30 + (int)RANDOM_F(30.0f);
}

/**
 * Signature: aIKN_into_shell_init(insect_actor_s*, game_s*) — 32 bytes
 */
static void aIKN_into_shell_init(aINS_INSECT_ACTOR* insect, GAME* game) {
    insect->tools_actor.actor_class.speed = 0.0f;
    insect->target_speed = 0.0f;
    insect->timer = 30 + (int)RANDOM_F(60.0f);
}

/**
 * Signature: aIKN_camouflage_init(insect_actor_s*, game_s*) — 92 bytes
 */
static void aIKN_camouflage_init(aINS_INSECT_ACTOR* insect, GAME* game) {
    insect->tools_actor.actor_class.speed = 0.0f;
    insect->target_speed = 0.0f;
    insect->_1E0 = 0.0f;
    insect->tools_actor.actor_class.shape_info.draw_shadow = FALSE;
    insect->timer = 120 + (int)RANDOM_F(180.0f);
}

/**
 * Signature: aIKN_drown_init(insect_actor_s*, game_s*) — 80 bytes
 */
static void aIKN_drown_init(aINS_INSECT_ACTOR* insect, GAME* game) {
    insect->insect_flags.bit_1 = TRUE;
    insect->insect_flags.destruct = TRUE;
    insect->tools_actor.actor_class.shape_info.draw_shadow = FALSE;
}

/**
 * Signature: aIKN_escape_cont_init(insect_actor_s*, game_s*) — 196 bytes
 */
static void aIKN_escape_cont_init(aINS_INSECT_ACTOR* insect, GAME* game) {
    insect->life_time = 0;
    insect->alpha_time = 80;
    insect->tools_actor.actor_class.shape_info.rotation.x = 0;
    insect->bg_type = aINS_BG_CHECK_TYPE_REG_NO_ATTR;
    insect->tools_actor.actor_class.gravity = 1.0f;
    insect->tools_actor.actor_class.max_velocity_y = -20.0f;

    aIKN_set_avoid_player_angl(insect, game, 1);

    insect->target_speed = 2.5f;
    insect->speed_step = 0.3f;
    insect->insect_flags.bit_1 = TRUE;
    insect->insect_flags.bit_2 = TRUE;
}

/**
 * Signature: aIKN_hide_yasi_init(insect_actor_s*, game_s*) — 80 bytes
 */
static void aIKN_hide_yasi_init(aINS_INSECT_ACTOR* insect, GAME* game) {
    insect->bg_type = aINS_BG_CHECK_TYPE_NONE;
    insect->tools_actor.actor_class.drawn = FALSE;
    insect->tools_actor.actor_class.shape_info.draw_shadow = FALSE;
    insect->tools_actor.actor_class.gravity = 0.0f;
    insect->tools_actor.actor_class.speed = 0.0f;
    insect->target_speed = 0.0f;
}

/**
 * Signature: aIKN_fall_yasi_init(insect_actor_s*, game_s*) — 192 bytes
 */
static void aIKN_fall_yasi_init(aINS_INSECT_ACTOR* insect, GAME* game) {
    ACTOR* actorx = &insect->tools_actor.actor_class;

    actorx->drawn = TRUE;
    actorx->shape_info.draw_shadow = FALSE;
    insect->bg_type = aINS_BG_CHECK_TYPE_REG_NO_ATTR;
    actorx->gravity = 2.0f;
    actorx->max_velocity_y = -20.0f;
    actorx->position_speed.y = 3.0f;
    actorx->speed = 1.0f;
    insect->target_speed = 1.0f;
    insect->speed_step = 0.0f;

    /* Random tumble rotation */
    actorx->shape_info.rotation.x = (s16)(RANDOM_F(DEG2SHORT_ANGLE2(360.0f)));
    actorx->shape_info.rotation.z = (s16)(RANDOM_F(DEG2SHORT_ANGLE2(360.0f)));
    actorx->world.angle.y = (s16)(RANDOM_F(DEG2SHORT_ANGLE2(360.0f)));
}

/**
 * Signature: aIKN_bound_yasi_init(insect_actor_s*, game_s*) — 68 bytes
 */
static void aIKN_bound_yasi_init(aINS_INSECT_ACTOR* insect, GAME* game) {
    ACTOR* actorx = &insect->tools_actor.actor_class;

    actorx->position_speed.y = 3.0f;
    actorx->gravity = 2.0f;
    actorx->speed = 0.5f;
    insect->target_speed = 0.5f;
    aIKN_YASI_BOUNCE(insect)++;
}

/**
 * Signature: aIKN_struggle_yasi_init(insect_actor_s*, game_s*) — 12 bytes
 */
static void aIKN_struggle_yasi_init(aINS_INSECT_ACTOR* insect, GAME* game) {
    insect->timer = 30 + (int)RANDOM_F(30.0f);
}

/**
 * Signature: aIKN_roll_yasi_init(insect_actor_s*, game_s*) — 44 bytes
 */
static void aIKN_roll_yasi_init(aINS_INSECT_ACTOR* insect, GAME* game) {
    insect->tools_actor.actor_class.speed = 0.0f;
    insect->target_speed = 0.0f;
    insect->speed_step = 0.0f;
}

/* ========================================================================
 * Core: setupAction and actor_move
 * ======================================================================== */

typedef void (*aIKN_INIT_PROC)(aINS_INSECT_ACTOR*, GAME*);

/**
 * Configure the crab's current action and call the associated init function.
 * Signature: aIKN_setupAction(insect_actor_s*, int, game_s*) — 92 bytes
 */
static void aIKN_setupAction(aINS_INSECT_ACTOR* insect, int action, GAME* game) {
    static aIKN_INIT_PROC init_proc[] = {
        aIKN_avoid_init,              /* AVOID */
        aIKN_let_escape_init,         /* LET_ESCAPE */
        aIKN_hide_kani_init,          /* HIDE_KANI */
        aIKN_wait_init,               /* WAIT */
        aIKN_move_init,               /* MOVE */
        aIKN_dug_init,                /* DUG */
        aIKN_appear_kani_init,        /* APPEAR_KANI */
        aIKN_appear_from_shell_init,  /* APPEAR_FROM_SHELL */
        aIKN_into_shell_init,         /* INTO_SHELL */
        aIKN_camouflage_init,         /* CAMOUFLAGE */
        (aIKN_INIT_PROC)none_proc1,   /* TURN */
        (aIKN_INIT_PROC)none_proc1,   /* DIVE */
        aIKN_drown_init,              /* DROWN */
        aIKN_escape_cont_init,        /* ESCAPE_CONT */
        aIKN_hide_yasi_init,          /* HIDE_YASI */
        aIKN_fall_yasi_init,          /* FALL_YASI */
        aIKN_bound_yasi_init,         /* BOUND_YASI */
        aIKN_struggle_yasi_init,      /* STRUGGLE_YASI */
        aIKN_roll_yasi_init,          /* ROLL_YASI */
    };

    static aINS_ACTION_PROC act_proc[] = {
        aIKN_avoid,              /* AVOID */
        (aINS_ACTION_PROC)none_proc1, /* LET_ESCAPE — just fades out */
        aIKN_hide_kani,          /* HIDE_KANI */
        aIKN_wait,               /* WAIT */
        aIKN_move,               /* MOVE */
        aIKN_dug,                /* DUG */
        aIKN_appear_kani,        /* APPEAR_KANI */
        aIKN_appear_from_shell,  /* APPEAR_FROM_SHELL */
        aIKN_into_shell,         /* INTO_SHELL */
        (aINS_ACTION_PROC)none_proc1, /* CAMOUFLAGE — static, just sits */
        aIKN_turn,               /* TURN */
        aIKN_dive,               /* DIVE */
        (aINS_ACTION_PROC)none_proc1, /* DROWN — instant destruct */
        (aINS_ACTION_PROC)none_proc1, /* ESCAPE_CONT — just fades out */
        aIKN_hide_yasi,          /* HIDE_YASI */
        aIKN_fall_yasi,          /* FALL_YASI */
        aIKN_bound_yasi,         /* BOUND_YASI */
        aIKN_struggle_yasi,      /* STRUGGLE_YASI */
        aIKN_roll_yasi,          /* ROLL_YASI */
    };

    insect->action = action;
    insect->action_proc = act_proc[action];
    (*init_proc[action])(insect, game);
}

/**
 * Main tick function dispatched every frame.
 * Signature: aIKN_actor_move(actor_s*, game_s*) — 164 bytes
 */
static void aIKN_actor_move(ACTOR* actorx, GAME* game) {
    aINS_INSECT_ACTOR* insect = (aINS_INSECT_ACTOR*)actorx;
    u32 catch_label;

    catch_label = mPlib_Get_item_net_catch_label();
    if (catch_label == (u32)actorx) {
        insect->alpha0 = 255;
        aIKN_setupAction(insect, aIKN_ACTION_LET_ESCAPE, game);
    } else if (insect->insect_flags.bit_3 == TRUE && insect->insect_flags.bit_2 == FALSE) {
        aIKN_setupAction(insect, aIKN_ACTION_LET_ESCAPE, game);
    } else {
        insect->action_proc(actorx, game);
    }
}

/* ========================================================================
 * Actor init — entry point
 * ======================================================================== */

/**
 * Initialize the hermit crab insect actor.
 * Signature: aIKN_actor_init — 428 bytes
 */
extern void aIKN_actor_init(ACTOR* actorx, GAME* game) {
    aINS_INSECT_ACTOR* insect = (aINS_INSECT_ACTOR*)actorx;
    int action;
    mActor_name_t* fg_p;

    insect->bg_range = 5.0f;
    insect->item = ITM_INSECT32; /* Hermit crab item */
    insect->insect_flags.bit_4 = FALSE;
    actorx->mv_proc = aIKN_actor_move;

    if (actorx->actor_specific == aINS_INIT_NORMAL) {
        int ux, uz;

        /* Store block center for active range checks */
        mFI_BkNum2WposXZ(&aIKN_BK_CENTER_X(insect), &aIKN_BK_CENTER_Z(insect),
                         actorx->block_x, actorx->block_z);
        aIKN_BK_CENTER_X(insect) += mFI_BK_WORLDSIZE_HALF_X_F;
        aIKN_BK_CENTER_Z(insect) += mFI_BK_WORLDSIZE_HALF_Z_F;

        mFI_Wpos2UtNum(&ux, &uz, actorx->world.position);
        insect->ut_x = ux;
        insect->ut_z = uz;

        /* Determine spawn type based on foreground item */
        fg_p = mFI_GetUnitFG(actorx->world.position);
        if (fg_p != NULL && IS_ITEM_ANY_PALM_TREE(*fg_p)) {
            /* Spawn on palm tree */
            aIKN_set_yasi_start_pos(insect, game);
            action = aIKN_ACTION_HIDE_YASI;
        } else if (mCoBG_CheckHole(actorx->world.position) == TRUE) {
            /* Spawn in a dug hole */
            action = aIKN_ACTION_HIDE_KANI;
        } else {
            /* Spawn on beach ground */
            actorx->world.position.y = mCoBG_GetBgY_OnlyCenter_FromWpos2(actorx->world.position, 0.0f);
            action = aIKN_ACTION_WAIT;
        }
    } else {
        /* Released from inventory */
        actorx->drawn = TRUE;
        aIKN_set_avoid_player_angl(insect, game, 1);
        action = aIKN_ACTION_LET_ESCAPE;
    }

    aIKN_YASI_BOUNCE(insect) = 0;
    aIKN_setupAction(insect, action, game);
}

#endif /* VERSION >= VER_DELUXE */
