#include "ac_reset_chair.h"

#include "m_common_data.h"
#include "m_rcp.h"
#include "sys_matrix.h"
#include "m_player_lib.h"

static void Reset_Chair_Actor_ct(ACTOR* actorx, GAME* game);
static void Reset_Chair_Actor_dt(ACTOR* actorx, GAME* game);
static void Reset_Chair_Actor_move(ACTOR* actorx, GAME* game);
static void Reset_Chair_Actor_draw(ACTOR* actorx, GAME* game);

ACTOR_PROFILE Reset_Chair_Profile = {
    mAc_PROFILE_RESET_CHAIR,
    ACTOR_PART_ITEM,
    ACTOR_STATE_NO_DRAW_WHILE_CULLED | ACTOR_STATE_NO_MOVE_WHILE_CULLED,
    EMPTY_NO,
    ACTOR_OBJ_BANK_KEEP,
    sizeof(RESET_CHAIR_ACTOR),
    &Reset_Chair_Actor_ct,
    &Reset_Chair_Actor_dt,
    &Reset_Chair_Actor_move,
    &Reset_Chair_Actor_draw,
    NULL,
};

static void aRC_wait(RESET_CHAIR_ACTOR* chair, GAME* game);
static void aRC_sailing(RESET_CHAIR_ACTOR* chair, GAME* game);
static void aRC_return(RESET_CHAIR_ACTOR* chair, GAME* game);
static void aRC_hide(RESET_CHAIR_ACTOR* chair, GAME* game);

static aRC_ACT_PROC aRC_act_proc_table[] = {
    &aRC_wait,
    &aRC_sailing,
    &aRC_return,
    &aRC_hide,
};

static void aRC_setupAction(RESET_CHAIR_ACTOR* chair, int action) {
    chair->action = action;
    chair->act_proc = aRC_act_proc_table[action];
    chair->timer = 0;
}

/* aRC_wait: 4B - idle state, chair is stationary */
static void aRC_wait(RESET_CHAIR_ACTOR* chair, GAME* game) {
    return;
}

/* aRC_hide: 4B - hidden state, chair is invisible */
static void aRC_hide(RESET_CHAIR_ACTOR* chair, GAME* game) {
    return;
}

/* aRC_sailing: 136B - chair sails outward from start to target */
static void aRC_sailing(RESET_CHAIR_ACTOR* chair, GAME* game) {
    f32 t;

    chair->timer++;
    t = (f32)chair->timer / 60.0f;
    if (t > 1.0f) {
        t = 1.0f;
    }

    chair->move_t = t;
    chair->actor_class.world.position.x = chair->start_pos.x + (chair->target_pos.x - chair->start_pos.x) * t;
    chair->actor_class.world.position.y = chair->start_pos.y + (chair->target_pos.y - chair->start_pos.y) * t;
    chair->actor_class.world.position.z = chair->start_pos.z + (chair->target_pos.z - chair->start_pos.z) * t;

    if (t >= 1.0f) {
        aRC_setupAction(chair, aRC_ACT_HIDE);
    }
}

/* aRC_return: 144B - chair returns from target back to start */
static void aRC_return(RESET_CHAIR_ACTOR* chair, GAME* game) {
    f32 t;

    chair->timer++;
    t = (f32)chair->timer / 60.0f;
    if (t > 1.0f) {
        t = 1.0f;
    }

    chair->move_t = 1.0f - t;
    chair->actor_class.world.position.x = chair->target_pos.x + (chair->start_pos.x - chair->target_pos.x) * t;
    chair->actor_class.world.position.y = chair->target_pos.y + (chair->start_pos.y - chair->target_pos.y) * t;
    chair->actor_class.world.position.z = chair->target_pos.z + (chair->start_pos.z - chair->target_pos.z) * t;

    if (t >= 1.0f) {
        aRC_setupAction(chair, aRC_ACT_WAIT);
    }
}

/* Reset_Chair_Actor_ct: 132B */
static void Reset_Chair_Actor_ct(ACTOR* actorx, GAME* game) {
    RESET_CHAIR_ACTOR* chair = (RESET_CHAIR_ACTOR*)actorx;

    chair->start_pos = actorx->world.position;
    chair->target_pos.x = actorx->world.position.x + 200.0f;
    chair->target_pos.y = actorx->world.position.y;
    chair->target_pos.z = actorx->world.position.z;
    chair->move_t = 0.0f;

    aRC_setupAction(chair, aRC_ACT_WAIT);
}

/* Reset_Chair_Actor_dt: 4B */
static void Reset_Chair_Actor_dt(ACTOR* actorx, GAME* game) {
    return;
}

/* Reset_Chair_Actor_move: 88B */
static void Reset_Chair_Actor_move(ACTOR* actorx, GAME* game) {
    RESET_CHAIR_ACTOR* chair = (RESET_CHAIR_ACTOR*)actorx;

    if (chair->act_proc != NULL) {
        chair->act_proc(chair, game);
    }
}

/* Reset_Chair_Actor_draw: 164B */
static void Reset_Chair_Actor_draw(ACTOR* actorx, GAME* game) {
    RESET_CHAIR_ACTOR* chair = (RESET_CHAIR_ACTOR*)actorx;

    if (chair->action == aRC_ACT_HIDE) {
        return;
    }

    OPEN_DISP(game->graph);

    Matrix_translate(actorx->world.position.x, actorx->world.position.y, actorx->world.position.z, 0);
    Matrix_scale(0.01f, 0.01f, 0.01f, 1);

    gSPMatrix(NEXT_POLY_OPA_DISP, _Matrix_to_Mtx_new(game->graph), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);

    CLOSE_DISP(game->graph);
}
