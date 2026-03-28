/**
 * pc_deluxe_stubs.c - Stubs for VER_DELUXE symbols not yet decompiled or
 * originating from ROM object banks / linker-generated addresses.
 *
 * These resolve undefined-symbol linker errors in the PC port.
 */
#include "types.h"
#include "graph.h"
#include "c_keyframe.h"
#include "m_actor_type.h"
#include "m_name_table.h"
#include "m_home_h.h"
#include "m_museum_display.h"
#include "m_private.h"
#include "libc64/qrand.h"
#include <string.h>

/* ======================================================================
 * Category 1: __float_max  (Metrowerks CW runtime symbol)
 *
 * On GC, this lives in .sdata and is referenced via the FLT_MAX macro
 * in sys_math.h when VERSION >= VER_GAFU01_00.  GCC/Clang don't emit
 * it, so we provide the value here.
 * ====================================================================== */

unsigned long __float_max[] = { 0x7F7FFFFF }; /* IEEE-754 FLT_MAX */

/* ======================================================================
 * Category 2: Function stubs  (VER_DELUXE-only, not yet decompiled)
 * ====================================================================== */

/* m_museum_display: identify a fossil -> returns a random identified fossil.
 * 25 fossils total (FTR_DIN_TRIKERA_HEAD through FTR_DIN_TRILOBITE). */
mActor_name_t mMmd_IdentifyFossil(mActor_name_t item) {
    if (item != ITM_FOSSIL) {
        return EMPTY_NO;
    }

    /* Return a random identified fossil from the 25 fossil set */
    return FTR_START(FTR_DIN_TRIKERA_HEAD) + (mActor_name_t)(qrand() % mMmd_FOSSIL_NUM);
}

/* m_collision_bg: 2D line-segment intersection test in XZ plane.
 * Returns non-zero if segments (p0->p1) and (p2->p3) intersect. */
int mCoBG_CheckLineSegmentIntersection_XZ(const xyz_t* p0, const xyz_t* p1,
                                           const xyz_t* p2, const xyz_t* p3) {
    /* Standard cross-product line segment intersection test */
    float d1x = p1->x - p0->x;
    float d1z = p1->z - p0->z;
    float d2x = p3->x - p2->x;
    float d2z = p3->z - p2->z;
    float denom = d1x * d2z - d1z * d2x;
    float dx = p2->x - p0->x;
    float dz = p2->z - p0->z;
    float t, u;

    if (denom > -0.001f && denom < 0.001f) {
        return 0; /* parallel */
    }

    t = (dx * d2z - dz * d2x) / denom;
    u = (dx * d1z - dz * d1x) / denom;

    return (t >= 0.0f && t <= 1.0f && u >= 0.0f && u <= 1.0f) ? 1 : 0;
}

/* bg_item_common: water a flower at the given block/unit coords.
 * The static version in bg_item_common.c_inc takes const u16* params;
 * the extern declaration in m_player.c uses int params. Provide the
 * extern-linkage stub matching the m_player.c declaration. */
void bg_item_common_water_flower(int bx, int bz, int ut_x, int ut_z) {
    (void)bx; (void)bz; (void)ut_x; (void)ut_z;
}

/* m_home: save-data version upgrade helpers (card upgrade). */
void mHm_CheckVersionAndUpdate(mHm_hs_c* home) {
    (void)home;
}

void mHm_UpdateCottageVersion_V0_V1(mHm_cottage_c* cottage) {
    (void)cottage;
}

/* dolphin/si: serial interface response (GC hardware). */
BOOL SIGetResponse(s32 chan, void* data) {
    (void)chan; (void)data;
    return FALSE;
}

/* ======================================================================
 * Category 3: ROM data stubs  (object bank models / skeletons / animations)
 *
 * These symbols are normally loaded at runtime from the disc's REL
 * module or object archive.  On PC they need to exist so the linker
 * is happy; the actual data is loaded from extracted assets.
 * ====================================================================== */

/* --- Monument models (ac_monument.c) --- */

Gfx obj_monument_clock_model[]       = { gsSPEndDisplayList() };
Gfx obj_monument_flowerclock_model[] = { gsSPEndDisplayList() };
Gfx obj_monument_windmill_model[]    = { gsSPEndDisplayList() };
Gfx obj_monument_fountain_model[]    = { gsSPEndDisplayList() };
Gfx obj_monument_statue_model[]      = { gsSPEndDisplayList() };
Gfx obj_monument_bench_model[]       = { gsSPEndDisplayList() };
Gfx obj_monument_streetlight_model[] = { gsSPEndDisplayList() };
Gfx obj_monument_bell_model[]        = { gsSPEndDisplayList() };

/* Monument skeleton/animation data (zero-initialized is safe; the
 * skeleton pointer will be NULL-checked before use). */
cKF_Skeleton_R_c  cKF_bs_r_obj_monument_clock       = {0};
cKF_Animation_R_c cKF_ba_r_obj_monument_clock        = {0};
cKF_Skeleton_R_c  cKF_bs_r_obj_monument_flowerclock  = {0};
cKF_Animation_R_c cKF_ba_r_obj_monument_flowerclock   = {0};
cKF_Skeleton_R_c  cKF_bs_r_obj_monument_windmill     = {0};
cKF_Animation_R_c cKF_ba_r_obj_monument_windmill      = {0};

/* --- NPC data tables (m_npc.c) --- */

/* model_skeleton: array of skeleton pointers indexed by species_sub_idx.
 * 256 entries covers all NPC species. NULL entries are safe (checked). */
cKF_Skeleton_R_c* model_skeleton[256] = {0};

/* npc_md_table: model data index table. Zero entries map to model 0. */
u16 npc_md_table[512] = {0};

/* --- Display list sub-address symbols (diary/score overlay) --- */

/* These are VER_GAFU01_00/VER_DELUXE offset-based symbols from the
 * kei_hyouji and onp_hyouji2 model data.  The NTSC-U (GAFE01_00)
 * variants are defined in dia_hyouji.c; these PAL/Deluxe variants
 * need separate stubs. */
Gfx kei_win_b2_model_1_data_407600[]      = { gsSPEndDisplayList() };
Gfx kei_win_amojiT_model_1_data_407680[]  = { gsSPEndDisplayList() };
Gfx onp_win_rmoji_model_1_data_4A4020[]   = { gsSPEndDisplayList() };

/* --- Logo model (ac_animal_logo.c, PAL/Deluxe copyright screen) --- */
Gfx logo_nin_copyT_model[] = { gsSPEndDisplayList() };

/* aEVD_act_follow, aEVD_backup_from_player, aEVD_teleport_to_player,
 * aEVD_draw_monument_outline: now defined in ac_ev_dokutu_talk.c_inc
 * (included by ac_ev_dokutu.c) — stubs removed. */

/* --- m_player_lib.c: cloth texture loading size query --- */
void mPlib_Load_PlayerTexAndPallet_size(void* tex_p, void* pal_p, int idx, int* tex_size_p, int* pal_size_p) {
    /* Standard design texture = 32x32 CI4 = 0x200 bytes tex + 0x20 bytes pal.
     * If tex_p/pal_p are NULL, just return sizes (used for size queries). */
    int tex_size = 0x200; /* mNW_DESIGN_TEX_SIZE */
    int pal_size = 0x20;  /* mNW_PALETTE_SIZE */

    if (idx < 0 || idx >= 0x116) {
        tex_size = 0;
        pal_size = 0;
    } else if (tex_p == NULL || pal_p == NULL) {
        /* Size-only query: return standard sizes */
    } else {
        /* TODO: actual cloth DMA from ROM - for now just zero the buffers */
        memset(tex_p, 0, tex_size);
        memset(pal_p, 0, pal_size);
    }

    if (tex_size_p) *tex_size_p = tex_size;
    if (pal_size_p) *pal_size_p = pal_size;
}

/* ======================================================================
 * Category 4: Phase-1 stubs for functions defined in modified existing files.
 *
 * These will be replaced with real implementations in Phase 2 when
 * the Deluxe conditional blocks are applied to m_field_info.c,
 * m_lights.c, evw_anime.c, m_name_table.c, and m_eappli.c.
 * ====================================================================== */

#include "m_field_info.h"
#include "m_lights.h"
#include "evw_anime.h"
#include "m_eappli.h"

/* m_field_info.c: find unit in block by world position */
mActor_name_t* mFI_Wpos2BkandUtNuminBlock_ref(int* bx, int* bz, int* ut_x, int* ut_z, xyz_t* wpos) {
    (void)bx; (void)bz; (void)ut_x; (void)ut_z; (void)wpos;
    return NULL;
}

/* m_field_info.c: get top foreground item pointer for a block */
mActor_name_t* mFI_BkNum2UtFGTop_field(int bx, int bz) {
    (void)bx; (void)bz;
    return NULL;
}

/* m_lights.c: set diffuse light direction/color */
void Light_diffuse_set(Lights* lights, u8 r, u8 g, u8 b, s16 x, s16 y, s16 z) {
    (void)lights; (void)r; (void)g; (void)b; (void)x; (void)y; (void)z;
}

/* evw_anime.c: set animated event with specific frame */
void Evw_Anime_Set_Param(GAME_PLAY* play, EVW_ANIME_DATA* evw_anime_data, u32 frame) {
    (void)play; (void)evw_anime_data; (void)frame;
}

/* m_name_table.c: get stacked food item for given food index and count */
mActor_name_t mNT_FoodCount2FoodItem(int food_idx, int count) {
    (void)food_idx; (void)count;
    return EMPTY_NO;
}

/* m_eappli.c: compute CRC16 of data buffer */
u16 mEA_getcrc16(u8* data, int size) {
    (void)data; (void)size;
    return 0;
}

/* m_museum.c: check fossil count and send purchase info mail if threshold met.
 * Safe stub: mMmd_CountDisplayedFossil() will be 0 on a new game. */
#include "m_museum.h"
void mMsm_CheckSendPurchaseInfoMail() {}

/* m_kabu_manager.c: Deluxe half-day Stalk Market price functions. */
#include "m_kabu_manager.h"
void Kabu_decide_price_schedule_new() {}
u16  Kabu_get_price_new() { return 100; }
void Kabu_manager_new() {}

/* ======================================================================
 * Category 5: Deluxe private-data stubs (Phase 1)
 *
 * storage_item and skin_tone_locked are NOT embedded in Private_c to
 * avoid growing Save_t and corrupting GCI field offsets.  The overlay
 * files that need them read/write these globals instead.
 * ====================================================================== */
mActor_name_t g_dlx_storage_item[mPr_STORAGE_SLOT_COUNT];
BOOL g_dlx_skin_tone_locked = FALSE;
