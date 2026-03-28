#include "types.h"
#if VERSION >= VER_DELUXE
#include "ef_effect_control.h"

#include "m_common_data.h"
#include "m_rcp.h"
#include "sys_matrix.h"

static void eResetbreak_Parts_init(xyz_t pos, int prio, s16 angle, GAME* game, u16 item_name, s16 arg0, s16 arg1);
static void eResetbreak_Parts_ct(eEC_Effect_c* effect, GAME* game, void* ct_arg);
static void eResetbreak_Parts_mv(eEC_Effect_c* effect, GAME* game);
static void eResetbreak_Parts_dw(eEC_Effect_c* effect, GAME* game);

eEC_PROFILE_c iam_ef_resetbreak_parts = {
    // clang-format off
    &eResetbreak_Parts_init,
    &eResetbreak_Parts_ct,
    &eResetbreak_Parts_mv,
    &eResetbreak_Parts_dw,
    eEC_IGNORE_DEATH,
    eEC_NO_CHILD_ID,
    eEC_DEFAULT_DEATH_DIST,
    // clang-format on
};

typedef struct {
    xyz_t velocity;
    xyz_t acceleration;
    f32 scale;
    s16 rotation;
    s16 rot_speed;
    s16 part_type;
} eResetbreak_Parts_data_c;

/* eResetbreak_Parts_init: 116B */
static void eResetbreak_Parts_init(xyz_t pos, int prio, s16 angle, GAME* game, u16 item_name, s16 arg0, s16 arg1) {
    eResetbreak_Parts_data_c data;

    data.velocity.x = RANDOM2_F(3.0f);
    data.velocity.y = 3.0f + RANDOM_F(4.0f);
    data.velocity.z = RANDOM2_F(3.0f);
    data.acceleration.x = 0.0f;
    data.acceleration.y = -0.25f;
    data.acceleration.z = 0.0f;
    data.scale = 0.01f + RANDOM_F(0.005f);
    data.rotation = (s16)(RANDOM_F(65535.0f));
    data.rot_speed = (s16)(RANDOM_F(3000.0f)) + 500;
    data.part_type = arg0;

    eEC_CLIP->make_effect_proc(eEC_EFFECT_RESETBREAK_PARTS, pos, NULL, game, &data, item_name, prio, arg0, arg1);
}

extern Gfx ef_dust01_modelT[];
extern u8 ef_dust01_0[];
extern u8 ef_dust01_1[];
extern u8 ef_dust01_2[];
extern u8 ef_dust01_3[];

/* eResetbreak_Parts_ct: 596B - reset break parts constructor */
static void eResetbreak_Parts_ct(eEC_Effect_c* effect, GAME* game, void* ct_arg) {
    eResetbreak_Parts_data_c* data = (eResetbreak_Parts_data_c*)ct_arg;
    f32 rad_y;
    f32 speed_x;
    f32 speed_z;

    effect->scale.x = data->scale;
    effect->scale.y = data->scale;
    effect->scale.z = data->scale;

    effect->timer = 40;

    effect->velocity = data->velocity;
    effect->acceleration = data->acceleration;

    effect->effect_specific[0] = data->rotation;
    effect->effect_specific[1] = data->rot_speed;
    effect->effect_specific[2] = data->part_type;
    effect->effect_specific[3] = 0;
    effect->effect_specific[4] = 0;
    effect->effect_specific[5] = 0;

    effect->offset.x = effect->position.x;
    effect->offset.y = effect->position.y;
    effect->offset.z = effect->position.z;

    /* Randomize initial spread based on part type */
    if (data->part_type == 0) {
        speed_x = RANDOM2_F(2.0f);
        speed_z = RANDOM2_F(2.0f);
        effect->velocity.x += speed_x;
        effect->velocity.z += speed_z;
        effect->acceleration.y = -0.2f;
    } else if (data->part_type == 1) {
        speed_x = RANDOM2_F(4.0f);
        speed_z = RANDOM2_F(4.0f);
        effect->velocity.x += speed_x;
        effect->velocity.z += speed_z;
        effect->velocity.y += RANDOM_F(2.0f);
        effect->acceleration.y = -0.3f;
    } else {
        speed_x = RANDOM2_F(1.5f);
        speed_z = RANDOM2_F(1.5f);
        effect->velocity.x += speed_x;
        effect->velocity.z += speed_z;
        effect->acceleration.y = -0.15f;
    }

    /* Apply gravity variation per part */
    effect->acceleration.y += RANDOM2_F(0.05f);

    /* Bounce parameters stored in offset */
    effect->offset.x = 0.0f;
    effect->offset.y = effect->position.y;
    effect->offset.z = 0.0f;
}

/* eResetbreak_Parts_mv: 372B - reset break parts move/update */
static void eResetbreak_Parts_mv(eEC_Effect_c* effect, GAME* game) {
    s16 counter = 40 - effect->timer;

    xyz_t_add(&effect->velocity, &effect->acceleration, &effect->velocity);
    xyz_t_add(&effect->position, &effect->velocity, &effect->position);

    effect->velocity.x *= sqrtf(0.95f);
    effect->velocity.z *= sqrtf(0.95f);

    effect->effect_specific[0] += effect->effect_specific[1];

    /* Ground bounce check */
    if (effect->position.y < effect->offset.y && effect->effect_specific[3] < 3) {
        effect->position.y = effect->offset.y;
        effect->velocity.y = -effect->velocity.y * 0.4f;
        effect->effect_specific[3]++;
        effect->effect_specific[1] = (s16)(effect->effect_specific[1] * 0.7f);
    }

    /* Scale down as particle ages */
    if (counter > 30) {
        effect->scale.x = eEC_CLIP->calc_adjust_proc(counter, 30, 40, effect->scale.x, 0.0f);
        effect->scale.y = effect->scale.x;
        effect->scale.z = effect->scale.x;
    }
}

/* eResetbreak_Parts_dw: 296B - reset break parts draw */
static void eResetbreak_Parts_dw(eEC_Effect_c* effect, GAME* game) {
    s16 counter = 40 - effect->timer;
    u8 alpha;
    f32 alpha_f;

    counter = CLAMP(counter, 0, 39);

    if (counter > 30) {
        alpha_f = eEC_CLIP->calc_adjust_proc(counter, 30, 39, 255.0f, 0.0f);
    } else {
        alpha_f = 255.0f;
    }
    alpha = (u8)alpha_f;

    OPEN_DISP(game->graph);

    eEC_CLIP->auto_matrix_xlu_proc(game, &effect->position, &effect->scale);
    gSPSegment(NEXT_POLY_XLU_DISP, ANIME_1_TXT_SEG, ef_dust01_0);
    gSPSegment(NEXT_POLY_XLU_DISP, ANIME_2_TXT_SEG, ef_dust01_1);

    if (effect->effect_specific[2] == 0) {
        gDPSetPrimColor(NEXT_POLY_XLU_DISP, 0, 128, 180, 160, 140, alpha);
    } else if (effect->effect_specific[2] == 1) {
        gDPSetPrimColor(NEXT_POLY_XLU_DISP, 0, 128, 140, 130, 120, alpha);
    } else {
        gDPSetPrimColor(NEXT_POLY_XLU_DISP, 0, 128, 200, 180, 160, alpha);
    }

    gSPDisplayList(NEXT_POLY_XLU_DISP, ef_dust01_modelT);

    CLOSE_DISP(game->graph);
}
#endif /* VERSION >= VER_DELUXE */
