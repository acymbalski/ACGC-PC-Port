#include "types.h"
#if VERSION >= VER_DELUXE
#include "ef_effect_control.h"

#include "m_common_data.h"
#include "m_rcp.h"
#include "sys_matrix.h"

static void eResetbreak_Piece_init(xyz_t pos, int prio, s16 angle, GAME* game, u16 item_name, s16 arg0, s16 arg1);
static void eResetbreak_Piece_ct(eEC_Effect_c* effect, GAME* game, void* ct_arg);
static void eResetbreak_Piece_mv(eEC_Effect_c* effect, GAME* game);
static void eResetbreak_Piece_dw(eEC_Effect_c* effect, GAME* game);

eEC_PROFILE_c iam_ef_resetbreak_piece = {
    // clang-format off
    &eResetbreak_Piece_init,
    &eResetbreak_Piece_ct,
    &eResetbreak_Piece_mv,
    &eResetbreak_Piece_dw,
    eEC_IGNORE_DEATH,
    eEC_NO_CHILD_ID,
    eEC_DEFAULT_DEATH_DIST,
    // clang-format on
};

extern u8 ef_dust01_0[];
extern u8 ef_dust01_1[];
extern Gfx ef_dust01_modelT[];

/* eResetbreak_Piece_init: 128B */
static void eResetbreak_Piece_init(xyz_t pos, int prio, s16 angle, GAME* game, u16 item_name, s16 arg0, s16 arg1) {
    eEC_CLIP->make_effect_proc(eEC_EFFECT_RESETBREAK_PIECE, pos, NULL, game, NULL, item_name, prio, arg0, arg1);
}

/* eResetbreak_Piece_ct: 548B */
static void eResetbreak_Piece_ct(eEC_Effect_c* effect, GAME* game, void* ct_arg) {
    effect->scale.x = 0.006f + RANDOM_F(0.004f);
    effect->scale.y = effect->scale.x;
    effect->scale.z = effect->scale.x;

    effect->timer = 50;

    effect->velocity.x = RANDOM2_F(4.0f);
    effect->velocity.y = 4.0f + RANDOM_F(3.0f);
    effect->velocity.z = RANDOM2_F(4.0f);

    effect->acceleration.x = 0.0f;
    effect->acceleration.y = -0.25f;
    effect->acceleration.z = 0.0f;

    effect->effect_specific[0] = (s16)(RANDOM_F(65535.0f));
    effect->effect_specific[1] = (s16)(RANDOM_F(2000.0f)) + 500;
    effect->effect_specific[2] = 0;
    effect->effect_specific[3] = 0;

    effect->offset.x = effect->scale.x;
    effect->offset.y = effect->position.y;
    effect->offset.z = 0.0f;
}

/* eResetbreak_Piece_mv: 420B */
static void eResetbreak_Piece_mv(eEC_Effect_c* effect, GAME* game) {
    s16 counter = 50 - effect->timer;

    xyz_t_add(&effect->velocity, &effect->acceleration, &effect->velocity);
    xyz_t_add(&effect->position, &effect->velocity, &effect->position);

    effect->velocity.x *= sqrtf(0.95f);
    effect->velocity.z *= sqrtf(0.95f);

    effect->effect_specific[0] += effect->effect_specific[1];

    /* Ground bounce */
    if (effect->position.y < effect->offset.y && effect->effect_specific[2] < 2) {
        effect->position.y = effect->offset.y;
        effect->velocity.y = -effect->velocity.y * 0.3f;
        effect->effect_specific[2]++;
    }

    /* Fade scale near end */
    if (counter > 40) {
        effect->scale.x = eEC_CLIP->calc_adjust_proc(counter, 40, 50, effect->offset.x, 0.0f);
        effect->scale.y = effect->scale.x;
        effect->scale.z = effect->scale.x;
    }
}

/* eResetbreak_Piece_dw: 312B */
static void eResetbreak_Piece_dw(eEC_Effect_c* effect, GAME* game) {
    s16 counter = 50 - effect->timer;
    u8 alpha;

    counter = CLAMP(counter, 0, 49);

    if (counter > 40) {
        alpha = (u8)eEC_CLIP->calc_adjust_proc(counter, 40, 49, 255.0f, 0.0f);
    } else {
        alpha = 0xFF;
    }

    OPEN_DISP(game->graph);

    eEC_CLIP->auto_matrix_xlu_proc(game, &effect->position, &effect->scale);
    gSPSegment(NEXT_POLY_XLU_DISP, ANIME_1_TXT_SEG, ef_dust01_0);
    gSPSegment(NEXT_POLY_XLU_DISP, ANIME_2_TXT_SEG, ef_dust01_1);
    gDPSetPrimColor(NEXT_POLY_XLU_DISP, 0, 128, 160, 140, 120, alpha);
    gSPDisplayList(NEXT_POLY_XLU_DISP, ef_dust01_modelT);

    CLOSE_DISP(game->graph);
}
#endif /* VERSION >= VER_DELUXE */
