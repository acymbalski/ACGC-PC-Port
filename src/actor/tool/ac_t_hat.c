#include "ac_t_hat.h"

#if VERSION >= VER_DELUXE

#include "m_name_table.h"
#include "sys_matrix.h"
#include "m_lib.h"
#include "m_rcp.h"

static void aTHT_actor_ct(ACTOR* actor, GAME* game);
static void aTHT_actor_move(ACTOR* actor, GAME* game);
static void aTHT_actor_draw(ACTOR* actor, GAME* game);
static void aTHT_setupAction(ACTOR* actor, int action);

ACTOR_PROFILE T_Hat_Profile = {
    mAc_PROFILE_T_HAT,
    ACTOR_PART_BG,
    ACTOR_STATE_NO_DRAW_WHILE_CULLED | ACTOR_STATE_NO_MOVE_WHILE_CULLED,
    EMPTY_NO,
    ACTOR_OBJ_BANK_TOOLS,
    sizeof(HAT_ACTOR),
    &aTHT_actor_ct,
    NONE_ACTOR_PROC,
    &aTHT_actor_move,
    &aTHT_actor_draw,
    NULL
};

extern Gfx crw_hat1_body_model[];

/* Deluxe: actor construct — initialize with wait action (36B) */
static void aTHT_actor_ct(ACTOR* actor, GAME* game) {
    aTHT_setupAction(actor, 4);
}

/* Deluxe: actor init — called during spawn (72B) */
static void aTHT_actor_init(ACTOR* actor, GAME* game) {
    HAT_ACTOR* hat = (HAT_ACTOR*)actor;

    hat->current_id = 0;
    hat->tools_class.work0 = 0;
    hat->proc = (HAT_PROC)none_proc1;
    aTHT_setupAction(actor, 4);
}

/* Deluxe: destruct — delete the actor (32B) */
static void aTHT_destruct(ACTOR* actor) {
    Actor_delete(actor);
}

/* Deluxe: set up action state with process table (32B) */
static void aTHT_setupAction(ACTOR* actor, int action) {
    HAT_ACTOR* hat = (HAT_ACTOR*)actor;
    static HAT_PROC process[] = {
        (HAT_PROC)none_proc1, (HAT_PROC)none_proc1, (HAT_PROC)none_proc1,
        aTHT_destruct, (HAT_PROC)none_proc1, NULL
    };

    hat->proc = process[action];
    hat->current_id = action;
    hat->tools_class.work0 = action;
}

/* Deluxe: actor move — dispatch current action (76B) */
static void aTHT_actor_move(ACTOR* actor, GAME* game) {
    HAT_ACTOR* hat = (HAT_ACTOR*)actor;

    if (hat->tools_class.work0 != hat->current_id) {
        aTHT_setupAction(actor, hat->tools_class.work0);
    }

    hat->proc(actor);
}

/* Deluxe: actor draw — render hat model from matrix (152B) */
static void aTHT_actor_draw(ACTOR* actor, GAME* game) {
    HAT_ACTOR* hat = (HAT_ACTOR*)actor;
    GRAPH* graph;
    Gfx* gfxp;

    if (hat->tools_class.init_matrix == 1) {
        graph = game->graph;

        OPEN_DISP(graph);

        Matrix_put(&hat->tools_class.matrix_work);
        Matrix_Position_Zero(&hat->tools_class.actor_class.world.position);

        hat->tools_class.init_matrix = 0;

        _texture_z_light_fog_prim_npc(graph);

        gfxp = NOW_POLY_OPA_DISP;
        gSPMatrix(gfxp++, _Matrix_to_Mtx_new(graph), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
        gSPDisplayList(gfxp++, crw_hat1_body_model);
        SET_POLY_OPA_DISP(gfxp);

        CLOSE_DISP(graph);
    }
}

#endif /* VERSION >= VER_DELUXE */
