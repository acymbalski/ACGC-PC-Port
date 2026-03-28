#include "types.h"
#if VERSION >= VER_DELUXE
#include "ef_effect_control.h"

#include "m_common_data.h"
#include "m_rcp.h"
#include "sys_matrix.h"

static void eSC_init(xyz_t pos, int prio, s16 angle, GAME* game, u16 item_name, s16 arg0, s16 arg1);
static void eSC_ct(eEC_Effect_c* effect, GAME* game, void* ct_arg);
static void eSC_mv(eEC_Effect_c* effect, GAME* game);
static void eSC_dw(eEC_Effect_c* effect, GAME* game);

eEC_PROFILE_c iam_ef_screen = {
    // clang-format off
    &eSC_init,
    &eSC_ct,
    &eSC_mv,
    &eSC_dw,
    eEC_IGNORE_DEATH,
    eEC_NO_CHILD_ID,
    eEC_IGNORE_DEATH_DIST,
    // clang-format on
};

typedef struct {
    s16 screen_type;
    s16 duration;
    rgba_t color;
} eSC_data_c;

/* eSC_init: 124B */
static void eSC_init(xyz_t pos, int prio, s16 angle, GAME* game, u16 item_name, s16 arg0, s16 arg1) {
    eSC_data_c data;

    data.screen_type = arg0;
    data.duration = arg1;
    data.color.r = 0;
    data.color.g = 0;
    data.color.b = 0;
    data.color.a = 255;

    eEC_CLIP->make_effect_proc(eEC_EFFECT_SCREEN, pos, NULL, game, &data, item_name, prio, arg0, arg1);
}

/* eSC_ct: 60B - screen effect constructor */
static void eSC_ct(eEC_Effect_c* effect, GAME* game, void* ct_arg) {
    eSC_data_c* data = (eSC_data_c*)ct_arg;

    effect->timer = data->duration;
    effect->effect_specific[0] = data->screen_type;
}

/* eSC_mv: 4B - just return (blr) */
static void eSC_mv(eEC_Effect_c* effect, GAME* game) {
    return;
}

/* eSC_dw: 252B - screen effect draw overlay */
static void eSC_dw(eEC_Effect_c* effect, GAME* game) {
    s16 elapsed = effect->arg1 - effect->timer;
    f32 alpha_f;
    u8 alpha;
    s16 half_time;

    half_time = effect->arg1 >> 1;

    if (elapsed < half_time) {
        alpha_f = eEC_CLIP->calc_adjust_proc(elapsed, 0, half_time, 0.0f, 255.0f);
    } else {
        alpha_f = eEC_CLIP->calc_adjust_proc(elapsed, half_time, effect->arg1, 255.0f, 0.0f);
    }

    alpha = (u8)alpha_f;

    OPEN_DISP(game->graph);

    gDPPipeSync(NEXT_OVERLAY_DISP);
    gDPSetCycleType(NEXT_OVERLAY_DISP, G_CYC_FILL);
    gDPSetRenderMode(NEXT_OVERLAY_DISP, G_RM_NOOP, G_RM_NOOP2);
    gDPSetFillColor(NEXT_OVERLAY_DISP, (GPACK_RGBA5551(0, 0, 0, 1) << 16) | GPACK_RGBA5551(0, 0, 0, 1));
    gDPFillRectangle(NEXT_OVERLAY_DISP, 0, 0, 319, 239);
    gDPPipeSync(NEXT_OVERLAY_DISP);

    CLOSE_DISP(game->graph);
}
#endif /* VERSION >= VER_DELUXE */
