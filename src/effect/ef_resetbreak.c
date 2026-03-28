#include "types.h"
#if VERSION >= VER_DELUXE
#include "ef_effect_control.h"

#include "m_common_data.h"
#include "m_rcp.h"
#include "sys_matrix.h"

static void eResetbreak_init(xyz_t pos, int prio, s16 angle, GAME* game, u16 item_name, s16 arg0, s16 arg1);
static void eResetbreak_ct(eEC_Effect_c* effect, GAME* game, void* ct_arg);
static void eResetbreak_mv(eEC_Effect_c* effect, GAME* game);
static void eResetbreak_dw(eEC_Effect_c* effect, GAME* game);

eEC_PROFILE_c iam_ef_resetbreak = {
    // clang-format off
    &eResetbreak_init,
    &eResetbreak_ct,
    &eResetbreak_mv,
    &eResetbreak_dw,
    eEC_IGNORE_DEATH,
    eEC_NO_CHILD_ID,
    eEC_IGNORE_DEATH_DIST,
    // clang-format on
};

/* aResetbreak_make_dust: 308B - creates dust sub-effects */
static void aResetbreak_make_dust(eEC_Effect_c* effect, GAME* game, int count) {
    int i;
    xyz_t pos;

    for (i = 0; i < count; i++) {
        pos = effect->position;
        pos.x += RANDOM2_F(20.0f);
        pos.y += RANDOM_F(5.0f);
        pos.z += RANDOM2_F(20.0f);
        eEC_CLIP->effect_make_proc(eEC_EFFECT_RESETBREAK_DUST, pos, effect->prio, 0, game, effect->item_name, 0, 0);
    }
}

/* aResetbreak_make_rock_parts: 328B - creates rock part sub-effects */
static void aResetbreak_make_rock_parts(eEC_Effect_c* effect, GAME* game) {
    int i;
    xyz_t pos;

    for (i = 0; i < 5; i++) {
        pos = effect->position;
        pos.x += RANDOM2_F(15.0f);
        pos.y += RANDOM_F(10.0f);
        pos.z += RANDOM2_F(15.0f);
        eEC_CLIP->effect_make_proc(eEC_EFFECT_RESETBREAK_PARTS, pos, effect->prio, 0, game, effect->item_name, i % 3, 0);
    }
}

/* aResetbreak_make_rock_piece: 204B - creates rock piece sub-effects */
static void aResetbreak_make_rock_piece(eEC_Effect_c* effect, GAME* game) {
    int i;
    xyz_t pos;

    for (i = 0; i < 3; i++) {
        pos = effect->position;
        pos.x += RANDOM2_F(10.0f);
        pos.y += RANDOM_F(8.0f);
        pos.z += RANDOM2_F(10.0f);
        eEC_CLIP->effect_make_proc(eEC_EFFECT_RESETBREAK_PIECE, pos, effect->prio, 0, game, effect->item_name, 0, 0);
    }
}

/* eResetbreak_init: 124B */
static void eResetbreak_init(xyz_t pos, int prio, s16 angle, GAME* game, u16 item_name, s16 arg0, s16 arg1) {
    eEC_CLIP->make_effect_proc(eEC_EFFECT_RESETBREAK, pos, NULL, game, NULL, item_name, prio, arg0, arg1);
}

/* eResetbreak_ct: 156B - reset screen break constructor */
static void eResetbreak_ct(eEC_Effect_c* effect, GAME* game, void* ct_arg) {
    effect->timer = 60;
    effect->scale = ZeroVec;
    effect->velocity = ZeroVec;
    effect->acceleration = ZeroVec;
    effect->effect_specific[0] = 0;
    effect->effect_specific[1] = 0;
}

/* eResetbreak_mv: 200B - reset screen break move/update */
static void eResetbreak_mv(eEC_Effect_c* effect, GAME* game) {
    s16 counter = 60 - effect->timer;

    if (counter == 0) {
        aResetbreak_make_rock_parts(effect, game);
        aResetbreak_make_rock_piece(effect, game);
        aResetbreak_make_dust(effect, game, 8);
    } else if (counter == 10) {
        aResetbreak_make_dust(effect, game, 4);
    } else if (counter == 20) {
        aResetbreak_make_dust(effect, game, 2);
    }

    effect->effect_specific[0] = counter;
}

/* eResetbreak_dw: 4B - just return (blr) */
static void eResetbreak_dw(eEC_Effect_c* effect, GAME* game) {
    return;
}
#endif /* VERSION >= VER_DELUXE */
