#include "m_ordinance_ovl.h"

#include "m_ordinance.h"
#include "m_common_data.h"
#include "m_font.h"
#include "m_lib.h"
#include "sys_matrix.h"
#include "audio.h"

static mOD_Ovl_c od_ovl_data;

/* Time shift cycle: None -> Early Bird -> Night Owl -> None */
static u8 mOD_title_str[11] = "Ordinances";

static u8 mOD_time_shift_str[11] = "Time Shift";
static u8 mOD_bell_boom_str[10] = "Bell Boom";
static u8 mOD_beautiful_str[15] = "Beautiful Town";

static u8 mOD_off_str[4] = "Off";
static u8 mOD_on_str[3] = "On";
static u8 mOD_none_str[5] = "None";
static u8 mOD_early_str[11] = "Early Bird";
static u8 mOD_late_str[10] = "Night Owl";

static void mOD_move_Move(Submenu* submenu, mSM_MenuInfo_c* menu_info) {
    (*submenu->overlay->move_Move_proc)(submenu, menu_info);
}

static void mOD_move_Play(Submenu* submenu, mSM_MenuInfo_c* menu_info) {
    mOD_Ovl_c* od_ovl = submenu->overlay->ordinance_ovl;
    u32 trigger = submenu->overlay->menu_control.trigger;
    int cursor = od_ovl->cursor_idx;

    if (trigger & BUTTON_B) {
        /* Cancel - restore all initial values */
        mOR_SetTimeShift(od_ovl->initial_time_shift);
        mOR_SetBellBoom(od_ovl->initial_bell_boom);
        mOR_SetBeautiful(od_ovl->initial_beautiful);
        (*submenu->overlay->move_chg_base_proc)(menu_info, mSM_MOVE_OUT_TOP);
        sAdo_SysTrgStart(NA_SE_MENU_EXIT);
    } else if (trigger & BUTTON_A) {
        /* Confirm current settings and exit */
        (*submenu->overlay->move_chg_base_proc)(menu_info, mSM_MOVE_OUT_TOP);
        sAdo_SysTrgStart(NA_SE_MENU_EXIT);
    } else if (trigger & BUTTON_CUP) {
        if (cursor > 0) {
            od_ovl->cursor_idx--;
            sAdo_SysTrgStart(NA_SE_CURSOL);
        }
    } else if (trigger & BUTTON_CDOWN) {
        if (cursor < mOD_CURSOR_NUM - 1) {
            od_ovl->cursor_idx++;
            sAdo_SysTrgStart(NA_SE_CURSOL);
        }
    } else if ((trigger & BUTTON_CLEFT) || (trigger & BUTTON_CRIGHT)) {
        /* Toggle/cycle the selected ordinance */
        switch (cursor) {
            case mOD_CURSOR_TIME_SHIFT: {
                int ts = mOR_GetTimeShift();
                if (trigger & BUTTON_CRIGHT) {
                    ts++;
                    if (ts > mOR_TIME_SHIFT_MAX) {
                        ts = mOR_TIME_SHIFT_NONE;
                    }
                } else {
                    ts--;
                    if (ts < mOR_TIME_SHIFT_NONE) {
                        ts = mOR_TIME_SHIFT_MAX;
                    }
                }
                mOR_SetTimeShift(ts);
                sAdo_SysTrgStart(NA_SE_CURSOL);
                break;
            }
            case mOD_CURSOR_BELL_BOOM: {
                mOR_SetBellBoom(!mOR_GetBellBoom());
                sAdo_SysTrgStart(NA_SE_CURSOL);
                break;
            }
            case mOD_CURSOR_BEAUTIFUL: {
                mOR_SetBeautiful(!mOR_GetBeautiful());
                sAdo_SysTrgStart(NA_SE_CURSOL);
                break;
            }
        }
    }
}

static void mOD_move_End(Submenu* submenu, mSM_MenuInfo_c* menu_info) {
    (*submenu->overlay->move_End_proc)(submenu, menu_info);
}

typedef void (*mOD_MOVE_PROC)(Submenu*, mSM_MenuInfo_c*);

static void mOD_ordinance_ovl_move(Submenu* submenu) {
    static mOD_MOVE_PROC ovl_move_proc[mSM_OVL_PROC_NUM] = {
        &mOD_move_Move,
        &mOD_move_Play,
        (mOD_MOVE_PROC)none_proc1,
        (mOD_MOVE_PROC)none_proc1,
        &mOD_move_End
    };

    mSM_MenuInfo_c* menu_info = &submenu->overlay->menu_info[mSM_OVL_ORDINANCE];

    (*menu_info->pre_move_func)(submenu);
    (*ovl_move_proc[menu_info->proc_status])(submenu, menu_info);
}

extern Gfx att_win_model[];

static void mOD_set_frame_dl(Submenu* submenu, GRAPH* graph, mSM_MenuInfo_c* menu_info, f32 x, f32 y) {
    Gfx* gfx;

    Matrix_scale(16.0f, 16.0f, 1.0f, MTX_LOAD);
    Matrix_translate(x, y, 140.0f, MTX_MULT);

    OPEN_DISP(graph);
    gfx = NOW_POLY_OPA_DISP;

    gDPPipeSync(gfx++);
    gDPSetBlendColor(gfx++, 255, 255, 255, 40);
    gSPMatrix(gfx++, _Matrix_to_Mtx_new(graph), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    gSPDisplayList(gfx++, att_win_model);
    gDPPipeSync(gfx++);
    gDPSetAlphaCompare(gfx++, G_AC_NONE);
    gDPSetBlendColor(gfx++, 255, 255, 255, 8);

    SET_POLY_OPA_DISP(gfx);
    CLOSE_DISP(graph);
}

static void mOD_set_character_dl(Submenu* submenu, GAME* game, f32 x, f32 y) {
    static rgba_t title_col   = { 255, 255, 255, 255 };
    static rgba_t normal_col  = {  70, 120, 245, 255 };
    static rgba_t select_col  = { 215,   0,   0, 255 };
    static rgba_t value_col   = {  40, 185, 110, 255 };

    mOD_Ovl_c* od_ovl = submenu->overlay->ordinance_ovl;
    rgba_t* label_color;
    f32 row_y;
    int time_shift;
    u8* value_str;
    int value_len;

    (*submenu->overlay->set_char_matrix_proc)(game->graph);

    /* Title */
    mFont_SetLineStrings(
        game,
        mOD_title_str, 10,
        x + 144.0f, -y + 58.0f,
        title_col.r, title_col.g, title_col.b, 255,
        FALSE, TRUE,
        0.875f, 0.875f,
        mFont_MODE_POLY
    );

    /* Row 0: Time Shift */
    row_y = -y + 78.0f;
    label_color = (od_ovl->cursor_idx == mOD_CURSOR_TIME_SHIFT) ? &select_col : &normal_col;
    mFont_SetLineStrings(
        game,
        mOD_time_shift_str, 10,
        x + 148.0f, row_y,
        label_color->r, label_color->g, label_color->b, 255,
        FALSE, TRUE,
        0.75f, 0.75f,
        mFont_MODE_POLY
    );

    time_shift = mOR_GetTimeShift();
    if (time_shift == mOR_TIME_SHIFT_EARLY) {
        value_str = mOD_early_str;
        value_len = 10;
    } else if (time_shift == mOR_TIME_SHIFT_LATE) {
        value_str = mOD_late_str;
        value_len = 9;
    } else {
        value_str = mOD_none_str;
        value_len = 4;
    }

    mFont_SetLineStrings(
        game,
        value_str, value_len,
        x + 230.0f, row_y,
        value_col.r, value_col.g, value_col.b, 255,
        FALSE, TRUE,
        0.75f, 0.75f,
        mFont_MODE_POLY
    );

    /* Row 1: Bell Boom */
    row_y = -y + 94.0f;
    label_color = (od_ovl->cursor_idx == mOD_CURSOR_BELL_BOOM) ? &select_col : &normal_col;
    mFont_SetLineStrings(
        game,
        mOD_bell_boom_str, 9,
        x + 148.0f, row_y,
        label_color->r, label_color->g, label_color->b, 255,
        FALSE, TRUE,
        0.75f, 0.75f,
        mFont_MODE_POLY
    );

    mFont_SetLineStrings(
        game,
        mOR_GetBellBoom() ? mOD_on_str : mOD_off_str,
        mOR_GetBellBoom() ? 2 : 3,
        x + 230.0f, row_y,
        value_col.r, value_col.g, value_col.b, 255,
        FALSE, TRUE,
        0.75f, 0.75f,
        mFont_MODE_POLY
    );

    /* Row 2: Beautiful Town */
    row_y = -y + 110.0f;
    label_color = (od_ovl->cursor_idx == mOD_CURSOR_BEAUTIFUL) ? &select_col : &normal_col;
    mFont_SetLineStrings(
        game,
        mOD_beautiful_str, 14,
        x + 148.0f, row_y,
        label_color->r, label_color->g, label_color->b, 255,
        FALSE, TRUE,
        0.75f, 0.75f,
        mFont_MODE_POLY
    );

    mFont_SetLineStrings(
        game,
        mOR_GetBeautiful() ? mOD_on_str : mOD_off_str,
        mOR_GetBeautiful() ? 2 : 3,
        x + 230.0f, row_y,
        value_col.r, value_col.g, value_col.b, 255,
        FALSE, TRUE,
        0.75f, 0.75f,
        mFont_MODE_POLY
    );
}

static void mOD_set_dl(Submenu* submenu, GAME* game, mSM_MenuInfo_c* menu_info) {
    GRAPH* graph = game->graph;
    f32 x = menu_info->position[0];
    f32 y = menu_info->position[1];

    mOD_set_frame_dl(submenu, graph, menu_info, x, y);
    mOD_set_character_dl(submenu, game, x, y);
}

static void mOD_ordinance_ovl_draw(Submenu* submenu, GAME* game) {
    mSM_MenuInfo_c* menu_info = &submenu->overlay->menu_info[mSM_OVL_ORDINANCE];

    (*menu_info->pre_draw_func)(submenu, game);
    mOD_set_dl(submenu, game, menu_info);
}

extern void mOD_ordinance_ovl_set_proc(Submenu* submenu) {
    mSM_Control_c* control = &submenu->overlay->menu_control;

    control->menu_move_func = &mOD_ordinance_ovl_move;
    control->menu_draw_func = &mOD_ordinance_ovl_draw;
}

static void mOD_ordinance_ovl_init(Submenu* submenu) {
    mSM_MenuInfo_c* menu_info = &submenu->overlay->menu_info[mSM_OVL_ORDINANCE];
    mOD_Ovl_c* od_ovl = submenu->overlay->ordinance_ovl;

    submenu->overlay->menu_control.animation_flag = FALSE;
    menu_info->proc_status = mSM_OVL_PROC_MOVE;
    menu_info->next_proc_status = mSM_OVL_PROC_PLAY;
    menu_info->move_drt = mSM_MOVE_IN_TOP;

    /* Save initial values for cancel restoration */
    od_ovl->initial_time_shift = mOR_GetTimeShift();
    od_ovl->initial_bell_boom = mOR_GetBellBoom();
    od_ovl->initial_beautiful = mOR_GetBeautiful();
    od_ovl->cursor_idx = 0;
}

extern void mOD_ordinance_ovl_construct(Submenu* submenu) {
    Submenu_Overlay_c* overlay = submenu->overlay;

    if (overlay->ordinance_ovl == NULL) {
        mem_clear((u8*)&od_ovl_data, sizeof(mOD_Ovl_c), 0);
        overlay->ordinance_ovl = &od_ovl_data;
    }

    mOD_ordinance_ovl_init(submenu);
    mOD_ordinance_ovl_set_proc(submenu);
}

extern void mOD_ordinance_ovl_destruct(Submenu* submenu) {
    submenu->overlay->ordinance_ovl = NULL;
}
