#include "m_banti.h"

#if VERSION >= VER_DELUXE

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

/* N64-style bulletin board (banti) clock display.
 * This module renders the clock HUD using N64-era display list commands
 * via the emu64 GBI system, as opposed to m_banti.c which uses native
 * GC/Dolphin rendering with skeleton-based keyframe animation.
 *
 * 8 functions total:
 *   banti_64_ct                       (44B)   - constructor
 *   banti_64_dt                       (4B)    - destructor
 *   banti_64_move                     (732B)  - update logic
 *   banti_evw_anime_texanime_tukihi   (192B)  - month/day texture animation
 *   banti_evw_anime_texanime_jikan    (196B)  - time digit texture animation
 *   banti_evw_anime_texanime          (148B)  - general texture animation
 *   Banti_Anime_Set                   (152B)  - set animation parameters
 *   banti_64_draw                     (1468B) - draw callback
 */

/* Static state for the N64-style banti display */
typedef struct banti_64_s {
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
    /* 0x28 */ u8 month_anim;
    /* 0x29 */ u8 day_anim;
    /* 0x2A */ u8 weekday_anim;
    /* 0x2B */ u8 ampm_anim;
    /* 0x2C */ u8 hour_upper_anim;
    /* 0x2D */ u8 hour_lower_anim;
    /* 0x2E */ u8 min_upper_anim;
    /* 0x2F */ u8 min_lower_anim;
    /* 0x30 */ lbRTC_time_c displayed_time;
    /* 0x38 */ lbRTC_time_c next_time;
} Banti_64_c;

static Banti_64_c banti_64;

static f32 banti_64_alpha_start = 0.0f;
static f32 banti_64_alpha_full = 1.0f;

/* Access GRAPH DL pointers at Deluxe-specific offsets */
#define GRAPH_64_DL_PTR0(g) (*(Gfx**)((u8*)(g) + 0xB0))
#define GRAPH_64_DL_PTR1(g) (*(Gfx**)((u8*)(g) + 0xD0))

/* External clock display list models */
extern Gfx clk_win_mode[];
extern Gfx clk_win_youbiT_model[];
extern Gfx clk_win_maruT_model[];
extern Gfx clk_win_maru2T_model[];
extern Gfx clk_win_ampmT_model[];

/* External textures - AM/PM */
extern u8 clk_win_pm_tex_rgb_ia8[];
extern u8 clk_win_am_tex_rgb_ia8[];

/* External textures - weekday */
extern u8 clk_win_sun_tex_rgb_ia8[];
extern u8 clk_win_mon_tex_rgb_ia8[];
extern u8 clk_win_tue_tex_rgb_ia8[];
extern u8 clk_win_wed_tex_rgb_ia8[];
extern u8 clk_win_thu_tex_rgb_ia8[];
extern u8 clk_win_fri_tex_rgb_ia8[];
extern u8 clk_win_sat_tex_rgb_ia8[];

/* External textures - time digits (jikan) */
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

/* External textures - date digits (suuji) */
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
static int banti_64_check_disp_condition(GAME_PLAY* play);
static void banti_64_disp_offset(GAME_PLAY* play);
static void banti_64_time_check(void);

/* === banti_64_ct === */
/* Constructor: initializes the N64-style banti display state (44B) */
extern void banti_64_ct(void) {
    GAME_PLAY* play;

    bzero(&banti_64, sizeof(Banti_64_c));

    banti_64.addressable_type = 0;
    banti_64.state_flag = 0;
    banti_64.timer = 0;
    banti_64.direction = 0;
    banti_64.disabled = 0;
    banti_64.alpha = 0.0f;

    lbRTC_TimeCopy(&banti_64.displayed_time, Common_GetPointer(time.rtc_time));
    lbRTC_TimeCopy(&banti_64.next_time, Common_GetPointer(time.rtc_time));

    play = (GAME_PLAY*)gamePT;
    if (play != NULL) {
        if (banti_64_check_disp_condition(play) == 0) {
            banti_64.mode = 2;
            banti_64.offset = banti_64_alpha_start;
        } else {
            banti_64.mode = 0;
            banti_64.offset = banti_64_alpha_start;
        }
    }
}

/* === banti_64_dt === */
/* Destructor: empty (4B - just blr) */
extern void banti_64_dt(void) {
}

/* === banti_64_check_disp_condition === */
/* Checks whether the N64-style banti should be displayed */
static int banti_64_check_disp_condition(GAME_PLAY* play) {
    int result;

    result = 1;

    if (mDemo_CheckDemo() == 0) {
        if (mEv_IsTitleDemo() <= 0) {
            if (banti_64.state_flag == 0) {
                goto check_play;
            }
        }
    }

    banti_64.addressable_type = 1;
    result = 0;
    goto done;

check_play:
    if (banti_64.addressable_type != 0) {
        result = 0;
    } else if (play->submenu.process_status != mSM_PROCESS_WAIT) {
        result = 0;
    }

done:
    return result;
}

/* === banti_64_disp_offset === */
/* Calculates display offset for position interpolation */
static void banti_64_disp_offset(GAME_PLAY* play) {
    f32 target;
    f32 fraction;

    if (banti_64.mode == 0) {
        target = 1.0f;
    } else {
        target = 0.0f;
    }

    fraction = 1.0f - sqrtf(0.8f);
    add_calc(&banti_64.alpha, target, fraction, 0.0425f, 0.0005f);
}

/* === banti_64_time_check === */
/* Checks for time changes and triggers animation transitions */
static void banti_64_time_check(void) {
    lbRTC_time_c* rtc_time = Common_GetPointer(time.rtc_time);
    int all_stopped;
    int now_hour, next_hour;
    int disp0, disp1;

    all_stopped = banti_64.month_anim | banti_64.day_anim |
                  banti_64.weekday_anim | banti_64.ampm_anim |
                  banti_64.hour_upper_anim | banti_64.hour_lower_anim |
                  banti_64.min_upper_anim | banti_64.min_lower_anim;

    if (all_stopped == 0) {
        /* Check for month change */
        if (banti_64.displayed_time.month != rtc_time->month) {
            banti_64.month_anim = 30;
        }

        /* Check for day change */
        if (banti_64.displayed_time.day != rtc_time->day) {
            banti_64.day_anim = 30;
        }

        /* Check for weekday change */
        if (banti_64.displayed_time.weekday != rtc_time->weekday) {
            banti_64.weekday_anim = 30;
        }

        /* Check for AM/PM change */
        {
            int disp_pm = banti_64.displayed_time.hour >= 12 ? 1 : 0;
            int rtc_pm = rtc_time->hour >= 12 ? 1 : 0;

            if (disp_pm != rtc_pm) {
                banti_64.ampm_anim = 30;
            }
        }

        /* Convert to 12-hour for comparison */
        now_hour = banti_64.displayed_time.hour;
        if (now_hour == 0 || now_hour == 12) {
            now_hour = 12;
        } else if (now_hour > 11) {
            now_hour -= 12;
        }

        next_hour = rtc_time->hour;
        if (next_hour == 0 || next_hour == 12) {
            next_hour = 12;
        } else if (next_hour > 11) {
            next_hour -= 12;
        }

        /* Check for hour tens digit change */
        disp0 = now_hour / 10;
        disp1 = next_hour / 10;
        if (disp0 != disp1) {
            banti_64.hour_upper_anim = 30;
        }

        /* Check for hour ones digit change */
        disp0 = now_hour % 10;
        disp1 = next_hour % 10;
        if (disp0 != disp1) {
            banti_64.hour_lower_anim = 30;
        }

        /* Check for minute tens digit change */
        disp0 = banti_64.displayed_time.min / 10;
        disp1 = rtc_time->min / 10;
        if (disp0 != disp1) {
            banti_64.min_upper_anim = 30;
        }

        /* Check for minute ones digit change */
        disp0 = banti_64.displayed_time.min % 10;
        disp1 = rtc_time->min % 10;
        if (disp0 != disp1) {
            banti_64.min_lower_anim = 30;
        }

        /* Copy next time if any animation started */
        if (banti_64.month_anim | banti_64.day_anim |
            banti_64.weekday_anim | banti_64.ampm_anim |
            banti_64.hour_upper_anim | banti_64.hour_lower_anim |
            banti_64.min_upper_anim | banti_64.min_lower_anim) {
            lbRTC_TimeCopy(&banti_64.next_time, rtc_time);
        }
    }

    /* Decrement animation counters */
    {
        int pre_state = banti_64.month_anim | banti_64.day_anim |
                        banti_64.weekday_anim | banti_64.ampm_anim |
                        banti_64.hour_upper_anim | banti_64.hour_lower_anim |
                        banti_64.min_upper_anim | banti_64.min_lower_anim;

        if (banti_64.month_anim > 0) banti_64.month_anim--;
        if (banti_64.day_anim > 0) banti_64.day_anim--;
        if (banti_64.weekday_anim > 0) banti_64.weekday_anim--;
        if (banti_64.ampm_anim > 0) banti_64.ampm_anim--;
        if (banti_64.hour_upper_anim > 0) banti_64.hour_upper_anim--;
        if (banti_64.hour_lower_anim > 0) banti_64.hour_lower_anim--;
        if (banti_64.min_upper_anim > 0) banti_64.min_upper_anim--;
        if (banti_64.min_lower_anim > 0) banti_64.min_lower_anim--;

        {
            int post_state = banti_64.month_anim | banti_64.day_anim |
                             banti_64.weekday_anim | banti_64.ampm_anim |
                             banti_64.hour_upper_anim | banti_64.hour_lower_anim |
                             banti_64.min_upper_anim | banti_64.min_lower_anim;

            /* All animations finished: commit the displayed time */
            if (pre_state != 0 && post_state == 0) {
                lbRTC_TimeCopy(&banti_64.displayed_time, &banti_64.next_time);
            }
        }
    }
}

/* === banti_64_move === */
/* Main move/update function for the N64-style banti (732B) */
extern void banti_64_move(GAME_PLAY* play) {
    int addressable_type;
    int update;

    banti_64_check_disp_condition(play);

    addressable_type = mPlib_Get_address_able_display();

    if (banti_64.addressable_type == addressable_type) {
        banti_64.timer = 0;
    } else {
        update = 0;
        banti_64.timer++;

        if (addressable_type == mPlayer_ADDRESSABLE_FALSE_READY_NET) {
            if (banti_64.timer > 18) {
                update = 1;
            }
        } else if (banti_64.addressable_type == mPlayer_ADDRESSABLE_TRUE) {
            if (banti_64.timer > 50 || addressable_type == mPlayer_ADDRESSABLE_FALSE_TALKING) {
                update = 1;
            }
        } else {
            if (banti_64.timer > 76 || addressable_type == mPlayer_ADDRESSABLE_FALSE_TALKING) {
                update = 1;
            }
        }

        if (update != 0) {
            banti_64.timer = 0;
            banti_64.addressable_type = addressable_type;
        }
    }

    banti_64_disp_offset(play);
    banti_64_time_check();
}

/* === banti_evw_anime_texanime_tukihi === */
/* Texture animation for month/day (tukihi) display using EVW system (192B).
 * Selects the appropriate date digit texture based on the animation
 * countdown frame, blending between current and next displayed value. */
static void banti_evw_anime_texanime_tukihi(int segment) {
    static u8* hiniti_tex_table[31] = {
        clk_win_suuji1_TA_tex_txt, clk_win_suuji2_TA_tex_txt, clk_win_suuji3_TA_tex_txt,
        clk_win_suuji4_TA_tex_txt, clk_win_suuji5_TA_tex_txt, clk_win_suuji6_TA_tex_txt,
        clk_win_suuji7_TA_tex_txt, clk_win_suuji8_TA_tex_txt, clk_win_suuji9_TA_tex_txt,
        clk_win_suuji10_TA_tex_txt, clk_win_suuji11_TA_tex_txt, clk_win_suuji12_TA_tex_txt
    };

    GAME_PLAY* play;
    GRAPH* graph;
    int frame;

    play = (GAME_PLAY*)gamePT;
    if (play == NULL) {
        return;
    }

    graph = play->game.graph;
    frame = play->game_frame;

    OPEN_DISP(graph);

    if (segment == 0) {
        /* Month */
        u8 tex_idx = banti_64.displayed_time.month - 1;
        if (tex_idx < 12) {
            gSPSegment(NOW_POLY_OPA_DISP++, G_MWO_SEGMENT_8, hiniti_tex_table[tex_idx]);
            gSPSegment(NOW_FONT_DISP++, G_MWO_SEGMENT_8, hiniti_tex_table[tex_idx]);
        }
        if (banti_64.month_anim > 0) {
            u8 next_idx = banti_64.next_time.month - 1;
            if (next_idx < 12) {
                gSPSegment(NOW_POLY_OPA_DISP++, G_MWO_SEGMENT_9, hiniti_tex_table[next_idx]);
                gSPSegment(NOW_FONT_DISP++, G_MWO_SEGMENT_9, hiniti_tex_table[next_idx]);
            }
        }
    } else {
        /* Day */
        u8 tex_idx = banti_64.displayed_time.day - 1;
        if (tex_idx < 31) {
            gSPSegment(NOW_POLY_OPA_DISP++, G_MWO_SEGMENT_8, hiniti_tex_table[tex_idx]);
            gSPSegment(NOW_FONT_DISP++, G_MWO_SEGMENT_8, hiniti_tex_table[tex_idx]);
        }
        if (banti_64.day_anim > 0) {
            u8 next_idx = banti_64.next_time.day - 1;
            if (next_idx < 31) {
                gSPSegment(NOW_POLY_OPA_DISP++, G_MWO_SEGMENT_9, hiniti_tex_table[next_idx]);
                gSPSegment(NOW_FONT_DISP++, G_MWO_SEGMENT_9, hiniti_tex_table[next_idx]);
            }
        }
    }

    CLOSE_DISP(graph);
}

/* === banti_evw_anime_texanime_jikan === */
/* Texture animation for time (jikan) digit display (196B).
 * Handles the hour/minute digit texture selection and animation
 * transitions when the time changes. */
static void banti_evw_anime_texanime_jikan(int segment) {
    static u8* jikan_tex_table[11] = {
        clk_win_jikan0_TA_tex_txt, clk_win_jikan1_TA_tex_txt, clk_win_jikan2_TA_tex_txt,
        clk_win_jikan3_TA_tex_txt, clk_win_jikan4_TA_tex_txt, clk_win_jikan5_TA_tex_txt,
        clk_win_jikan6_TA_tex_txt, clk_win_jikan7_TA_tex_txt, clk_win_jikan8_TA_tex_txt,
        clk_win_jikan9_TA_tex_txt, clk_win_jikan_TA_tex_txt
    };

    GAME_PLAY* play;
    GRAPH* graph;
    int now_hour, next_hour;
    int idx0, idx1;

    play = (GAME_PLAY*)gamePT;
    if (play == NULL) {
        return;
    }

    graph = play->game.graph;

    /* Convert to 12-hour format */
    now_hour = banti_64.displayed_time.hour;
    if (now_hour == 0 || now_hour == 12) {
        now_hour = 12;
    } else if (now_hour > 11) {
        now_hour -= 12;
    }

    next_hour = banti_64.next_time.hour;
    if (next_hour == 0 || next_hour == 12) {
        next_hour = 12;
    } else if (next_hour > 11) {
        next_hour -= 12;
    }

    OPEN_DISP(graph);

    switch (segment) {
        case 0: /* Hour tens */
            idx0 = now_hour / 10;
            idx1 = next_hour / 10;
            if (idx0 == 0) idx0 = 10; /* blank for leading zero */
            if (idx1 == 0) idx1 = 10;
            gSPSegment(NOW_POLY_OPA_DISP++, G_MWO_SEGMENT_8, jikan_tex_table[idx0]);
            gSPSegment(NOW_FONT_DISP++, G_MWO_SEGMENT_8, jikan_tex_table[idx0]);
            if (banti_64.hour_upper_anim > 0) {
                gSPSegment(NOW_POLY_OPA_DISP++, G_MWO_SEGMENT_9, jikan_tex_table[idx1]);
                gSPSegment(NOW_FONT_DISP++, G_MWO_SEGMENT_9, jikan_tex_table[idx1]);
            }
            break;

        case 1: /* Hour ones */
            idx0 = now_hour % 10;
            idx1 = next_hour % 10;
            gSPSegment(NOW_POLY_OPA_DISP++, G_MWO_SEGMENT_8, jikan_tex_table[idx0]);
            gSPSegment(NOW_FONT_DISP++, G_MWO_SEGMENT_8, jikan_tex_table[idx0]);
            if (banti_64.hour_lower_anim > 0) {
                gSPSegment(NOW_POLY_OPA_DISP++, G_MWO_SEGMENT_9, jikan_tex_table[idx1]);
                gSPSegment(NOW_FONT_DISP++, G_MWO_SEGMENT_9, jikan_tex_table[idx1]);
            }
            break;

        case 2: /* Minute tens */
            idx0 = banti_64.displayed_time.min / 10;
            idx1 = banti_64.next_time.min / 10;
            gSPSegment(NOW_POLY_OPA_DISP++, G_MWO_SEGMENT_8, jikan_tex_table[idx0]);
            gSPSegment(NOW_FONT_DISP++, G_MWO_SEGMENT_8, jikan_tex_table[idx0]);
            if (banti_64.min_upper_anim > 0) {
                gSPSegment(NOW_POLY_OPA_DISP++, G_MWO_SEGMENT_9, jikan_tex_table[idx1]);
                gSPSegment(NOW_FONT_DISP++, G_MWO_SEGMENT_9, jikan_tex_table[idx1]);
            }
            break;

        case 3: /* Minute ones */
            idx0 = banti_64.displayed_time.min % 10;
            idx1 = banti_64.next_time.min % 10;
            gSPSegment(NOW_POLY_OPA_DISP++, G_MWO_SEGMENT_8, jikan_tex_table[idx0]);
            gSPSegment(NOW_FONT_DISP++, G_MWO_SEGMENT_8, jikan_tex_table[idx0]);
            if (banti_64.min_lower_anim > 0) {
                gSPSegment(NOW_POLY_OPA_DISP++, G_MWO_SEGMENT_9, jikan_tex_table[idx1]);
                gSPSegment(NOW_FONT_DISP++, G_MWO_SEGMENT_9, jikan_tex_table[idx1]);
            }
            break;
    }

    CLOSE_DISP(graph);
}

/* === banti_evw_anime_texanime === */
/* General texture animation using EVW_ANIME_TEXANIME data (148B).
 * Resolves which texture frame to display at the given animation
 * frame, then sets the segment pointer for the specified segment. */
static void banti_evw_anime_texanime(GAME_PLAY* play, Gfx* gfx, int segment, EVW_ANIME_TEXANIME* texanime, int frame, int count) {
    GRAPH* graph;
    int anim_frame;
    void* tex_p;

    if (texanime == NULL || play == NULL) {
        return;
    }

    graph = play->game.graph;
    anim_frame = frame % (texanime->frame_count * 2);
    tex_p = texanime->texture_tbl[texanime->animation_pattern[anim_frame / 2]];

    OPEN_DISP(graph);

    gSPSegment(NOW_POLY_OPA_DISP++, segment, tex_p);
    gSPSegment(NOW_POLY_XLU_DISP++, segment, tex_p);
    gSPSegment(NOW_FONT_DISP++, segment, tex_p);

    CLOSE_DISP(graph);
}

/* === Banti_Anime_Set === */
/* Sets animation parameters by iterating EVW_ANIME_DATA entries (152B).
 * This is the N64-style equivalent of Evw_Anime_Set, used specifically
 * for the banti clock display. It processes the animation data array
 * and applies texture animations for each segment. */
static void Banti_Anime_Set(GAME_PLAY* play, Gfx* gfx, EVW_ANIME_DATA* evw_anime_data, int frame) {
    int segment;

    if (evw_anime_data == NULL) {
        return;
    }

    segment = evw_anime_data->segment;
    if (segment == 0) {
        return;
    }

    do {
        segment = evw_anime_data->segment;

        if (evw_anime_data->type == EVW_ANIME_TYPE_TEXANIME) {
            banti_evw_anime_texanime(play, gfx, G_MWO_SEGMENT_7 + ABS(segment),
                                     (EVW_ANIME_TEXANIME*)evw_anime_data->data_p,
                                     frame, 0);
        }

        evw_anime_data++;
    } while (segment >= 0);
}

/* === banti_64_draw === */
/* Main draw function for the N64-style banti clock HUD (1468B).
 * Renders the clock display using N64-era GBI display list commands.
 * Draws weekday indicator, AM/PM, date, time digits, and colon blink. */
extern void banti_64_draw(GAME_PLAY* play) {
    static u8* week_tex_table[lbRTC_WEEK] = {
        clk_win_sun_tex_rgb_ia8, clk_win_mon_tex_rgb_ia8, clk_win_tue_tex_rgb_ia8,
        clk_win_wed_tex_rgb_ia8, clk_win_thu_tex_rgb_ia8, clk_win_fri_tex_rgb_ia8,
        clk_win_sat_tex_rgb_ia8
    };

    static u8* ampm_tex_table[2] = {
        clk_win_am_tex_rgb_ia8, clk_win_pm_tex_rgb_ia8
    };

    static u8* jikan_tex_table[11] = {
        clk_win_jikan0_TA_tex_txt, clk_win_jikan1_TA_tex_txt, clk_win_jikan2_TA_tex_txt,
        clk_win_jikan3_TA_tex_txt, clk_win_jikan4_TA_tex_txt, clk_win_jikan5_TA_tex_txt,
        clk_win_jikan6_TA_tex_txt, clk_win_jikan7_TA_tex_txt, clk_win_jikan8_TA_tex_txt,
        clk_win_jikan9_TA_tex_txt, clk_win_jikan_TA_tex_txt
    };

    static u8* hiniti_tex_table[12] = {
        clk_win_suuji1_TA_tex_txt, clk_win_suuji2_TA_tex_txt, clk_win_suuji3_TA_tex_txt,
        clk_win_suuji4_TA_tex_txt, clk_win_suuji5_TA_tex_txt, clk_win_suuji6_TA_tex_txt,
        clk_win_suuji7_TA_tex_txt, clk_win_suuji8_TA_tex_txt, clk_win_suuji9_TA_tex_txt,
        clk_win_suuji10_TA_tex_txt, clk_win_suuji11_TA_tex_txt, clk_win_suuji12_TA_tex_txt
    };

    GRAPH* graph;
    int poly_render;
    Mtx* m;
    int alpha;
    int is_pm;
    int now_hour, next_hour;

    if (banti_64.mode == 2) {
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

    OPEN_DISP(graph);

    {
        Gfx* gfx;
        Gfx** gfx_pp;

        if (poly_render == 1) {
            gfx = NOW_POLY_OPA_DISP;
        } else {
            gfx = NOW_FONT_DISP;
        }
        gfx_pp = &gfx;

        /* Set up projection matrix */
        gSPMatrix(gfx_pp[0]++, m, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_PROJECTION);

        /* Set up model matrix with scale */
        Matrix_scale(16.0f, 16.0f, 16.0f, 0);

        if (banti_64.direction == 1) {
            Matrix_translate(-184.0f, 0.0f, 0.0f, 1);
        }

        {
            f32 offset_x;
            f32 base_x;

            if (banti_64.disabled != 0) {
                base_x = banti_64_alpha_full;
            } else {
                base_x = banti_64_alpha_start;
            }

            offset_x = base_x;
            if (banti_64.direction == 1) {
                offset_x = -base_x;
            }

            {
                f32 move_amount;

                move_amount = offset_x - banti_64.offset;
                Matrix_translate(move_amount, 0.0f, 0.0f, 1);
            }
        }

        /* Set model view matrix */
        gSPMatrix(gfx_pp[0]++, _Matrix_to_Mtx_new(graph), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);

        /* Set rendering mode */
        gSPDisplayList(gfx_pp[0]++, clk_win_mode);
        gDPSetRenderMode(gfx_pp[0]++, G_RM_PASS, G_RM_XLU_SURF2);

        alpha = (int)(banti_64.alpha * 255.0f);

        /* Draw weekday */
        if (banti_64.displayed_time.weekday == lbRTC_SUNDAY) {
            gDPSetPrimColor(gfx_pp[0]++, 0, (u8)alpha, 255, 255, 225, (u8)alpha);
            gDPSetEnvColor(gfx_pp[0]++, 200, 60, 255, (u8)alpha);
        } else {
            gDPSetPrimColor(gfx_pp[0]++, 0, (u8)alpha, 255, 255, 225, (u8)alpha);
            gDPSetEnvColor(gfx_pp[0]++, 70, 140, 50, (u8)alpha);
        }

        gSPSegment(gfx_pp[0]++, G_MWO_SEGMENT_9, week_tex_table[banti_64.displayed_time.weekday]);
        gSPDisplayList(gfx_pp[0]++, clk_win_youbiT_model);

        /* Draw AM/PM */
        is_pm = banti_64.displayed_time.hour >= 12 ? 1 : 0;

        gDPSetPrimColor(gfx_pp[0]++, 0, (u8)alpha, 255, 255, 120, (u8)alpha);
        gDPSetEnvColor(gfx_pp[0]++, 70, 10, 10, (u8)alpha);
        gSPSegment(gfx_pp[0]++, G_MWO_SEGMENT_A, ampm_tex_table[is_pm]);
        gSPDisplayList(gfx_pp[0]++, clk_win_ampmT_model);

        /* Draw date (month/day) */
        gDPSetPrimColor(gfx_pp[0]++, 0, (u8)alpha, 235, 255, 120, (u8)alpha);
        gDPSetEnvColor(gfx_pp[0]++, 80, 40, 40, (u8)alpha);

        {
            u8 month_idx = banti_64.displayed_time.month - 1;
            if (month_idx < 12) {
                gSPSegment(gfx_pp[0]++, G_MWO_SEGMENT_8, hiniti_tex_table[month_idx]);
            }
        }

        {
            u8 day_idx = banti_64.displayed_time.day - 1;
            if (day_idx < 12) {
                gSPSegment(gfx_pp[0]++, G_MWO_SEGMENT_9, hiniti_tex_table[day_idx]);
            }
        }

        /* Draw time digits */
        gDPSetPrimColor(gfx_pp[0]++, 0, (u8)alpha, 255, 255, 255, (u8)alpha);
        gDPSetEnvColor(gfx_pp[0]++, 60, 25, 10, (u8)alpha);

        now_hour = banti_64.displayed_time.hour;
        if (now_hour == 0 || now_hour == 12) {
            now_hour = 12;
        } else if (now_hour > 11) {
            now_hour -= 12;
        }

        /* Hour tens */
        {
            int idx = now_hour / 10;
            if (idx == 0) idx = 10; /* blank for leading zero */
            gSPSegment(gfx_pp[0]++, G_MWO_SEGMENT_8, jikan_tex_table[idx]);
        }

        /* Hour ones */
        {
            int idx = now_hour % 10;
            gSPSegment(gfx_pp[0]++, G_MWO_SEGMENT_9, jikan_tex_table[idx]);
        }

        /* Colon blink (odd seconds) */
        if ((Common_Get(time.rtc_time).sec & 1) == 1) {
            gDPSetPrimColor(gfx_pp[0]++, 0, (u8)alpha, 215, 120, 0, (u8)alpha);
            gDPSetEnvColor(gfx_pp[0]++, 70, 50, 50, (u8)alpha);
            gSPDisplayList(gfx_pp[0]++, clk_win_maru2T_model);
        }

        /* Minute tens */
        {
            int idx = banti_64.displayed_time.min / 10;
            gSPSegment(gfx_pp[0]++, G_MWO_SEGMENT_8, jikan_tex_table[idx]);
        }

        /* Minute ones */
        {
            int idx = banti_64.displayed_time.min % 10;
            gSPSegment(gfx_pp[0]++, G_MWO_SEGMENT_9, jikan_tex_table[idx]);
        }

        /* Circle/dot decoration */
        gDPSetPrimColor(gfx_pp[0]++, 0, (u8)alpha, 255, 255, 0, (u8)alpha);
        gDPSetEnvColor(gfx_pp[0]++, 70, 50, 50, (u8)alpha);
        gSPDisplayList(gfx_pp[0]++, clk_win_maruT_model);

        /* Store back the display list pointer */
        if (poly_render == 1) {
            SET_POLY_OPA_DISP(*gfx_pp);
        } else {
            SET_FONT_DISP(*gfx_pp);
        }
    }

    CLOSE_DISP(graph);
}

#endif /* VERSION >= VER_DELUXE */
