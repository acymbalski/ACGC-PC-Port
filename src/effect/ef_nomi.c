#include "types.h"
#if VERSION >= VER_DELUXE
#include "ef_effect_control.h"

#include "m_common_data.h"
#include "m_rcp.h"
#include "sys_matrix.h"

static void eNM_init(xyz_t pos, int prio, s16 angle, GAME* game, u16 item_name, s16 arg0, s16 arg1);
static void eNM_ct(eEC_Effect_c* effect, GAME* game, void* ct_arg);
static void eNM_mv(eEC_Effect_c* effect, GAME* game);
static void eNM_dw(eEC_Effect_c* effect, GAME* game);

eEC_PROFILE_c iam_ef_nomi = {
    // clang-format off
    &eNM_init,
    &eNM_ct,
    &eNM_mv,
    &eNM_dw,
    eEC_IGNORE_DEATH,
    eEC_NO_CHILD_ID,
    eEC_DEFAULT_DEATH_DIST,
    // clang-format on
};

typedef struct {
    xyz_t velocity;
    f32 scale;
    s16 angle_y;
} eNM_data_c;

static void eNM_init(xyz_t pos, int prio, s16 angle, GAME* game, u16 item_name, s16 arg0, s16 arg1) {
    eNM_data_c data;

    data.velocity.x = RANDOM2_F(1.5f);
    data.velocity.y = 1.5f + RANDOM_F(1.0f);
    data.velocity.z = RANDOM2_F(1.5f);
    data.scale = 0.003f;
    data.angle_y = angle;

    eEC_CLIP->make_effect_proc(eEC_EFFECT_NOMI, pos, NULL, game, &data, item_name, prio, arg0, arg1);
}

/* eNM_ct: 344B - flea effect constructor */
static void eNM_ct(eEC_Effect_c* effect, GAME* game, void* ct_arg) {
    eNM_data_c* data = (eNM_data_c*)ct_arg;
    f32 rad_y;

    effect->scale.x = data->scale;
    effect->scale.y = data->scale;
    effect->scale.z = data->scale;

    effect->timer = 30;

    effect->velocity = data->velocity;

    effect->acceleration.x = 0.0f;
    effect->acceleration.y = -0.15f;
    effect->acceleration.z = 0.0f;

    rad_y = SHORT2RAD_ANGLE2(data->angle_y);
    eEC_CLIP->vector_rotate_y_proc(&effect->velocity, rad_y);

    effect->effect_specific[0] = 0;
    effect->effect_specific[1] = data->angle_y;
    effect->effect_specific[2] = (s16)(RANDOM_F(65535.0f));

    effect->offset.x = RANDOM2_F(3.0f);
    effect->offset.y = 0.0f;
    effect->offset.z = RANDOM2_F(3.0f);

    effect->position.x += effect->offset.x;
    effect->position.z += effect->offset.z;
}

/* eNM_mv: 96B - flea effect move/update */
static void eNM_mv(eEC_Effect_c* effect, GAME* game) {
    s16 counter = 30 - effect->timer;

    xyz_t_add(&effect->velocity, &effect->acceleration, &effect->velocity);
    xyz_t_add(&effect->position, &effect->velocity, &effect->position);

    effect->velocity.x *= sqrtf(0.9f);
    effect->velocity.z *= sqrtf(0.9f);

    effect->effect_specific[2] += 2000;
}

extern Gfx ef_dust01_modelT[];
extern u8 ef_dust01_0[];

/* eNM_dw: 404B - flea effect draw */
static void eNM_dw(eEC_Effect_c* effect, GAME* game) {
    s16 counter = 30 - effect->timer;
    f32 alpha_f;
    u8 alpha;
    f32 scale_val;
    xyz_t draw_scale;

    counter = CLAMP(counter, 0, 29);

    if (counter < 5) {
        alpha_f = eEC_CLIP->calc_adjust_proc(counter, 0, 5, 0.0f, 255.0f);
    } else if (counter > 20) {
        alpha_f = eEC_CLIP->calc_adjust_proc(counter, 20, 29, 255.0f, 0.0f);
    } else {
        alpha_f = 255.0f;
    }

    alpha = (u8)alpha_f;

    scale_val = eEC_CLIP->calc_adjust_proc(counter, 0, 15, 0.001f, effect->scale.x);

    draw_scale.x = scale_val;
    draw_scale.y = scale_val;
    draw_scale.z = scale_val;

    OPEN_DISP(game->graph);

    eEC_CLIP->auto_matrix_xlu_proc(game, &effect->position, &draw_scale);
    gSPSegment(NEXT_POLY_XLU_DISP, ANIME_1_TXT_SEG, ef_dust01_0);
    gSPSegment(NEXT_POLY_XLU_DISP, ANIME_2_TXT_SEG, ef_dust01_0);
    gDPSetPrimColor(NEXT_POLY_XLU_DISP, 0, 128, 80, 60, 40, alpha);
    gSPDisplayList(NEXT_POLY_XLU_DISP, ef_dust01_modelT);

    CLOSE_DISP(game->graph);
}
#endif /* VERSION >= VER_DELUXE */
