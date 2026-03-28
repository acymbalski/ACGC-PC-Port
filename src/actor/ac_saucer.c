#if VERSION >= VER_DELUXE

#include "ac_saucer.h"

#include "m_common_data.h"
#include "m_name_table.h"
#include "m_rcp.h"
#include "m_play.h"
#include "sys_matrix.h"
#include "m_lib.h"
#include "graph.h"

static void aSA_actor_ct(ACTOR* actor, GAME* game);
static void aSA_actor_dt(ACTOR* actor, GAME* game);
static void aSA_actor_move(ACTOR* actor, GAME* game);
static void aSA_actor_draw(ACTOR* actor, GAME* game);

/* clang-format off */
ACTOR_PROFILE Saucer_Profile = {
    mAc_PROFILE_SAUCER,
    ACTOR_PART_ITEM,
    ACTOR_STATE_NO_MOVE_WHILE_CULLED | ACTOR_STATE_NO_DRAW_WHILE_CULLED | ACTOR_STATE_CAN_MOVE_IN_DEMO_SCENES,
    EMPTY_NO,
    ACTOR_OBJ_BANK_KEEP,
    sizeof(SAUCER_ACTOR),
    &aSA_actor_ct,
    &aSA_actor_dt,
    &aSA_actor_move,
    &aSA_actor_draw,
    NULL,
};
/* clang-format on */

/* ============================================================ */
/* aSA_actor_ct (260B)                                          */
/* Constructor - initialize the saucer actor                    */
/* ============================================================ */
static void aSA_actor_ct(ACTOR* actor, GAME* game) {
    SAUCER_ACTOR* saucer = (SAUCER_ACTOR*)actor;

    saucer->hover_y = actor->world.position.y;
    saucer->spin_angle = 0.0f;
    saucer->hover_timer = 0.0f;
    saucer->state = 0;

    actor->scale.x = 0.01f;
    actor->scale.y = 0.01f;
    actor->scale.z = 0.01f;

    actor->cull_width = 800.0f;
    actor->cull_radius = 800.0f;
    actor->gravity = 0.0f;
    actor->max_velocity_y = 0.0f;
}

/* ============================================================ */
/* aSA_actor_dt (4B)                                            */
/* Destructor - nothing to clean up                             */
/* ============================================================ */
static void aSA_actor_dt(ACTOR* actor, GAME* game) {
    return;
}

/* ============================================================ */
/* aSA_actor_move (104B)                                        */
/* Main update - hover bobbing and spin rotation                */
/* ============================================================ */
static void aSA_actor_move(ACTOR* actor, GAME* game) {
    SAUCER_ACTOR* saucer = (SAUCER_ACTOR*)actor;

    /* Advance spin rotation */
    saucer->spin_angle += 1000.0f;
    if (saucer->spin_angle >= 65536.0f) {
        saucer->spin_angle -= 65536.0f;
    }

    /* Hover bobbing motion */
    saucer->hover_timer += 400.0f;
    if (saucer->hover_timer >= 65536.0f) {
        saucer->hover_timer -= 65536.0f;
    }

    actor->world.position.y = saucer->hover_y + sin_s((s16)saucer->hover_timer) * 10.0f;
}

/* ============================================================ */
/* aSA_actor_draw (328B)                                        */
/* Draw callback - render the saucer model                      */
/* ============================================================ */
static void aSA_actor_draw(ACTOR* actor, GAME* game) {
    SAUCER_ACTOR* saucer = (SAUCER_ACTOR*)actor;
    GRAPH* g = game->graph;

    _texture_z_light_fog_prim(g);

    OPEN_DISP(g);

    Matrix_translate(actor->world.position.x, actor->world.position.y, actor->world.position.z, MTX_LOAD);
    Matrix_RotateY((s16)saucer->spin_angle, MTX_MULT);
    Matrix_scale(0.01f, 0.01f, 0.01f, MTX_MULT);

    gSPMatrix(NEXT_POLY_OPA_DISP, _Matrix_to_Mtx_new(g), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);

    CLOSE_DISP(g);
}

#endif /* VERSION >= VER_DELUXE */
