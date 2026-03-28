#include "m_storage_ovl.h"

#if VERSION >= VER_DELUXE

#include "m_common_data.h"
#include "m_font.h"
#include "m_item_name.h"
#include "m_player_lib.h"
#include "m_private.h"
#include "m_lib.h"
#include "sys_matrix.h"
#include "audio.h"

static mSO_Ovl_c storage_ovl_data;

static u8 mSO_page_strs[mSO_PAGE_NUM][7] = {
    "Page 1",
    "Page 2",
    "Page 3"
};

static void mSO_move_Move(Submenu* submenu, mSM_MenuInfo_c* menu_info) {
    (*submenu->overlay->move_Move_proc)(submenu, menu_info);
}

static void mSO_move_Wait(Submenu* submenu, mSM_MenuInfo_c* menu_info) {
    mSO_Ovl_c* so_ovl = submenu->overlay->storage_ovl;

    /* Animate page scroll */
    if (so_ovl->scroll_y != so_ovl->target_scroll_y) {
        add_calc(&so_ovl->scroll_y, so_ovl->target_scroll_y, 0.3f, 20.0f, 1.0f);
    }

    if (so_ovl->current_page != so_ovl->target_page) {
        f32 diff = so_ovl->scroll_y - so_ovl->target_scroll_y;
        if (diff < 0.0f) {
            diff = -diff;
        }

        if (diff < 2.0f) {
            so_ovl->current_page = so_ovl->target_page;
            so_ovl->scroll_y = so_ovl->target_scroll_y;
            menu_info->proc_status = mSM_OVL_PROC_PLAY;
        }
    } else {
        menu_info->proc_status = mSM_OVL_PROC_PLAY;
    }
}

static void mSO_move_Play(Submenu* submenu, mSM_MenuInfo_c* menu_info) {
    mSO_Ovl_c* so_ovl = submenu->overlay->storage_ovl;
    u32 trigger = submenu->overlay->menu_control.trigger;
    int cursor = so_ovl->cursor_idx;

    if (trigger & BUTTON_B) {
        /* Exit storage menu */
        (*submenu->overlay->move_chg_base_proc)(menu_info, mSM_MOVE_OUT_TOP);
        sAdo_SysTrgStart(NA_SE_MENU_EXIT);
    } else if (trigger & BUTTON_A) {
        /* Confirm item action */
        if (Now_Private != NULL) {
            int idx = so_ovl->current_page * mSO_ITEMS_PER_PAGE + cursor;

            if (idx >= 0 && idx < mSO_TOTAL_ITEMS) {
                if (so_ovl->mode == mSO_MODE_TAKEOUT) {
                    mActor_name_t item = g_dlx_storage_item[idx];

                    if (item != EMPTY_NO) {
                        sAdo_SysTrgStart(NA_SE_CURSOL);
                    }
                } else {
                    sAdo_SysTrgStart(NA_SE_CURSOL);
                }
            }
        }
    } else if (trigger & BUTTON_CUP) {
        if (cursor > 0) {
            so_ovl->cursor_idx--;
            sAdo_SysTrgStart(NA_SE_CURSOL);
        }
    } else if (trigger & BUTTON_CDOWN) {
        if (cursor < mSO_ITEMS_PER_PAGE - 1) {
            so_ovl->cursor_idx++;
            sAdo_SysTrgStart(NA_SE_CURSOL);
        }
    } else if (trigger & BUTTON_CLEFT) {
        if (so_ovl->current_page > 0) {
            so_ovl->target_page = so_ovl->current_page - 1;
            so_ovl->target_scroll_y = (f32)so_ovl->target_page * -160.0f;
            so_ovl->cursor_idx = 0;
            menu_info->proc_status = mSM_OVL_PROC_WAIT;
            sAdo_SysTrgStart(NA_SE_CURSOL);
        }
    } else if (trigger & BUTTON_CRIGHT) {
        if (so_ovl->current_page < mSO_PAGE_NUM - 1) {
            so_ovl->target_page = so_ovl->current_page + 1;
            so_ovl->target_scroll_y = (f32)so_ovl->target_page * -160.0f;
            so_ovl->cursor_idx = 0;
            menu_info->proc_status = mSM_OVL_PROC_WAIT;
            sAdo_SysTrgStart(NA_SE_CURSOL);
        }
    }
}

static void mSO_move_End(Submenu* submenu, mSM_MenuInfo_c* menu_info) {
    (*submenu->overlay->move_End_proc)(submenu, menu_info);
}

typedef void (*mSO_MOVE_PROC)(Submenu*, mSM_MenuInfo_c*);

static void mSO_storage_ovl_move(Submenu* submenu) {
    static mSO_MOVE_PROC ovl_move_proc[mSM_OVL_PROC_NUM] = {
        &mSO_move_Move,
        &mSO_move_Play,
        &mSO_move_Wait,
        (mSO_MOVE_PROC)none_proc1,
        &mSO_move_End
    };

    mSM_MenuInfo_c* menu_info = &submenu->overlay->menu_info[mSM_OVL_STORAGE];

    (*menu_info->pre_move_func)(submenu);
    (*ovl_move_proc[menu_info->proc_status])(submenu, menu_info);
}

static f32 mSO_get_page_posY(Submenu* submenu, mSM_MenuInfo_c* menu_info, int page) {
    mSO_Ovl_c* so_ovl = submenu->overlay->storage_ovl;
    return menu_info->position[1] + so_ovl->scroll_y + (f32)page * 160.0f;
}

extern Gfx att_win_model[];

static void mSO_set_page_dl(Submenu* submenu, mSM_MenuInfo_c* menu_info, GAME* game, GRAPH* graph, int page, int is_current) {
    mSO_Ovl_c* so_ovl = submenu->overlay->storage_ovl;
    f32 x = menu_info->position[0];
    f32 y = mSO_get_page_posY(submenu, menu_info, page);
    Gfx* gfx;
    rgba_t title_col;
    rgba_t normal_col;
    rgba_t select_col;
    int i;
    f32 row_y;

    title_col.r = 255; title_col.g = 255; title_col.b = 255; title_col.a = 255;
    normal_col.r = 70; normal_col.g = 120; normal_col.b = 245; normal_col.a = 255;
    select_col.r = 215; select_col.g = 0; select_col.b = 0; select_col.a = 255;

    /* Draw page frame */
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

    /* Draw page title */
    (*submenu->overlay->set_char_matrix_proc)(graph);
    mFont_SetLineStrings(game, mSO_page_strs[page], 6,
                         x + 148.0f, -y + 58.0f,
                         title_col.r, title_col.g, title_col.b, 255,
                         FALSE, TRUE, 0.875f, 0.875f, mFont_MODE_POLY);

    /* Draw item slots */
    if (is_current) {
        int base_idx = page * mSO_ITEMS_PER_PAGE;

        for (i = 0; i < mSO_ITEMS_PER_PAGE; i++) {
            rgba_t* color_p;
            mActor_name_t item;

            if (Now_Private == NULL) {
                break;
            }

            item = g_dlx_storage_item[base_idx + i];
            row_y = -y + 72.0f + (f32)(i * 10);

            if (so_ovl->cursor_idx == i) {
                color_p = &select_col;
            } else {
                color_p = &normal_col;
            }

            if (item != EMPTY_NO) {
                u8 item_name_buf[mIN_ITEM_NAME_LEN];

                mIN_copy_name_str(item_name_buf, item);
                mFont_SetLineStrings(game, item_name_buf, mIN_ITEM_NAME_LEN,
                                     x + 160.0f, row_y,
                                     color_p->r, color_p->g, color_p->b, 255,
                                     FALSE, TRUE, 0.75f, 0.75f, mFont_MODE_POLY);
            }
        }
    }
}

/* Deluxe: set items in storage overlay — draw item list for current page (524B) */
static void mSO_set_items(Submenu* submenu, GRAPH* graph, f32 x, f32 y, int page) {
    mSO_Ovl_c* so_ovl = submenu->overlay->storage_ovl;
    int base_idx = page * mSO_ITEMS_PER_PAGE;
    int i;
    rgba_t normal_col;
    rgba_t select_col;
    f32 row_y;

    normal_col.r = 70; normal_col.g = 120; normal_col.b = 245; normal_col.a = 255;
    select_col.r = 215; select_col.g = 0; select_col.b = 0; select_col.a = 255;

    for (i = 0; i < mSO_ITEMS_PER_PAGE; i++) {
        rgba_t* color_p;
        mActor_name_t item;

        if (Now_Private == NULL) {
            break;
        }

        item = g_dlx_storage_item[base_idx + i];
        row_y = -y + 72.0f + (f32)(i * 10);

        if (so_ovl->cursor_idx == i) {
            color_p = &select_col;
        } else {
            color_p = &normal_col;
        }

        if (item != EMPTY_NO) {
            u8 item_name_buf[mIN_ITEM_NAME_LEN];

            mIN_copy_name_str(item_name_buf, item);
            mFont_SetLineStrings(gamePT, item_name_buf, mIN_ITEM_NAME_LEN,
                                 x + 160.0f, row_y,
                                 color_p->r, color_p->g, color_p->b, 255,
                                 FALSE, TRUE, 0.75f, 0.75f, mFont_MODE_POLY);
        }
    }
}

static void mSO_set_dl(Submenu* submenu, mSM_MenuInfo_c* menu_info, GAME* game) {
    GRAPH* graph = game->graph;
    mSO_Ovl_c* so_ovl = submenu->overlay->storage_ovl;

    mSO_set_page_dl(submenu, menu_info, game, graph, so_ovl->current_page, TRUE);
}

static void mSO_storage_ovl_draw(Submenu* submenu, GAME* game) {
    mSM_MenuInfo_c* menu_info = &submenu->overlay->menu_info[mSM_OVL_STORAGE];

    (*menu_info->pre_draw_func)(submenu, game);
    mSO_set_dl(submenu, menu_info, game);
}

extern void mSO_storage_ovl_set_proc(Submenu* submenu) {
    mSM_Control_c* control = &submenu->overlay->menu_control;

    control->menu_move_func = &mSO_storage_ovl_move;
    control->menu_draw_func = &mSO_storage_ovl_draw;
}

static void mSO_storage_ovl_init(Submenu* submenu, mSM_MenuInfo_c* menu_info) {
    mSO_Ovl_c* so_ovl = submenu->overlay->storage_ovl;

    submenu->overlay->menu_control.animation_flag = FALSE;
    menu_info->proc_status = mSM_OVL_PROC_MOVE;
    menu_info->next_proc_status = mSM_OVL_PROC_PLAY;
    menu_info->move_drt = mSM_MOVE_IN_TOP;

    so_ovl->cursor_idx = 0;
    so_ovl->current_page = 0;
    so_ovl->target_page = 0;
    so_ovl->scroll_y = 0.0f;
    so_ovl->target_scroll_y = 0.0f;
    so_ovl->mode = mSO_MODE_TAKEOUT;
    so_ovl->initial_setup = TRUE;
}

extern void mSO_storage_ovl_construct(Submenu* submenu) {
    Submenu_Overlay_c* overlay = submenu->overlay;

    if (overlay->storage_ovl == NULL) {
        mem_clear((u8*)&storage_ovl_data, sizeof(mSO_Ovl_c), 0);
        overlay->storage_ovl = &storage_ovl_data;
    }

    mSO_storage_ovl_init(submenu, &overlay->menu_info[mSM_OVL_STORAGE]);
    mSO_storage_ovl_set_proc(submenu);
}

extern void mSO_storage_ovl_destruct(Submenu* submenu) {
    submenu->overlay->storage_ovl = NULL;
}

#endif /* VERSION >= VER_DELUXE */
