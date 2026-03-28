#include "types.h"
#if VERSION >= VER_DELUXE
#include "ac_ins_nomi.h"

#include "m_name_table.h"
#include "m_common_data.h"
#include "m_player_lib.h"
#include "ac_set_ovl_insect.h"

enum {
    aINM_ACTION_WAIT,
    aINM_ACTION_LET_ESCAPE,

    aINM_ACTION_NUM
};

static void aINM_actor_move(ACTOR* actorx, GAME* game);
static void aINM_setupAction(aINS_INSECT_ACTOR* insect, int action, GAME* game);

extern void aINM_actor_init(ACTOR* actorx, GAME* game) {
    aINS_INSECT_ACTOR* insect = (aINS_INSECT_ACTOR*)actorx;
    int action;

    insect->bg_range = 2.0f;
    insect->item = ITM_INSECT38;
    insect->insect_flags.bit_4 = FALSE;
    actorx->mv_proc = aINM_actor_move;

    if (actorx->actor_specific == aINS_INIT_NORMAL) {
        actorx->shape_info.draw_shadow = FALSE;
        insect->tools_actor.actor_class.drawn = TRUE;
        action = aINM_ACTION_WAIT;
    } else {
        actorx->drawn = TRUE;
        action = aINM_ACTION_LET_ESCAPE;
    }

    aINM_setupAction(insect, action, game);
}

static void aINM_set_escape_angle(ACTOR* actorx, GAME* game) {
    ACTOR* playerx;
    f32 dx;
    f32 dz;
    s16 angle;

    playerx = GET_PLAYER_ACTOR_GAME_ACTOR(game);
    if (playerx != NULL) {
        dx = actorx->world.position.x - playerx->world.position.x;
        dz = actorx->world.position.z - playerx->world.position.z;
        angle = atans_table(dx, dz);
        angle += (s16)RANDOM_CENTER_F(DEG2SHORT_ANGLE2(60.0f));
        actorx->world.angle.y = angle;
        actorx->shape_info.rotation.y = angle;
    }
}

static void aINM_set_effect(aINS_INSECT_ACTOR* insect, GAME* game) {
    xyz_t pos;
    ACTOR* actorx;
    f32 dx;
    f32 dz;
    f32 distSq;
    ACTOR* playerx;

    actorx = (ACTOR*)insect;
    playerx = GET_PLAYER_ACTOR_GAME_ACTOR(game);
    if (playerx != NULL) {
        dx = actorx->world.position.x - playerx->world.position.x;
        dz = actorx->world.position.z - playerx->world.position.z;
        distSq = (dx * dx) + (dz * dz);
        if (distSq < 2500.0f) {
            xyz_t_move(&pos, &actorx->world.position);
            pos.y += 10.0f;
            eEC_CLIP->effect_make_proc(eEC_EFFECT_KANTANHU, pos, 1,
                                       actorx->shape_info.rotation.y, game, EMPTY_NO, 0, 0);
        }
    }
}

static void aINM_let_escape(ACTOR* actorx, GAME* game) {
    aINS_INSECT_ACTOR* insect = (aINS_INSECT_ACTOR*)actorx;
    f32 grav;

    insect->_1E0 += 0.5f;
    if (insect->_1E0 >= 2.0f) {
        insect->_1E0 -= 2.0f;
    }

    grav = actorx->gravity;
    grav *= 1.1f;
    if (grav > 12.0f) {
        grav = 12.0f;
    }
    actorx->gravity = grav;
}

static void aINM_wait(ACTOR* actorx, GAME* game) {
    aINS_INSECT_ACTOR* insect = (aINS_INSECT_ACTOR*)actorx;
    f32 dx;
    f32 dz;
    f32 distSq;
    ACTOR* playerx;

    insect->_1E0 += 0.5f;
    if (insect->_1E0 >= 2.0f) {
        insect->_1E0 -= 2.0f;
    }

    playerx = GET_PLAYER_ACTOR_GAME_ACTOR(game);
    if (playerx != NULL) {
        dx = actorx->world.position.x - playerx->world.position.x;
        dz = actorx->world.position.z - playerx->world.position.z;
        distSq = (dx * dx) + (dz * dz);

        if (distSq < 900.0f) {
            insect->patience += 5.0f;
            if (insect->patience > 100.0f) {
                insect->patience = 100.0f;
            }
        } else {
            insect->patience -= 0.5f;
            if (insect->patience < 0.0f) {
                insect->patience = 0.0f;
            }
        }
    }

    if (insect->patience > 90.0f) {
        aINM_set_effect(insect, game);
        aINM_setupAction(insect, aINM_ACTION_LET_ESCAPE, game);
    } else {
        insect->timer--;
        if (insect->timer <= 0) {
            insect->timer = 30 + (int)RANDOM_F(30.0f);
            actorx->world.position.x += RANDOM_CENTER_F(2.0f);
            actorx->world.position.z += RANDOM_CENTER_F(2.0f);
        }
    }
}

static void aINM_let_escape_init(aINS_INSECT_ACTOR* insect, GAME* game) {
    ACTOR* playerx;
    s16 angle;

    insect->life_time = 0;
    insect->alpha_time = 80;
    insect->tools_actor.actor_class.gravity = 0.06f;
    insect->tools_actor.actor_class.max_velocity_y = 12.0f;
    insect->tools_actor.actor_class.speed = 3.0f;
    insect->tools_actor.actor_class.shape_info.rotation.x = 0;
    insect->bg_type = aINS_BG_CHECK_TYPE_REG_NO_ATTR;
    insect->tools_actor.actor_class.shape_info.draw_shadow = TRUE;

    playerx = GET_PLAYER_ACTOR_GAME_ACTOR(game);
    if (playerx != NULL) {
        angle = playerx->shape_info.rotation.y + (s16)RANDOM_CENTER_F(DEG2SHORT_ANGLE2(120.0f));
        insect->tools_actor.actor_class.world.angle.y = angle;
        insect->tools_actor.actor_class.shape_info.rotation.y = angle;
    }

    aINM_set_escape_angle(&insect->tools_actor.actor_class, game);
    insect->insect_flags.bit_1 = TRUE;
    insect->insect_flags.bit_2 = TRUE;
}

typedef void (*aINM_INIT_PROC)(aINS_INSECT_ACTOR* insect, GAME* game);

static void aINM_setupAction(aINS_INSECT_ACTOR* insect, int action, GAME* game) {
    static aINM_INIT_PROC init_proc[] = {
        (aINM_INIT_PROC)none_proc1,
        aINM_let_escape_init,
    };

    static aINS_ACTION_PROC act_proc[] = {
        aINM_wait,
        aINM_let_escape,
    };

    insect->action = action;
    insect->action_proc = act_proc[action];
    (*init_proc[action])(insect, game);
}

static void aINM_actor_move(ACTOR* actorx, GAME* game) {
    aINS_INSECT_ACTOR* insect = (aINS_INSECT_ACTOR*)actorx;
    u32 catch_label;

    catch_label = mPlib_Get_item_net_catch_label();
    if (catch_label == (u32)actorx) {
        insect->alpha0 = 255;
        aINM_setupAction(insect, aINM_ACTION_LET_ESCAPE, game);
    } else if (insect->insect_flags.bit_3 == TRUE && insect->insect_flags.bit_2 == FALSE) {
        aINM_setupAction(insect, aINM_ACTION_LET_ESCAPE, game);
    } else {
        insect->action_proc(actorx, game);
    }
}
#endif /* VERSION >= VER_DELUXE */
