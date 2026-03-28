#include "ac_resetcenter_indoor.h"

#include "m_common_data.h"
#include "m_rcp.h"
#include "sys_matrix.h"
#include "m_player_lib.h"
#include "m_scene_table.h"

static void Resetcenter_Indoor_Actor_ct(ACTOR* actorx, GAME* game);
static void Resetcenter_Indoor_Actor_dt(ACTOR* actorx, GAME* game);
static void Resetcenter_Indoor_Actor_move(ACTOR* actorx, GAME* game);
static void Resetcenter_Indoor_Actor_draw(ACTOR* actorx, GAME* game);

ACTOR_PROFILE Resetcenter_Indoor_Profile = {
    mAc_PROFILE_RESETCENTER_INDOOR,
    ACTOR_PART_ITEM,
    ACTOR_STATE_NO_DRAW_WHILE_CULLED | ACTOR_STATE_NO_MOVE_WHILE_CULLED,
    EMPTY_NO,
    ACTOR_OBJ_BANK_KEEP,
    sizeof(RESETCENTER_INDOOR_ACTOR),
    &Resetcenter_Indoor_Actor_ct,
    &Resetcenter_Indoor_Actor_dt,
    &Resetcenter_Indoor_Actor_move,
    &Resetcenter_Indoor_Actor_draw,
    NULL,
};

/* ========== Door Management ========== */

/* aRI_door_ct: 16B */
static void aRI_door_ct(aRI_door_c* door) {
    door->alpha = 0.0f;
    door->target_alpha = 0.0f;
    door->speed = 0.05f;
    door->state = 0;
}

/* aRI_door_dt: 16B */
static void aRI_door_dt(aRI_door_c* door) {
    door->alpha = 0.0f;
    door->state = 0;
}

/* aRI_door_move: 92B */
static void aRI_door_move(aRI_door_c* door) {
    f32 diff = door->target_alpha - door->alpha;

    if (diff > door->speed) {
        door->alpha += door->speed;
    } else if (diff < -door->speed) {
        door->alpha -= door->speed;
    } else {
        door->alpha = door->target_alpha;
    }
}

/* aRI_request_appear_door: 96B */
static void aRI_request_appear_door(RESETCENTER_INDOOR_ACTOR* actor) {
    actor->door.target_alpha = 1.0f;
    actor->door.speed = 0.04f;
    actor->door.state = 1;
}

/* aRI_request_disappear_door: 52B */
static void aRI_request_disappear_door(RESETCENTER_INDOOR_ACTOR* actor) {
    actor->door.target_alpha = 0.0f;
    actor->door.speed = 0.08f;
    actor->door.state = 2;
}

/* aRI_get_door_alpha_percent: 44B */
static f32 aRI_get_door_alpha_percent(RESETCENTER_INDOOR_ACTOR* actor) {
    return actor->door.alpha;
}

/* aRI_draw_door: 300B */
static void aRI_draw_door(RESETCENTER_INDOOR_ACTOR* actor, GAME* game) {
    u8 alpha;

    if (actor->door.alpha <= 0.0f) {
        return;
    }

    alpha = (u8)(actor->door.alpha * 255.0f);

    OPEN_DISP(game->graph);

    Matrix_translate(actor->actor_class.world.position.x, actor->actor_class.world.position.y,
                     actor->actor_class.world.position.z, 0);
    Matrix_scale(0.01f, 0.01f, 0.01f, 1);

    gDPSetPrimColor(NEXT_POLY_XLU_DISP, 0, 128, 100, 80, 60, alpha);
    gSPMatrix(NEXT_POLY_XLU_DISP, _Matrix_to_Mtx_new(game->graph), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);

    CLOSE_DISP(game->graph);
}

/* ========== Lighting System ========== */

/* aRI_light_ct: 272B */
static void aRI_light_ct(aRI_light_c* light, GAME_PLAY* play) {
    light->state = 0;
    light->target_state = 0;
    light->intensity = 0.0f;
    light->target_intensity = 0.0f;

    Light_diffuse_ct(&light->light, 0, 200, 180, 120, 0, 0);
    light->light_list = Global_light_list_new((GAME*)play, &play->global_light, &light->light);
}

/* aRI_light_dt: 52B */
static void aRI_light_dt(aRI_light_c* light, GAME_PLAY* play) {
    if (light->light_list != NULL) {
        Global_light_list_delete(&play->global_light, light->light_list);
        light->light_list = NULL;
    }
}

/* aRI_light_move: 672B */
static void aRI_light_move(aRI_light_c* light, GAME_PLAY* play) {
    f32 diff;

    diff = light->target_intensity - light->intensity;
    if (diff > 0.02f) {
        light->intensity += 0.02f;
    } else if (diff < -0.02f) {
        light->intensity -= 0.02f;
    } else {
        light->intensity = light->target_intensity;
    }

    if (light->intensity > 0.0f) {
        u8 r = (u8)(200.0f * light->intensity);
        u8 g = (u8)(180.0f * light->intensity);
        u8 b = (u8)(120.0f * light->intensity);

        Light_diffuse_set(&light->light, r, g, b,
                          (s16)light->position.x, (s16)light->position.y, (s16)light->position.z);
    }
}

/* aRI_make_point_light: 80B */
static void aRI_make_point_light(RESETCENTER_INDOOR_ACTOR* actor, GAME_PLAY* play) {
    int i;
    for (i = 0; i < aRI_LIGHT_NUM; i++) {
        aRI_light_ct(&actor->lights[i], play);
    }
}

/* aRI_delete_point_light: 52B */
static void aRI_delete_point_light(RESETCENTER_INDOOR_ACTOR* actor, GAME_PLAY* play) {
    int i;
    for (i = 0; i < aRI_LIGHT_NUM; i++) {
        aRI_light_dt(&actor->lights[i], play);
    }
}

/* aRI_request_light_on: 80B */
static void aRI_request_light_on(RESETCENTER_INDOOR_ACTOR* actor) {
    int i;
    for (i = 0; i < aRI_LIGHT_NUM; i++) {
        actor->lights[i].target_state = 1;
        actor->lights[i].target_intensity = 1.0f;
    }
}

/* aRI_request_light_off: 80B */
static void aRI_request_light_off(RESETCENTER_INDOOR_ACTOR* actor) {
    int i;
    for (i = 0; i < aRI_LIGHT_NUM; i++) {
        actor->lights[i].target_state = 0;
        actor->lights[i].target_intensity = 0.0f;
    }
}

/* aRI_draw_light_before: 32B */
static void aRI_draw_light_before(GAME* game, cKF_SkeletonInfo_R_c* keyframe, int joint_idx,
                                   Gfx** dl_pp, u8* dl_flags, void* arg,
                                   s_xyz* joint_rot, xyz_t* joint_pos) {
    return;
}

/* aRI_draw_light: 392B */
static void aRI_draw_light(RESETCENTER_INDOOR_ACTOR* actor, GAME* game) {
    int i;

    OPEN_DISP(game->graph);

    for (i = 0; i < aRI_LIGHT_NUM; i++) {
        if (actor->lights[i].intensity > 0.0f) {
            u8 alpha = (u8)(actor->lights[i].intensity * 200.0f);

            Matrix_translate(actor->lights[i].position.x, actor->lights[i].position.y,
                             actor->lights[i].position.z, 0);
            Matrix_scale(0.01f, 0.01f, 0.01f, 1);

            gDPSetPrimColor(NEXT_POLY_XLU_DISP, 0, 128, 255, 200, 100, alpha);
            gSPMatrix(NEXT_POLY_XLU_DISP, _Matrix_to_Mtx_new(game->graph),
                       G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
        }
    }

    CLOSE_DISP(game->graph);
}

/* ========== TV System ========== */

/* aRI_tv_ct: 88B */
static void aRI_tv_ct(RESETCENTER_INDOOR_ACTOR* actor, GAME_PLAY* play) {
    actor->tv.state = 0;
    actor->tv.timer = 0;
    actor->tv.frame = 0;
    actor->tv.flicker = 0.0f;
    actor->tv.color.r = 80;
    actor->tv.color.g = 120;
    actor->tv.color.b = 200;
    actor->tv.color.a = 255;
}

/* aRI_tv_dt: 32B */
static void aRI_tv_dt(RESETCENTER_INDOOR_ACTOR* actor, GAME_PLAY* play) {
    actor->tv.state = 0;
}

/* aRI_tv_move: 552B */
static void aRI_tv_move(aRI_tv_c* tv) {
    if (tv->state == 0) {
        return;
    }

    tv->timer++;
    tv->frame = (tv->timer >> 2) & 3;

    /* TV flicker effect */
    tv->flicker = 0.8f + RANDOM_F(0.2f);

    /* Color shift based on frame */
    switch (tv->frame) {
        case 0:
            tv->color.r = (u8)(80.0f * tv->flicker);
            tv->color.g = (u8)(120.0f * tv->flicker);
            tv->color.b = (u8)(200.0f * tv->flicker);
            break;
        case 1:
            tv->color.r = (u8)(100.0f * tv->flicker);
            tv->color.g = (u8)(140.0f * tv->flicker);
            tv->color.b = (u8)(180.0f * tv->flicker);
            break;
        case 2:
            tv->color.r = (u8)(60.0f * tv->flicker);
            tv->color.g = (u8)(100.0f * tv->flicker);
            tv->color.b = (u8)(160.0f * tv->flicker);
            break;
        case 3:
            tv->color.r = (u8)(90.0f * tv->flicker);
            tv->color.g = (u8)(130.0f * tv->flicker);
            tv->color.b = (u8)(190.0f * tv->flicker);
            break;
    }
}

/* aRI_draw_tvlight: 308B */
static void aRI_draw_tvlight(RESETCENTER_INDOOR_ACTOR* actor, GAME* game) {
    if (actor->tv.state == 0) {
        return;
    }

    OPEN_DISP(game->graph);

    Matrix_translate(actor->actor_class.world.position.x,
                     actor->actor_class.world.position.y + 30.0f,
                     actor->actor_class.world.position.z - 20.0f, 0);
    Matrix_scale(0.005f, 0.005f, 0.005f, 1);

    gDPSetPrimColor(NEXT_POLY_XLU_DISP, 0, 128,
                     actor->tv.color.r, actor->tv.color.g, actor->tv.color.b, 180);
    gSPMatrix(NEXT_POLY_XLU_DISP, _Matrix_to_Mtx_new(game->graph),
               G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);

    CLOSE_DISP(game->graph);
}

#if VERSION >= VER_DELUXE
extern void aRI_tv_sound(void) {
    static xyz_t tv_pos = { 0.0f, 30.0f, -20.0f };

    sAdo_OngenTrgStart(0x461, &tv_pos);
}
#endif /* VERSION >= VER_DELUXE */

/* ========== Mode Management ========== */

/* aRI_get_reset_mode: 180B */
static int aRI_get_reset_mode(RESETCENTER_INDOOR_ACTOR* actor) {
    return actor->mode;
}

/* aRI_get_reset_mode_E: 64B */
static int aRI_get_reset_mode_E(RESETCENTER_INDOOR_ACTOR* actor) {
    if (actor->mode == aRI_MODE_ANGRY) {
        return 1;
    }
    return 0;
}

/* aRI_next_mode: 164B */
static void aRI_next_mode(RESETCENTER_INDOOR_ACTOR* actor) {
    actor->mode = actor->next_mode;
    actor->mode_timer = 0;

    switch (actor->mode) {
        case aRI_MODE_NORMAL:
            aRI_request_light_on(actor);
            actor->tv.state = 1;
            break;
        case aRI_MODE_ANGRY:
            aRI_request_light_off(actor);
            actor->tv.state = 0;
            break;
        case aRI_MODE_CALM:
            aRI_request_light_on(actor);
            actor->tv.state = 1;
            break;
    }
}

/* ========== Color Management ========== */

/* aRI_get_room_prim_color: 72B */
static void aRI_get_room_prim_color(u8* r, u8* g, u8* b, RESETCENTER_INDOOR_ACTOR* actor) {
    *r = actor->room_prim_r;
    *g = actor->room_prim_g;
    *b = actor->room_prim_b;
}

/* aRI_set_resetcenter_prim_color: 444B */
static void aRI_set_resetcenter_prim_color(RESETCENTER_INDOOR_ACTOR* actor) {
    switch (actor->mode) {
        case aRI_MODE_NORMAL:
            actor->room_prim_r = 200;
            actor->room_prim_g = 180;
            actor->room_prim_b = 150;
            break;
        case aRI_MODE_ANGRY:
            /* Dim red during angry mode */
            actor->room_prim_r = 180;
            actor->room_prim_g = 80;
            actor->room_prim_b = 60;
            break;
        case aRI_MODE_CALM:
            actor->room_prim_r = 190;
            actor->room_prim_g = 190;
            actor->room_prim_b = 170;
            break;
    }
}

/* aRI_set_bg_disp_prim_color: 424B */
static void aRI_set_bg_disp_prim_color(RESETCENTER_INDOOR_ACTOR* actor, GAME* game) {
    OPEN_DISP(game->graph);

    gDPSetPrimColor(NEXT_POLY_OPA_DISP, 0, 128,
                     actor->room_prim_r, actor->room_prim_g, actor->room_prim_b, 255);

    CLOSE_DISP(game->graph);
}

/* aRI_draw_normal: 276B */
static void aRI_draw_normal(RESETCENTER_INDOOR_ACTOR* actor, GAME* game) {
    OPEN_DISP(game->graph);

    Matrix_translate(actor->actor_class.world.position.x,
                     actor->actor_class.world.position.y,
                     actor->actor_class.world.position.z, 0);
    Matrix_scale(0.01f, 0.01f, 0.01f, 1);

    gDPSetPrimColor(NEXT_POLY_OPA_DISP, 0, 128,
                     actor->room_prim_r, actor->room_prim_g, actor->room_prim_b, 255);
    gSPMatrix(NEXT_POLY_OPA_DISP, _Matrix_to_Mtx_new(game->graph),
               G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);

    CLOSE_DISP(game->graph);
}

/* aRI_delete_boots_of_Racket: 48B */
static void aRI_delete_boots_of_Racket(void) {
    /* Remove racket NPC boots model reference */
}

/* aRI_delete_boots_of_Reset: 48B */
static void aRI_delete_boots_of_Reset(void) {
    /* Remove reset NPC boots model reference */
}

/* ========== Main Actor Callbacks ========== */

/* Resetcenter_Indoor_Actor_ct: 192B */
static void Resetcenter_Indoor_Actor_ct(ACTOR* actorx, GAME* game) {
    RESETCENTER_INDOOR_ACTOR* actor = (RESETCENTER_INDOOR_ACTOR*)actorx;
    GAME_PLAY* play = (GAME_PLAY*)game;

    actor->mode = aRI_MODE_NORMAL;
    actor->next_mode = aRI_MODE_NORMAL;
    actor->mode_timer = 0;

    aRI_door_ct(&actor->door);
    aRI_make_point_light(actor, play);
    aRI_tv_ct(actor, play);

    /* Set initial light positions */
    actor->lights[0].position.x = actorx->world.position.x - 30.0f;
    actor->lights[0].position.y = actorx->world.position.y + 50.0f;
    actor->lights[0].position.z = actorx->world.position.z;

    actor->lights[1].position.x = actorx->world.position.x + 30.0f;
    actor->lights[1].position.y = actorx->world.position.y + 50.0f;
    actor->lights[1].position.z = actorx->world.position.z;

    aRI_set_resetcenter_prim_color(actor);
    aRI_request_light_on(actor);
    actor->tv.state = 1;
}

/* Resetcenter_Indoor_Actor_dt: 88B */
static void Resetcenter_Indoor_Actor_dt(ACTOR* actorx, GAME* game) {
    RESETCENTER_INDOOR_ACTOR* actor = (RESETCENTER_INDOOR_ACTOR*)actorx;
    GAME_PLAY* play = (GAME_PLAY*)game;

    aRI_door_dt(&actor->door);
    aRI_delete_point_light(actor, play);
    aRI_tv_dt(actor, play);
}

/* Resetcenter_Indoor_Actor_move: 72B */
static void Resetcenter_Indoor_Actor_move(ACTOR* actorx, GAME* game) {
    RESETCENTER_INDOOR_ACTOR* actor = (RESETCENTER_INDOOR_ACTOR*)actorx;
    GAME_PLAY* play = (GAME_PLAY*)game;
    int i;

    actor->mode_timer++;
    aRI_door_move(&actor->door);
    aRI_tv_move(&actor->tv);

    for (i = 0; i < aRI_LIGHT_NUM; i++) {
        aRI_light_move(&actor->lights[i], play);
    }
}

/* Resetcenter_Indoor_Actor_draw: 132B */
static void Resetcenter_Indoor_Actor_draw(ACTOR* actorx, GAME* game) {
    RESETCENTER_INDOOR_ACTOR* actor = (RESETCENTER_INDOOR_ACTOR*)actorx;

    aRI_set_bg_disp_prim_color(actor, game);
    aRI_draw_normal(actor, game);
    aRI_draw_door(actor, game);
    aRI_draw_light(actor, game);
    aRI_draw_tvlight(actor, game);
}
