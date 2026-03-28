#include "types.h"
#if VERSION >= VER_DELUXE
#include "ef_effect_control.h"

#include "m_common_data.h"
#include "m_rcp.h"
#include "sys_matrix.h"

static void eResetbreak_Dust_init(xyz_t pos, int prio, s16 angle, GAME* game, u16 item_name, s16 arg0, s16 arg1);
static void eResetbreak_Dust_ct(eEC_Effect_c* effect, GAME* game, void* ct_arg);
static void eResetbreak_Dust_mv(eEC_Effect_c* effect, GAME* game);
static void eResetbreak_Dust_dw(eEC_Effect_c* effect, GAME* game);

eEC_PROFILE_c iam_ef_resetbreak_dust = {
    // clang-format off
    &eResetbreak_Dust_init,
    &eResetbreak_Dust_ct,
    &eResetbreak_Dust_mv,
    &eResetbreak_Dust_dw,
    eEC_IGNORE_DEATH,
    eEC_NO_CHILD_ID,
    eEC_DEFAULT_DEATH_DIST,
    // clang-format on
};

extern u8 ef_dust01_0[];
extern u8 ef_dust01_1[];
extern u8 ef_dust01_2[];
extern u8 ef_dust01_3[];
extern Gfx ef_dust01_modelT[];

static u8* eResetbreak_Dust_tex_table[] = {
    ef_dust01_0,
    ef_dust01_1,
    ef_dust01_2,
    ef_dust01_3,
};

/* eResetbreak_Dust_init: 116B */
static void eResetbreak_Dust_init(xyz_t pos, int prio, s16 angle, GAME* game, u16 item_name, s16 arg0, s16 arg1) {
    eEC_CLIP->make_effect_proc(eEC_EFFECT_RESETBREAK_DUST, pos, NULL, game, NULL, item_name, prio, arg0, arg1);
}

/* eResetbreak_Dust_ct: 300B */
static void eResetbreak_Dust_ct(eEC_Effect_c* effect, GAME* game, void* ct_arg) {
    effect->scale.x = 0.008f + RANDOM_F(0.004f);
    effect->scale.y = effect->scale.x;
    effect->scale.z = effect->scale.x;

    effect->timer = 24;

    effect->velocity.x = RANDOM2_F(2.0f);
    effect->velocity.y = 1.0f + RANDOM_F(2.0f);
    effect->velocity.z = RANDOM2_F(2.0f);

    effect->acceleration.x = 0.0f;
    effect->acceleration.y = -0.1f;
    effect->acceleration.z = 0.0f;

    effect->effect_specific[0] = 0;
    effect->offset.x = effect->scale.x;
}

/* eResetbreak_Dust_mv: 72B */
static void eResetbreak_Dust_mv(eEC_Effect_c* effect, GAME* game) {
    xyz_t_add(&effect->velocity, &effect->acceleration, &effect->velocity);
    xyz_t_add(&effect->position, &effect->velocity, &effect->position);

    effect->velocity.x *= sqrtf(0.9f);
    effect->velocity.z *= sqrtf(0.9f);
}

/* eResetbreak_Dust_dw: 284B */
static void eResetbreak_Dust_dw(eEC_Effect_c* effect, GAME* game) {
    s16 counter = (24 - effect->timer) >> 1;
    u8 alpha;
    int tex_idx;

    counter = CLAMP(counter, 0, 11);
    tex_idx = CLAMP(counter >> 2, 0, 3);

    if (counter < 8) {
        alpha = 0xFF;
    } else {
        alpha = (u8)eEC_CLIP->calc_adjust_proc(counter, 8, 11, 255.0f, 0.0f);
    }

    effect->scale.x = eEC_CLIP->calc_adjust_proc(24 - effect->timer, 0, 24, effect->offset.x, effect->offset.x * 1.5f);
    effect->scale.y = effect->scale.x;
    effect->scale.z = effect->scale.x;

    OPEN_DISP(game->graph);

    eEC_CLIP->auto_matrix_xlu_proc(game, &effect->position, &effect->scale);
    gSPSegment(NEXT_POLY_XLU_DISP, ANIME_1_TXT_SEG, eResetbreak_Dust_tex_table[tex_idx]);
    gSPSegment(NEXT_POLY_XLU_DISP, ANIME_2_TXT_SEG, eResetbreak_Dust_tex_table[tex_idx]);
    gDPSetPrimColor(NEXT_POLY_XLU_DISP, 0, 128, 200, 180, 150, alpha);
    gSPDisplayList(NEXT_POLY_XLU_DISP, ef_dust01_modelT);

    CLOSE_DISP(game->graph);
}
#endif /* VERSION >= VER_DELUXE */
