#include "m_skintone_ovl.h"

#if VERSION >= VER_DELUXE

#include "m_common_data.h"
#include "m_font.h"
#include "m_player_lib.h"
#include "m_private.h"
#include "m_lib.h"
#include "sys_matrix.h"
#include "audio.h"
#include "m_rcp.h"

static mST_Ovl_c st_ovl_data;

/* 8 skin tone presets mapped to sunburn rank values 0-7 */
static int mST_rank_values[mST_TONE_NUM] = { 0, 1, 2, 3, 4, 5, 6, 8 };

/* RGB colors for each skin tone swatch (light to dark) */
static u8 mST_tone_colors[mST_TONE_NUM][3] = {
    { 255, 224, 195 },  /* 0: lightest */
    { 245, 210, 180 },  /* 1: light */
    { 235, 195, 160 },  /* 2: light-medium */
    { 220, 180, 145 },  /* 3: medium */
    { 200, 160, 125 },  /* 4: medium-tan */
    { 180, 138, 105 },  /* 5: tan */
    { 155, 112,  80 },  /* 6: dark-tan */
    { 130,  90,  60 },  /* 7: dark */
};

/* Needlework cloud-bubble display lists */
extern Gfx needlework_before_model[];
extern Gfx inv_original_w_model_before[];
extern Gfx inv_original_w1T_model[];
extern Gfx inv_original_w2T_model[];
extern Gfx inv_original_w3T_model[];
extern Gfx inv_original_w4T_model[];
extern Gfx inv_original_w5T_model[];
extern Gfx inv_original_w6T_model[];
extern Gfx inv_original_w7T_model[];
extern Gfx inv_original_w8T_model[];
extern Gfx inv_original_w9_model[];
extern Gfx inv_original_ueT_model[];
extern Gfx inv_original_waku_model[];
extern Gfx inv_original_mb_before_model[];
extern Gfx inv_original_f_model[];
extern Gfx inv_original_mb1_model[];
extern Gfx inv_original_mb2_model[];
extern Gfx inv_original_mb3_model[];
extern Gfx inv_original_mb4_model[];
extern Gfx inv_original_mb5_model[];
extern Gfx inv_original_mb6_model[];
extern Gfx inv_original_mb7_model[];
extern Gfx inv_original_mb8_model[];

static void mST_move_Move(Submenu* submenu, mSM_MenuInfo_c* menu_info) {
    (*submenu->overlay->move_Move_proc)(submenu, menu_info);
}

static void mST_move_Wait(Submenu* submenu, mSM_MenuInfo_c* menu_info) {
    mST_Ovl_c* st_ovl = submenu->overlay->skintone_ovl;

    if (st_ovl->cursor_idx >= 0 && st_ovl->cursor_idx < mST_TONE_NUM) {
        menu_info->proc_status = mSM_OVL_PROC_PLAY;
    }
}

static void mST_apply_preview(mST_Ovl_c* st_ovl) {
    if (Now_Private != NULL) {
        Now_Private->sunburn.rank = mST_rank_values[st_ovl->cursor_idx];
        mPlib_Set_change_color_request();
    }
}

static void mST_move_Play(Submenu* submenu, mSM_MenuInfo_c* menu_info) {
    mST_Ovl_c* st_ovl = submenu->overlay->skintone_ovl;
    u32 trigger = submenu->overlay->menu_control.trigger;
    int cursor = st_ovl->cursor_idx;
    int col = cursor % mST_TONE_COLS;
    int row = cursor / mST_TONE_COLS;

    if (trigger & BUTTON_B) {
        /* Cancel - restore initial rank */
        if (Now_Private != NULL) {
            Now_Private->sunburn.rank = st_ovl->initial_rank;
            mPlib_Set_change_color_request();
        }
        (*submenu->overlay->move_chg_base_proc)(menu_info, mSM_MOVE_OUT_TOP);
        sAdo_SysTrgStart(NA_SE_MENU_EXIT);
    } else if (trigger & BUTTON_A) {
        /* Confirm current selection */
        if (cursor >= 0 && cursor < mST_TONE_NUM && Now_Private != NULL) {
            Now_Private->sunburn.rank = mST_rank_values[cursor];
            g_dlx_skin_tone_locked = TRUE;
            mPlib_Set_change_color_request();
        }
        (*submenu->overlay->move_chg_base_proc)(menu_info, mSM_MOVE_OUT_TOP);
        sAdo_SysTrgStart(NA_SE_MENU_EXIT);
    } else if (trigger & (BUTTON_CUP | BUTTON_DUP)) {
        /* Up: move to previous row */
        if (row > 0) {
            st_ovl->cursor_idx = (row - 1) * mST_TONE_COLS + col;
            mST_apply_preview(st_ovl);
            sAdo_SysTrgStart(NA_SE_CURSOL);
        }
    } else if (trigger & (BUTTON_CDOWN | BUTTON_DDOWN)) {
        /* Down: move to next row */
        if (row < mST_TONE_ROWS - 1) {
            st_ovl->cursor_idx = (row + 1) * mST_TONE_COLS + col;
            mST_apply_preview(st_ovl);
            sAdo_SysTrgStart(NA_SE_CURSOL);
        }
    } else if (trigger & (BUTTON_CLEFT | BUTTON_DLEFT)) {
        /* Left: move to left column */
        if (col > 0) {
            st_ovl->cursor_idx = row * mST_TONE_COLS + (col - 1);
            mST_apply_preview(st_ovl);
            sAdo_SysTrgStart(NA_SE_CURSOL);
        }
    } else if (trigger & (BUTTON_CRIGHT | BUTTON_DRIGHT)) {
        /* Right: move to right column */
        if (col < mST_TONE_COLS - 1) {
            st_ovl->cursor_idx = row * mST_TONE_COLS + (col + 1);
            mST_apply_preview(st_ovl);
            sAdo_SysTrgStart(NA_SE_CURSOL);
        }
    }
}

static void mST_move_End(Submenu* submenu, mSM_MenuInfo_c* menu_info) {
    (*submenu->overlay->move_End_proc)(submenu, menu_info);
}

typedef void (*mST_MOVE_PROC)(Submenu*, mSM_MenuInfo_c*);

static void mST_skintone_ovl_move(Submenu* submenu) {
    static mST_MOVE_PROC ovl_move_proc[mSM_OVL_PROC_NUM] = {
        &mST_move_Move,
        &mST_move_Play,
        (mST_MOVE_PROC)none_proc1,
        (mST_MOVE_PROC)none_proc1,
        &mST_move_End
    };

    mSM_MenuInfo_c* menu_info = &submenu->overlay->menu_info[mSM_OVL_SKINTONE];

    (*menu_info->pre_move_func)(submenu);
    (*ovl_move_proc[menu_info->proc_status])(submenu, menu_info);
}

static void mST_set_frame_dl(Submenu* submenu, GRAPH* graph, mSM_MenuInfo_c* menu_info, f32 x, f32 y) {
    /*
     * Needlework-style cloud bubble with 8 solid-colored skin tone boxes.
     * Box layout matches inv_original_mb1-8 grid (2 cols x 4 rows):
     *   mb1=left col row0 (x=-88), mb2=right col row0 (x=-56)
     *   mb3=left col row1,         mb4=right col row1
     *   mb5=left col row2,         mb6=right col row2
     *   mb7=left col row3,         mb8=right col row3
     *
     * Cursor indices (left-to-right, top-to-bottom) map directly:
     *   cursor 0 (row0,col0) -> mb1 (left,row0)
     *   cursor 1 (row0,col1) -> mb2 (right,row0)
     *   cursor 2 (row1,col0) -> mb3
     *   cursor 3 (row1,col1) -> mb4
     *   etc.
     */
    static Gfx* gfx_table[mST_TONE_NUM] = {
        inv_original_mb1_model, inv_original_mb2_model,
        inv_original_mb3_model, inv_original_mb4_model,
        inv_original_mb5_model, inv_original_mb6_model,
        inv_original_mb7_model, inv_original_mb8_model,
    };
    int tex_x;
    int tex_y;
    int i;
    mST_Ovl_c* st_ovl = submenu->overlay->skintone_ovl;

    Matrix_scale(16.0f, 16.0f, 1.0f, MTX_LOAD);
    Matrix_translate(x, y, 140.0f, MTX_MULT);

    OPEN_POLY_OPA_DISP(graph);

    gSPMatrix(POLY_OPA_DISP++, _Matrix_to_Mtx_new(graph), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    gSPDisplayList(POLY_OPA_DISP++, needlework_before_model);

    /* Background pattern tiles */
    tex_x = -submenu->overlay->menu_control.texture_pos[0] * 4.0f;
    tex_y = -submenu->overlay->menu_control.texture_pos[1] * 4.0f;
    gSPDisplayList(POLY_OPA_DISP++, inv_original_w_model_before);
    gDPSetTileSize_Dolphin(POLY_OPA_DISP++, G_TX_RENDERTILE, tex_x, tex_y, 32, 32);
    gSPDisplayList(POLY_OPA_DISP++, inv_original_w1T_model);
    gSPDisplayList(POLY_OPA_DISP++, inv_original_w2T_model);
    gSPDisplayList(POLY_OPA_DISP++, inv_original_w3T_model);
    gSPDisplayList(POLY_OPA_DISP++, inv_original_w4T_model);
    gSPDisplayList(POLY_OPA_DISP++, inv_original_w5T_model);
    gSPDisplayList(POLY_OPA_DISP++, inv_original_w6T_model);
    gSPDisplayList(POLY_OPA_DISP++, inv_original_w7T_model);
    gSPDisplayList(POLY_OPA_DISP++, inv_original_w8T_model);
    gSPDisplayList(POLY_OPA_DISP++, inv_original_w9_model);
    gSPDisplayList(POLY_OPA_DISP++, inv_original_ueT_model);
    gSPDisplayList(POLY_OPA_DISP++, inv_original_waku_model);

    /* Set up box rendering - override texture combine to use solid PRIMITIVE color */
    gSPDisplayList(POLY_OPA_DISP++, inv_original_mb_before_model);
    gDPSetCombineMode(POLY_OPA_DISP++, G_CC_PRIMITIVE, G_CC_PRIMITIVE);

    /* Draw 8 skin tone color boxes */
    for (i = 0; i < mST_TONE_NUM; i++) {
        u8 r = mST_tone_colors[i][0];
        u8 g = mST_tone_colors[i][1];
        u8 b = mST_tone_colors[i][2];

        /* Brighten selected box slightly, darken unselected */
        if (st_ovl != NULL && st_ovl->cursor_idx == i) {
            /* Selected: draw at full brightness with white border effect */
            gDPSetPrimColor(POLY_OPA_DISP++, 0, 0,
                            r < 235 ? r + 20 : 255,
                            g < 235 ? g + 20 : 255,
                            b < 235 ? b + 20 : 255, 255);
        } else {
            gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, r, g, b, 255);
        }
        gSPDisplayList(POLY_OPA_DISP++, gfx_table[i]);
    }

    /* Front frame overlay */
    gSPDisplayList(POLY_OPA_DISP++, inv_original_f_model);

    CLOSE_POLY_OPA_DISP(graph);
}

static void mST_set_dl(Submenu* submenu, GAME* game, mSM_MenuInfo_c* menu_info) {
    GRAPH* graph = game->graph;
    f32 x = menu_info->position[0];
    f32 y = menu_info->position[1];

    mST_set_frame_dl(submenu, graph, menu_info, x, y);
}

static void mST_skintone_ovl_draw(Submenu* submenu, GAME* game) {
    mSM_MenuInfo_c* menu_info = &submenu->overlay->menu_info[mSM_OVL_SKINTONE];

    (*menu_info->pre_draw_func)(submenu, game);
    mST_set_dl(submenu, game, menu_info);
}

extern void mST_skintone_ovl_set_proc(Submenu* submenu) {
    mSM_Control_c* control = &submenu->overlay->menu_control;

    control->menu_move_func = &mST_skintone_ovl_move;
    control->menu_draw_func = &mST_skintone_ovl_draw;
}

static void mST_skintone_ovl_init(Submenu* submenu) {
    mSM_MenuInfo_c* menu_info = &submenu->overlay->menu_info[mSM_OVL_SKINTONE];
    mST_Ovl_c* st_ovl = submenu->overlay->skintone_ovl;
    int i;

    submenu->overlay->menu_control.animation_flag = FALSE;
    menu_info->proc_status = mSM_OVL_PROC_MOVE;
    menu_info->next_proc_status = mSM_OVL_PROC_PLAY;
    menu_info->move_drt = mSM_MOVE_IN_TOP;

    /* Save initial rank for cancel restoration */
    st_ovl->initial_rank = Now_Private != NULL ? Now_Private->sunburn.rank : 0;

    /* Set cursor to nearest matching preset */
    st_ovl->cursor_idx = 0;
    for (i = 0; i < mST_TONE_NUM; i++) {
        if (mST_rank_values[i] == st_ovl->initial_rank) {
            st_ovl->cursor_idx = i;
            break;
        }
    }
}

extern void mST_skintone_ovl_construct(Submenu* submenu) {
    Submenu_Overlay_c* overlay = submenu->overlay;

    if (overlay->skintone_ovl == NULL) {
        mem_clear((u8*)&st_ovl_data, sizeof(mST_Ovl_c), 0);
        overlay->skintone_ovl = &st_ovl_data;
    }

    mST_skintone_ovl_init(submenu);
    mST_skintone_ovl_set_proc(submenu);
}

extern void mST_skintone_ovl_destruct(Submenu* submenu) {
    submenu->overlay->skintone_ovl = NULL;
}

#endif /* VERSION >= VER_DELUXE */
