#include "types.h"
#if VERSION >= VER_DELUXE
#include "m_banti.h"

#include "m_play.h"
#include "libultra/libultra.h"
#include "m_player_lib.h"
#include "m_demo.h"
#include "sys_matrix.h"
#include "m_font.h"
#include "m_common_data.h"
#include "m_field_info.h"
#include "m_event.h"
#include "m_lib.h"
#include "evw_anime.h"

/* Deluxe-only function not yet in evw_anime.h */
extern void Evw_Anime_Set_Param(GAME_PLAY* play, EVW_ANIME_DATA* evw_anime_data, u32 frame);

/* Banti PP (Piracy Protection banner) static data */

typedef struct banti_pp_s {
    /* 0x00 */ int addressable_type;
    /* 0x04 */ int state_flag;
    /* 0x08 */ int timer;
    /* 0x0C */ int direction;
    /* 0x10 */ int disabled;
    /* 0x14 */ f32 alpha;
    /* 0x18 */ f32 offset;
    /* 0x1C */ int pad1C;
    /* 0x20 */ int mode;
    /* 0x24 */ int pad24;
    /* 0x28 */ u8 hour_upper_anim;
    /* 0x29 */ u8 hour_lower_anim;
    /* 0x2A */ u8 weekday_anim;
    /* 0x2B */ u8 ampm_anim;
    /* 0x2C */ u8 min_upper_anim;
    /* 0x2D */ u8 min_lower_anim;
    /* 0x2E */ u8 min_upper_digit;
    /* 0x2F */ u8 min_lower_digit;
    /* 0x30 */ lbRTC_time_c displayed_time;
    /* 0x38 */ lbRTC_time_c next_time;
} Banti_pp_c;

static Banti_pp_c banti_pp;

/* Float constants matching original binary */
static f32 banti_pp_alpha_start = 0.0f;
static f32 banti_pp_alpha_full = 1.0f;

/* Access GRAPH DL pointers at Deluxe-specific offsets 0xB0 and 0xD0 */
#define GRAPH_PP_DL_PTR0(g) (*(Gfx**)((u8*)(g) + 0xB0))
#define GRAPH_PP_DL_PTR1(g) (*(Gfx**)((u8*)(g) + 0xD0))
#define GRAPH_PP_ALLOC_PTR(g) (*(Gfx**)((u8*)(g) + 0xB4))

/* External clock display list models */
extern Gfx clk_win_mode[];
extern Gfx clk_win_youbiT_model[];
extern Gfx clk_win_maruT_model[];
extern Gfx clk_win_maru2T_model[];
extern Gfx clk_win_ampmT_model[];

/* External textures */
extern u8 clk_win_pm_tex_rgb_ia8[];
extern u8 clk_win_am_tex_rgb_ia8[];

extern u8 clk_win_sun_tex_rgb_ia8[];
extern u8 clk_win_mon_tex_rgb_ia8[];
extern u8 clk_win_tue_tex_rgb_ia8[];
extern u8 clk_win_wed_tex_rgb_ia8[];
extern u8 clk_win_thu_tex_rgb_ia8[];
extern u8 clk_win_fri_tex_rgb_ia8[];
extern u8 clk_win_sat_tex_rgb_ia8[];

extern u8 clk_win_jikan0_TA_tex_txt[];
extern u8 clk_win_jikan1_TA_tex_txt[];
extern u8 clk_win_jikan2_TA_tex_txt[];
extern u8 clk_win_jikan3_TA_tex_txt[];
extern u8 clk_win_jikan4_TA_tex_txt[];
extern u8 clk_win_jikan5_TA_tex_txt[];
extern u8 clk_win_jikan6_TA_tex_txt[];
extern u8 clk_win_jikan7_TA_tex_txt[];
extern u8 clk_win_jikan8_TA_tex_txt[];
extern u8 clk_win_jikan9_TA_tex_txt[];
extern u8 clk_win_jikan_TA_tex_txt[];

extern u8 clk_win_suuji1_TA_tex_txt[];
extern u8 clk_win_suuji2_TA_tex_txt[];
extern u8 clk_win_suuji3_TA_tex_txt[];
extern u8 clk_win_suuji4_TA_tex_txt[];
extern u8 clk_win_suuji5_TA_tex_txt[];
extern u8 clk_win_suuji6_TA_tex_txt[];
extern u8 clk_win_suuji7_TA_tex_txt[];
extern u8 clk_win_suuji8_TA_tex_txt[];
extern u8 clk_win_suuji9_TA_tex_txt[];
extern u8 clk_win_suuji10_TA_tex_txt[];
extern u8 clk_win_suuji11_TA_tex_txt[];
extern u8 clk_win_suuji12_TA_tex_txt[];

/* Forward declarations */
extern void banti_pp_dt(void);
extern void banti_pp_ct(void);
extern void banti_pp_move(GAME_PLAY* play);
extern void banti_pp_draw(GAME_PLAY* play);

static int banti_check_disp_condition(GAME_PLAY* play);
static void banti_disp_offset(GAME_PLAY* play);
static void banti_draw_yobi(Gfx** gfx_pp, GAME_PLAY* play, int poly_render);
static void banti_draw_ampm(Gfx** gfx_pp, GAME_PLAY* play, int poly_render);
static void banti_draw_time(Gfx** gfx_pp, GAME_PLAY* play, int poly_render);
static void banti_draw_time_sub(Gfx** gfx_pp, GAME_PLAY* play, int idx0, int idx1, int hide_zero, int pos_idx, int anim_state, int alpha);
static void banti_evw_anime(Gfx** gfx_pp, GAME_PLAY* play, int restore, EVW_ANIME_DATA* evw_anime, int count);
static Gfx* banti_get_DL_pointer(GRAPH* graph, int mode);
static void banti_set_DL_pointer(GRAPH* graph, Gfx* gfx, int mode);

/* === banti_pp_dt === */
/* 4 bytes: just blr (empty destructor) */
extern void banti_pp_dt(void) {
}

/* === banti_pp_ct === */
/* Initializes the PP banner state */
extern void banti_pp_ct(void) {
    GAME_PLAY* play;

    bzero(&banti_pp, sizeof(Banti_pp_c));

    banti_pp.addressable_type = 0;
    banti_pp.state_flag = 0;
    banti_pp.timer = 0;
    banti_pp.direction = 0;
    banti_pp.disabled = 0;
    banti_pp.alpha = 0.0f;

    lbRTC_TimeCopy(&banti_pp.displayed_time, Common_GetPointer(time.rtc_time));
    lbRTC_TimeCopy(&banti_pp.next_time, Common_GetPointer(time.rtc_time));

    play = (GAME_PLAY*)gamePT;
    if (play != NULL) {
        if (banti_check_disp_condition(play) == 0) {
            banti_pp.mode = 2;
            banti_pp.offset = banti_pp_alpha_start;
        } else {
            banti_pp.mode = 0;
            banti_pp.offset = banti_pp_alpha_start;
        }
    }
}

/* === banti_check_disp_condition === */
/* Checks whether the PP banner should be displayed */
static int banti_check_disp_condition(GAME_PLAY* play) {
    int result;

    result = 1;

    if (mDemo_CheckDemo() == 0) {
        if (mEv_IsTitleDemo() <= 0) {
            if (banti_pp.state_flag == 0) {
                goto check_play;
            }
        }
    }

    banti_pp.addressable_type = 1;
    result = 0;
    goto done;

check_play:
    if (banti_pp.addressable_type != 0) {
        result = 0;
    } else if (play->submenu.process_status != mSM_PROCESS_WAIT) {
        result = 0;
    }

done:
    return result;
}

/* === banti_disp_offset === */
/* Calculates the display offset for the PP banner position */
static void banti_disp_offset(GAME_PLAY* play) {
    f32 target;
    f32 fraction;

    if (banti_pp.mode == 0) {
        target = 1.0f;
    } else {
        target = 0.0f;
    }

    fraction = 1.0f - sqrtf(0.8f);
    add_calc(&banti_pp.alpha, target, fraction, 0.0425f, 0.0005f);
}

/* === banti_pp_move === */
/* Main move/update function for PP banner */
extern void banti_pp_move(GAME_PLAY* play) {
    int addressable_type;
    int update;

    banti_check_disp_condition(play);

    addressable_type = mPlib_Get_address_able_display();

    if (banti_pp.addressable_type == addressable_type) {
        banti_pp.timer = 0;
    } else {
        update = 0;
        banti_pp.timer++;

        if (addressable_type == mPlayer_ADDRESSABLE_FALSE_READY_NET) {
            if (banti_pp.timer > 18) {
                update = 1;
            }
        } else if (banti_pp.addressable_type == mPlayer_ADDRESSABLE_TRUE) {
            if (banti_pp.timer > 50 || addressable_type == mPlayer_ADDRESSABLE_FALSE_TALKING) {
                update = 1;
            }
        } else {
            if (banti_pp.timer > 76 || addressable_type == mPlayer_ADDRESSABLE_FALSE_TALKING) {
                update = 1;
            }
        }

        if (update != 0) {
            banti_pp.timer = 0;
            banti_pp.addressable_type = addressable_type;
        }
    }

    banti_disp_offset(play);
    /* TODO: calls banti_time_check or equivalent at runtime 0x2de8b4 */
}

/* === banti_get_DL_pointer === */
/* Gets the display list pointer from GRAPH at the appropriate offset */
static Gfx* banti_get_DL_pointer(GRAPH* graph, int mode) {
    if (mode == 1) {
        return GRAPH_PP_DL_PTR0(graph);
    } else {
        return GRAPH_PP_DL_PTR1(graph);
    }
}

/* === banti_set_DL_pointer === */
/* Sets the display list pointer in GRAPH at the appropriate offset */
static void banti_set_DL_pointer(GRAPH* graph, Gfx* gfx, int mode) {
    if (mode == 1) {
        GRAPH_PP_DL_PTR0(graph) = gfx;
    } else {
        GRAPH_PP_DL_PTR1(graph) = gfx;
    }
}

/* === banti_evw_anime === */
/* Runs environment animation by temporarily redirecting the DL pointer */
static void banti_evw_anime(Gfx** gfx_pp, GAME_PLAY* play, int restore, EVW_ANIME_DATA* evw_anime, int count) {
    GRAPH* graph;
    Gfx* saved_dl;
    int frame;

    graph = play->game.graph;
    frame = 30 - count;
    saved_dl = GRAPH_PP_DL_PTR0(graph);
    GRAPH_PP_DL_PTR0(graph) = *gfx_pp;

    Evw_Anime_Set_Param(play, evw_anime, frame);

    *gfx_pp = GRAPH_PP_DL_PTR0(graph);

    if (restore != 1) {
        GRAPH_PP_DL_PTR0(graph) = saved_dl;
    }
}

/* === banti_draw_yobi === */
/* Draws the day-of-week indicator */
static void banti_draw_yobi(Gfx** gfx_pp, GAME_PLAY* play, int poly_render) {
    static u8* week_tex_table[lbRTC_WEEK] = {
        clk_win_sun_tex_rgb_ia8, clk_win_mon_tex_rgb_ia8, clk_win_tue_tex_rgb_ia8,
        clk_win_wed_tex_rgb_ia8, clk_win_thu_tex_rgb_ia8, clk_win_fri_tex_rgb_ia8,
        clk_win_sat_tex_rgb_ia8
    };

    static u8* ampm_tex_table[2] = {
        clk_win_am_tex_rgb_ia8, clk_win_pm_tex_rgb_ia8
    };

    Gfx* dl;
    u8 weekday;
    u8 ampm_flag;

    /* Set prim depth + blend */
    dl = *gfx_pp;
    gDPSetPrimColor(dl++, 0, 0xFF, 0xFA, 0x00, 0x00, 0xFF);
    *gfx_pp = dl;

    weekday = banti_pp.weekday_anim;
    if (weekday == 0) {
        /* No animation, use default DL */
        gSPSegment(gfx_pp[0]++, G_MWO_SEGMENT_9, &banti_pp);
        goto draw_ampm_part;
    }

    /* Check next weekday for color selection */
    ampm_flag = banti_pp.next_time.weekday;
    if (ampm_flag == 0) {
        /* Sunday: red tint */
        gDPSetPrimColor(gfx_pp[0]++, 0, 0, 0xFF, 0xFF, 0xE1, 0xFF);
        gDPSetEnvColor(gfx_pp[0]++, 0xC8, 0x3C, 0xFF, 0xFF);
    } else {
        /* Other days */
        gDPSetPrimColor(gfx_pp[0]++, 0, 0, 0xFF, 0xFF, 0xE1, 0xFF);
        gDPSetEnvColor(gfx_pp[0]++, 0x46, 0x8C, 0x32, 0xFF);
    }

    /* Set segment and matrix for weekday display */
    gSPSegment(gfx_pp[0]++, G_MWO_SEGMENT_9, week_tex_table[banti_pp.next_time.weekday]);
    gSPMatrix(gfx_pp[0]++, 0, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);

    banti_evw_anime(gfx_pp, play, 0, NULL, weekday);

draw_ampm_part:
    /* Draw weekday text */
    {
        u8 displayed_weekday;

        gSPSegment(gfx_pp[0]++, G_MWO_SEGMENT_8, &banti_pp);
        displayed_weekday = banti_pp.displayed_time.weekday;

        if (displayed_weekday == 0) {
            /* Sunday colors */
            gDPSetPrimColor(gfx_pp[0]++, 0, 0, 0xFF, 0xFF, 0xE1, 0xFF);
            gDPSetEnvColor(gfx_pp[0]++, 0xC8, 0x3C, 0xFF, 0xFF);
        } else {
            /* Other day colors */
            gDPSetPrimColor(gfx_pp[0]++, 0, 0, 0xFF, 0xFF, 0xE1, 0xFF);
            gDPSetEnvColor(gfx_pp[0]++, 0x46, 0x8C, 0x32, 0xFF);
        }

        gSPMatrix(gfx_pp[0]++, 0, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    }
}

/* === banti_draw_ampm === */
/* Draws the AM/PM indicator */
static void banti_draw_ampm(Gfx** gfx_pp, GAME_PLAY* play, int poly_render) {
    static u8* ampm_tex_table[2] = {
        clk_win_am_tex_rgb_ia8, clk_win_pm_tex_rgb_ia8
    };

    int is_pm;
    u8 ampm_anim_flag;

    /* Set prim depth */
    gDPSetPrimColor(gfx_pp[0]++, 0, 0xFF, 0xFA, 0x00, 0x00, 0xFF);
    gDPSetEnvColor(gfx_pp[0]++, 0, 0, 0, 0xE1FF);

    /* Determine AM/PM from displayed hour */
    is_pm = banti_pp.displayed_time.hour >= 12 ? 1 : 0;

    ampm_anim_flag = banti_pp.ampm_anim;
    if (ampm_anim_flag == 0) {
        /* No animation - just set segment to default */
        gSPSegment(gfx_pp[0]++, G_MWO_SEGMENT_9, &banti_pp);
        goto draw_end;
    }

    /* Animation active */
    {
        u8 next_is_pm;

        next_is_pm = banti_pp.next_time.hour >= 12 ? 1 : 0;

        gSPSegment(gfx_pp[0]++, G_MWO_SEGMENT_9, ampm_tex_table[next_is_pm]);

        if (next_is_pm == 0) {
            gDPSetPrimColor(gfx_pp[0]++, 0, 0, 0xE6, 0x78, 0x00, 0xFF);
            gDPSetEnvColor(gfx_pp[0]++, 0, 0, 0, 0xFF);
        } else {
            gDPSetPrimColor(gfx_pp[0]++, 0, 0, 0x8C, 0x3D, 0xB5, 0xFF);
            gDPSetEnvColor(gfx_pp[0]++, 0, 0, 0, 0xFF);
        }

        gSPMatrix(gfx_pp[0]++, 0, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);

        banti_evw_anime(gfx_pp, play, 0, NULL, ampm_anim_flag);
    }

draw_end:
    /* Draw AM/PM text with displayed time */
    gSPSegment(gfx_pp[0]++, G_MWO_SEGMENT_8, ampm_tex_table[is_pm]);

    if (is_pm == 0) {
        gDPSetPrimColor(gfx_pp[0]++, 0, 0, 0xE6, 0x78, 0x00, 0xFF);
        gDPSetEnvColor(gfx_pp[0]++, 0, 0, 0, 0xFF);
    } else {
        gDPSetPrimColor(gfx_pp[0]++, 0, 0, 0x8C, 0x3D, 0xB5, 0xFF);
        gDPSetEnvColor(gfx_pp[0]++, 0, 0, 0, 0xFF);
    }

    gSPMatrix(gfx_pp[0]++, 0, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
}

/* === banti_draw_time_sub === */
/* Helper to draw a single time digit position */
static void banti_draw_time_sub(Gfx** gfx_pp, GAME_PLAY* play, int idx0, int idx1, int hide_zero, int pos_idx, int anim_state, int alpha) {
    static u8* jikan_tex_table[11] = {
        clk_win_jikan0_TA_tex_txt, clk_win_jikan1_TA_tex_txt, clk_win_jikan2_TA_tex_txt,
        clk_win_jikan3_TA_tex_txt, clk_win_jikan4_TA_tex_txt, clk_win_jikan5_TA_tex_txt,
        clk_win_jikan6_TA_tex_txt, clk_win_jikan7_TA_tex_txt, clk_win_jikan8_TA_tex_txt,
        clk_win_jikan9_TA_tex_txt, clk_win_jikan_TA_tex_txt
    };

    static u32 pos_colors[6 * 3] = {
        0, 0, 0,
        0, 0, 0,
        0, 0, 0,
        0, 0, 0,
        0, 0, 0,
        0, 0, 0
    };

    if (hide_zero == TRUE) {
        if (idx0 == 0) {
            idx0 = 0x20;
        }
        if (idx1 == 0) {
            idx1 = 0x20;
        }
    }

    if (anim_state == 0) {
        /* No animation - just set segment texture */
        gSPSegment(gfx_pp[0]++, G_MWO_SEGMENT_9, &banti_pp);
    } else {
        /* Animation active */
        gSPSegment(gfx_pp[0]++, G_MWO_SEGMENT_9, jikan_tex_table[idx1]);
        gSPMatrix(gfx_pp[0]++, pos_colors + pos_idx * 3 + 2, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);

        banti_evw_anime(gfx_pp, play, 0, NULL, anim_state);
    }

    /* Set main segment texture */
    gSPSegment(gfx_pp[0]++, G_MWO_SEGMENT_8, jikan_tex_table[idx0]);
    gSPMatrix(gfx_pp[0]++, pos_colors + pos_idx * 3, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
}

/* === banti_draw_time === */
/* Draws the full time display (hours and minutes) */
static void banti_draw_time(Gfx** gfx_pp, GAME_PLAY* play, int poly_render) {
    int now_hour;
    int next_hour;
    int ten;
    int ten2;

    /* Set colors for time display */
    gDPSetPrimColor(gfx_pp[0]++, 0, 0xFF, 0xFA, 0x00, 0x00, 0xFF);
    gDPSetEnvColor(gfx_pp[0]++, 0x6E, 0x46, 0x46, 0xFF);

    /* Convert displayed hour to 12-hour format */
    now_hour = banti_pp.displayed_time.hour;
    if (now_hour == 0) {
        now_hour = 12;
    } else if (now_hour > 11) {
        now_hour -= 12;
    }

    /* Convert next hour to 12-hour format */
    next_hour = banti_pp.next_time.hour;
    if (next_hour == 0) {
        next_hour = 12;
    } else if (next_hour > 11) {
        next_hour -= 12;
    }

    /* Draw month digits (position 0) */
    banti_draw_time_sub(gfx_pp, play,
                        banti_pp.displayed_time.month, banti_pp.next_time.month,
                        0, 0, banti_pp.hour_upper_anim, poly_render);

    /* Draw day digits (position 1) */
    banti_draw_time_sub(gfx_pp, play,
                        banti_pp.displayed_time.day, banti_pp.next_time.day,
                        0, 1, banti_pp.hour_lower_anim, poly_render);

    /* Draw hour tens (position 2) */
    ten = now_hour / 10;
    ten2 = next_hour / 10;
    banti_draw_time_sub(gfx_pp, play,
                        ten, ten2,
                        1, 2, banti_pp.min_upper_anim, poly_render);

    /* Draw hour ones (position 3) */
    ten = now_hour / 10;
    ten2 = next_hour / 10;
    banti_draw_time_sub(gfx_pp, play,
                        now_hour - ten * 10, next_hour - ten2 * 10,
                        0, 3, banti_pp.min_lower_anim, poly_render);

    /* Draw minute tens (position 4) */
    banti_draw_time_sub(gfx_pp, play,
                        banti_pp.displayed_time.min / 10, banti_pp.next_time.min / 10,
                        0, 4, banti_pp.min_upper_digit, poly_render);

    /* Draw minute ones (position 5) */
    {
        int disp_min;
        int next_min;
        int disp_tens;
        int next_tens;

        disp_min = banti_pp.displayed_time.min;
        next_min = banti_pp.next_time.min;
        disp_tens = disp_min / 10;
        next_tens = next_min / 10;

        banti_draw_time_sub(gfx_pp, play,
                            disp_min - disp_tens * 10, next_min - next_tens * 10,
                            0, 5, banti_pp.min_lower_digit, poly_render);
    }
}

/* === banti_pp_draw === */
/* Main draw function for the PP banner */
extern void banti_pp_draw(GAME_PLAY* play) {
    GRAPH* graph;
    int poly_render;
    Mtx* m;
    Gfx* gfx;

    if (banti_pp.mode == 2) {
        return;
    }

    if (mFI_GET_TYPE(mFI_GetFieldId()) != mFI_FIELDTYPE_FG) {
        return;
    }

    if (mEv_CheckFirstIntro() == TRUE) {
        return;
    }

    graph = play->game.graph;
    poly_render = 0;

    m = (Mtx*)GRAPH_ALLOC(graph, sizeof(Mtx));

    if (play->submenu.process_status != mSM_PROCESS_WAIT && play->submenu.mode > 2) {
        poly_render = 1;
    }

    if (m != NULL) {
        mFont_CulcOrthoMatrix(m);
    }

    /* Set up display list pointer */
    gfx = banti_get_DL_pointer(graph, poly_render);
    {
        Gfx* gfx_local = gfx;
        Gfx** gfx_pp = &gfx_local;

        /* Set up projection matrix */
        gSPMatrix(gfx_pp[0]++, m, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);

        /* Set up model matrix with scale */
        Matrix_scale(16.0f, 16.0f, 16.0f, 0);

        if (banti_pp.direction == 1) {
            Matrix_translate(-184.0f, 0.0f, 0.0f, 1);
        }

        {
            f32 offset_x;
            f32 base_x;

            if (banti_pp.disabled != 0) {
                base_x = banti_pp_alpha_full;
            } else {
                base_x = banti_pp_alpha_start;
            }

            offset_x = base_x;
            if (banti_pp.direction == 1) {
                offset_x = -base_x;
            }

            {
                f32 move_amount;

                move_amount = offset_x - banti_pp.offset;
                Matrix_translate(move_amount, 0.0f, 0.0f, 1);
            }
        }

        /* Set model view matrix */
        gSPMatrix(gfx_pp[0]++, _Matrix_to_Mtx_new(graph), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);

        /* Set rendering mode */
        gSPDisplayList(gfx_pp[0]++, clk_win_mode);
        gDPSetRenderMode(gfx_pp[0]++, G_RM_PASS, G_RM_XLU_SURF2);

        {
            Gfx* dl;

            dl = *gfx_pp;
            gSPMatrix(dl++, _Matrix_to_Mtx_new(graph), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_PROJECTION);
            *gfx_pp = dl;
        }

        /* Draw components */
        banti_draw_yobi(gfx_pp, play, poly_render);
        banti_draw_ampm(gfx_pp, play, poly_render);
        banti_draw_time(gfx_pp, play, poly_render);

        /* Final rendering setup */
        gDPSetPrimColor(gfx_pp[0]++, 0, 0xFF, 0xFA, 0x00, 0x00, 0xFF);
        gDPSetEnvColor(gfx_pp[0]++, 0xFFB4, 0x50, 0xFF, 0xFF);

        gSPMatrix(gfx_pp[0]++, 0, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
        gDPSetPrimColor(gfx_pp[0]++, 0, 0xFF, 0x78, 0x46, 0x00, 0xFF);

        gSPMatrix(gfx_pp[0]++, 0, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);

        /* Check if seconds are odd for dot blink */
        if ((Common_Get(time.rtc_time).sec & 1) == 1) {
            gDPSetPrimColor(gfx_pp[0]++, 0, 0xFF, 0x78, 0x46, 0x00, 0xFF);
            gSPMatrix(gfx_pp[0]++, 0, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
        }

        /* Store back the display list pointer */
        banti_set_DL_pointer(graph, *gfx_pp, poly_render);
    }
}
#endif /* VERSION >= VER_DELUXE */
