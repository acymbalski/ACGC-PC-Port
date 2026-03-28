#include "types.h"
#if VERSION >= VER_DELUXE
#include "ef_effect_control.h"

#include "m_common_data.h"
#include "m_rcp.h"
#include "sys_matrix.h"

static void eWaterDrop_init(xyz_t pos, int prio, s16 angle, GAME* game, u16 item_name, s16 arg0, s16 arg1);
static void eWaterDrop_ct(eEC_Effect_c* effect, GAME* game, void* ct_arg);
static void eWaterDrop_mv(eEC_Effect_c* effect, GAME* game);
static void eWaterDrop_dw(eEC_Effect_c* effect, GAME* game);

eEC_PROFILE_c iam_ef_water_drop = {
    // clang-format off
    &eWaterDrop_init,
    &eWaterDrop_ct,
    &eWaterDrop_mv,
    &eWaterDrop_dw,
    eEC_IGNORE_DEATH,
    eEC_NO_CHILD_ID,
    eEC_DEFAULT_DEATH_DIST,
    // clang-format on
};

typedef struct {
    xyz_t velocity;
    xyz_t acceleration;
    f32 scale;
    s16 angle_y;
} eWaterDrop_data_c;

/* eWaterDrop_init: 128B */
static void eWaterDrop_init(xyz_t pos, int prio, s16 angle, GAME* game, u16 item_name, s16 arg0, s16 arg1) {
    eWaterDrop_data_c data;

    data.velocity.x = RANDOM2_F(1.0f);
    data.velocity.y = 2.0f + RANDOM_F(1.5f);
    data.velocity.z = RANDOM2_F(1.0f);
    data.acceleration.x = 0.0f;
    data.acceleration.y = -0.2f;
    data.acceleration.z = 0.0f;
    data.scale = 0.004f;
    data.angle_y = angle;

    eEC_CLIP->make_effect_proc(eEC_EFFECT_WATER_DROP, pos, NULL, game, &data, item_name, prio, arg0, arg1);
}

extern u8 ef_dust01_0[];
extern u8 ef_dust01_1[];
extern u8 ef_dust01_2[];
extern u8 ef_dust01_3[];

static u8* eWaterDrop_tex_table[] = {
    ef_dust01_0,
    ef_dust01_1,
    ef_dust01_2,
    ef_dust01_3,
};

typedef struct {
    u8 tex0;
    u8 tex1;
} eWaterDrop_2tile_c;

static eWaterDrop_2tile_c eWaterDrop_tile_idx[] = {
    {0, 0},
    {0, 1},
    {1, 1},
    {1, 2},
    {2, 2},
    {2, 3},
    {3, 3},
    {3, 3},
    {3, 3},
    {3, 3},
};

static u8 eWaterDrop_alpha_table[20] = {
    0xFF, 0xFF, 0xE0, 0xC0, 0xA0, 0x80, 0x60, 0x40,
    0x20, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
};

/* eWaterDrop_ct: 300B - water drop constructor */
static void eWaterDrop_ct(eEC_Effect_c* effect, GAME* game, void* ct_arg) {
    eWaterDrop_data_c* data = (eWaterDrop_data_c*)ct_arg;
    f32 rad_y;

    effect->scale.x = data->scale;
    effect->scale.y = data->scale;
    effect->scale.z = data->scale;

    effect->timer = 20;

    effect->velocity = data->velocity;
    effect->acceleration = data->acceleration;

    rad_y = SHORT2RAD_ANGLE2(data->angle_y);
    eEC_CLIP->vector_rotate_y_proc(&effect->velocity, rad_y);

    effect->effect_specific[0] = 0;
    effect->effect_specific[1] = data->angle_y;

    effect->position.x += RANDOM2_F(5.0f);
    effect->position.y += RANDOM_F(3.0f);
    effect->position.z += RANDOM2_F(5.0f);

    effect->offset.x = effect->scale.x;
    effect->offset.y = 0.0f;
    effect->offset.z = 0.0f;
}

/* eWaterDrop_mv: 112B - water drop update */
static void eWaterDrop_mv(eEC_Effect_c* effect, GAME* game) {
    s16 counter = 20 - effect->timer;

    xyz_t_add(&effect->velocity, &effect->acceleration, &effect->velocity);
    xyz_t_add(&effect->position, &effect->velocity, &effect->position);

    effect->velocity.x *= sqrtf(0.85f);
    effect->velocity.z *= sqrtf(0.85f);

    effect->scale.x = eEC_CLIP->calc_adjust_proc(counter, 0, 10, effect->offset.x, effect->offset.x * 2.0f);
    effect->scale.y = effect->scale.x;
    effect->scale.z = effect->scale.x;
}

extern Gfx ef_dust01_modelT[];

/* eWaterDrop_dw: 664B - water drop draw */
static void eWaterDrop_dw(eEC_Effect_c* effect, GAME* game) {
    s16 counter = 20 - effect->timer;
    int tex0;
    int tex1;
    u8 alpha;
    f32 scale_mult;

    counter = CLAMP(counter, 0, 19);

    tex0 = eWaterDrop_tile_idx[counter >> 1].tex0;
    tex1 = eWaterDrop_tile_idx[counter >> 1].tex1;
    alpha = eWaterDrop_alpha_table[counter];

    OPEN_DISP(game->graph);

    eEC_CLIP->auto_matrix_xlu_proc(game, &effect->position, &effect->scale);

    gSPSegment(NEXT_POLY_XLU_DISP, ANIME_1_TXT_SEG, eWaterDrop_tex_table[tex0]);
    gSPSegment(NEXT_POLY_XLU_DISP, ANIME_2_TXT_SEG, eWaterDrop_tex_table[tex1]);

    if (effect->arg0 == 0) {
        gDPSetPrimColor(NEXT_POLY_XLU_DISP, 0, 128, 180, 200, 255, alpha);
    } else if (effect->arg0 == 1) {
        gDPSetPrimColor(NEXT_POLY_XLU_DISP, 0, 128, 150, 180, 220, alpha);
    } else {
        gDPSetPrimColor(NEXT_POLY_XLU_DISP, 0, 128, 200, 220, 255, alpha);
    }

    gSPDisplayList(NEXT_POLY_XLU_DISP, ef_dust01_modelT);

    CLOSE_DISP(game->graph);
}
#endif /* VERSION >= VER_DELUXE */
