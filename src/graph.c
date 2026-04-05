#include "graph.h"

#include "audio.h"
#include "dvderr.h"
#include "famicom_emu.h"
#include "first_game.h"
#include "game.h"
#include "irqmgr.h"
#include "libc64/malloc.h"
#include "libforest/emu64/emu64_wrapper.h"
#include "jsyswrap.h"
#include "libu64/debug.h"
#include "libultra/libultra.h"
#include "m_bgm.h"
#include "m_common_data.h"
#include "m_debug.h"
#include "m_game_dlftbls.h"
#include "sys_math.h"
#include "m_play.h"
#include "m_prenmi.h"
#include "m_select.h"
#include "m_trademark.h"
#include "m_vibctl.h"
#include "player_select.h"
#include "save_menu.h"
#include "second_game.h"
#include "sys_dynamic.h"
#include "sys_ucode.h"
#include "zurumode.h"
#ifdef TARGET_PC
#include "m_card.h"
#include "pc_model_viewer.h"
#include "pc_diag.h"
#include "pc_platform.h"
#include "pc_settings.h"
#include "pc_snapshot.h"
#include <setjmp.h>
extern int g_pc_running;
#endif

GRAPH graph_class;

static int skip_frame; // TODO: this is actually declared in graph_main
#if VERSION != VER_GAFU01_00
u8 SoftResetEnable;
#endif
static int frame; // TODO: this is actually declared in graph_task_set00

#ifdef TARGET_PC
#define CONSTRUCT_THA_GA(tha_ga, name, name2) (THA_GA_ct((tha_ga), sys_dynamic.name, name2 ## _SIZE * sizeof(Gfx)))
#else
#define CONSTRUCT_THA_GA(tha_ga, name, name2) (THA_GA_ct((tha_ga), sys_dynamic.##name, ##name2##_SIZE * sizeof(Gfx)))
#endif

static void graph_setup_double_buffer(GRAPH* this) {
    bzero(&sys_dynamic, sizeof(dynamic_t));
    sys_dynamic.start_magic = SYSDYNAMIC_START_MAGIC;
    sys_dynamic.end_magic = SYSDYNAMIC_END_MAGIC;

    CONSTRUCT_THA_GA(&this->bg_opaque_thaga, new0, NEW0);
    CONSTRUCT_THA_GA(&this->bg_translucent_thaga, new1, NEW1);
    CONSTRUCT_THA_GA(&this->polygon_opaque_thaga, poly_opa, POLY_OPA);
    CONSTRUCT_THA_GA(&this->polygon_translucent_thaga, poly_xlu, POLY_XLU);
    CONSTRUCT_THA_GA(&this->overlay_thaga, overlay, OVERLAY);
    CONSTRUCT_THA_GA(&this->work_thaga, work, WORK);
    CONSTRUCT_THA_GA(&this->font_thaga, font, FONT);
    CONSTRUCT_THA_GA(&this->shadow_thaga, shadow, SHADOW);
    CONSTRUCT_THA_GA(&this->light_thaga, light, LIGHT);

    this->Gfx_list10 = sys_dynamic.new0;
    this->Gfx_list11 = sys_dynamic.new1;
    this->Gfx_list00 = sys_dynamic.poly_opa;
    this->Gfx_list01 = sys_dynamic.poly_xlu;
    this->Gfx_list04 = sys_dynamic.overlay;
    this->Gfx_list05 = sys_dynamic.work;
    this->Gfx_list07 = sys_dynamic.font;
    this->Gfx_list08 = sys_dynamic.shadow;
    this->Gfx_list09 = sys_dynamic.light;

    this->gfxsave = NULL;
}

#define ARE_INIT_PROCS_EQUAL(proc0, proc1) (((void (*)(GAME*))proc0) == ((void (*)(GAME*))proc1))
static DLFTBL_GAME* game_get_next_game_dlftbl(GAME* game) {
    void (*next_game_init_proc)(GAME*) = game_get_next_game_init(game);

    if (ARE_INIT_PROCS_EQUAL(next_game_init_proc, first_game_init)) {
        return &game_dlftbls[0];
    } else if (ARE_INIT_PROCS_EQUAL(next_game_init_proc, select_init)) {
        return &game_dlftbls[1];
    } else if (ARE_INIT_PROCS_EQUAL(next_game_init_proc, play_init)) {
        return &game_dlftbls[2];
    } else if (ARE_INIT_PROCS_EQUAL(next_game_init_proc, second_game_init)) {
        return &game_dlftbls[3];
    } else if (ARE_INIT_PROCS_EQUAL(next_game_init_proc, trademark_init)) {
        return &game_dlftbls[5];
    } else if (ARE_INIT_PROCS_EQUAL(next_game_init_proc, player_select_init)) {
        return &game_dlftbls[6];
    } else if (ARE_INIT_PROCS_EQUAL(next_game_init_proc, save_menu_init)) {
        return &game_dlftbls[7];
    } else if (ARE_INIT_PROCS_EQUAL(next_game_init_proc, famicom_emu_init)) {
        return &game_dlftbls[8];
    } else if (ARE_INIT_PROCS_EQUAL(next_game_init_proc, prenmi_init)) {
        return &game_dlftbls[9];
    }
#ifdef TARGET_PC
    else if (ARE_INIT_PROCS_EQUAL(next_game_init_proc, pc_model_viewer_init)) {
        return &game_dlftbls[10];
    }
#endif

    return NULL;
}

extern void graph_ct(GRAPH* this) {
    bzero(this, sizeof(GRAPH));
    this->frame_counter = 0;
    this->cfb_bank = 0;
    SETREG(SREG, 33, GETREG(SREG, 33) & ~2);
    SETREG(SREG, 33, GETREG(SREG, 33) & ~1);
    zurumode_init();
    GRAPH_SET_DOING_POINT(this, CT);
}

extern void graph_dt(GRAPH* this) {
    GRAPH_SET_DOING_POINT(this, DT);
    zurumode_cleanup();
}

static void graph_task_set00(GRAPH* this) {
    ucode_info ucode[2];

    GRAPH_SET_DOING_POINT(this, WAIT_TASK);
    GRAPH_SET_DOING_POINT(this, WAIT_TASK_FINISHED);
    if (ResetStatus < IRQ_RESET_DELAY) {
        this->last_dl = this->Gfx_list05;
        if (this->taskEndCallback != NULL) {
            this->taskEndCallback(this, this->taskEndData);
        }

        if (ResetStatus < IRQ_RESET_DELAY) {
            ucode[0].type = UCODE_TYPE_POLY_TEXT;
            ucode[1].type = UCODE_TYPE_SPRITE_TEXT;
            ucode[0].ucode_p = ucode_GetPolyTextStart();
            ucode[1].ucode_p = ucode_GetSpriteTextStart();
            JW_BeginFrame();
            emu64_init();
            emu64_set_ucode_info(2, ucode);
            emu64_set_first_ucode(ucode[0].ucode_p);
            PC_DIAG(3, "graph_task_set00: emu64_taskstart(Gfx_list05=%p)\n", (void*)this->Gfx_list05);
            emu64_taskstart(this->Gfx_list05); /* work data */
#ifdef TARGET_PC
            {
                extern int pc_emu64_frame_cmds, pc_emu64_frame_tri_cmds, pc_emu64_frame_vtx_cmds;
                extern int pc_emu64_frame_dl_cmds, pc_emu64_frame_crashes;
                extern int pc_gx_draw_call_count;
                PC_DIAG(5, "emu64 stats: cmds=%d tri=%d vtx=%d gl_draws=%d\n",
                        pc_emu64_frame_cmds, pc_emu64_frame_tri_cmds, pc_emu64_frame_vtx_cmds,
                        pc_gx_draw_call_count);
            }
#endif
            emu64_cleanup();
            JW_EndFrame();
            frame++;
        }
    }
}

static int graph_draw_finish(GRAPH* this) {
    int err;
    OPEN_DISP(this);

    gSPBranchList(NOW_WORK_DISP++, this->Gfx_list10);
    gSPBranchList(NOW_BG_OPA_DISP++, this->Gfx_list08);
    gSPBranchList(NOW_SHADOW_DISP++, this->Gfx_list11);
    gSPBranchList(NOW_BG_XLU_DISP++, this->Gfx_list00);
    gSPBranchList(NOW_POLY_OPA_DISP++, this->Gfx_list01);
    gSPBranchList(NOW_POLY_XLU_DISP++, this->Gfx_list09);
    gSPBranchList(NOW_LIGHT_DISP++, this->Gfx_list07);
    gSPBranchList(NOW_FONT_DISP++, this->Gfx_list04);
    gDPPipeSync(NOW_OVERLAY_DISP++);
    gDPFullSync(NOW_OVERLAY_DISP++);
    gSPEndDisplayList(NOW_OVERLAY_DISP++);

    CLOSE_DISP(this);
    err = FALSE;

    SYSDYNAMIC_OPEN();
    if (!SYSDYNAMIC_CHECK_START()) {
#if VERSION == VER_GAFU01_00
        _dbg_hungup(__FILE__, 416);
#elif VERSION == VER_GAFE01_00
        _dbg_hungup(__FILE__, 417);
#endif
    }

    if (!SYSDYNAMIC_CHECK_END()) {
        err = TRUE;
#if VERSION == VER_GAFU01_00
        _dbg_hungup(__FILE__, 424);
#elif VERSION == VER_GAFE01_00
        _dbg_hungup(__FILE__, 425);
#endif
    }
    SYSDYNAMIC_CLOSE();

    if (THA_GA_isCrash(&this->polygon_opaque_thaga)) {
        err = TRUE;
    }

    if (THA_GA_isCrash(&this->polygon_translucent_thaga)) {
        err = TRUE;
    }

    if (THA_GA_isCrash(&this->overlay_thaga)) {
        err = TRUE;
    }

    if (THA_GA_isCrash(&this->font_thaga)) {
        err = TRUE;
    }

    if (THA_GA_isCrash(&this->shadow_thaga)) {
        err = TRUE;
    }

    if (THA_GA_isCrash(&this->light_thaga)) {
        err = TRUE;
    }

    if (THA_GA_isCrash(&this->bg_opaque_thaga)) {
        err = TRUE;
    }

    if (THA_GA_isCrash(&this->bg_translucent_thaga)) {
        err = TRUE;
    }

    return err;
}

static void do_soft_reset(GAME* game) {
    SoftResetEnable = FALSE;
    mBGM_reset();
    mVibctl_reset();
    sAdo_SoftReset();
    ResetTime = osGetTime();
    ResetStatus = IRQ_RESET_PRENMI;
}

static void reset_check(GRAPH* this, GAME* game) {
    if (SoftResetEnable && osShutdown) {
        do_soft_reset(game);
    }
}

// Aus version removes debug frame skip logic
#if VERSION >= VER_GAFU01_00
static void graph_main(GRAPH* this, GAME* game) {
    game->disable_prenmi = FALSE;
    graph_setup_double_buffer(this);
    game_get_controller(game);
    game->disable_display = FALSE;
    GRAPH_SET_DOING_POINT(this, GAME_MAIN);
    game_main(game);
    GRAPH_SET_DOING_POINT(this, GAME_MAIN_FINISHED);
    if (ResetStatus < IRQ_RESET_DELAY) {
#ifdef TARGET_PC
        if (g_pc_frameskip_active) {
            /* Logic-only tick: skip all rendering but do event pump + frame counter */
            extern void VIWaitForRetrace(void);
            VIWaitForRetrace();
            this->frame_counter++;
        } else
#endif
        if (game->disable_display == FALSE) {
            int draw_err = graph_draw_finish(this);
            PC_DIAG(5, "graph_main: draw_finish=%d ResetStatus=%d\n", draw_err, ResetStatus);
            if (draw_err == FALSE) {
                GRAPH_SET_DOING_POINT(this, TASK_SET);
                graph_task_set00(this);
                GRAPH_SET_DOING_POINT(this, TASK_SET_FINISHED);
                this->frame_counter++;

                if ((GETREG(SREG, 33) & 1) != 0) {
                    SETREG(SREG, 33, GETREG(SREG, 33) & ~1);
                }
            }
        }
    }

    if (GETREG(SREG, 20) < 2) {
        GRAPH_SET_DOING_POINT(this, AUDIO);
        sAdo_GameFrame();
        GRAPH_SET_DOING_POINT(this, AUDIO_FINISHED);
    }

    reset_check(this, game);

    if (ResetStatus == IRQ_RESET_PRENMI && game->disable_prenmi == FALSE) {
        GAME_GOTO_NEXT(game, prenmi, PRENMI);
    }
}
#else
static void graph_main(GRAPH* this, GAME* game) {
    game->disable_prenmi = FALSE;
    PC_DIAG(10, "graph_main: enter, frame_counter=%d game=%p exec=%p cleanup=%p doing=%d\n",
            this->frame_counter, (void*)game, (void*)game->exec, (void*)game->cleanup, game->doing);
    graph_setup_double_buffer(this);
    game_get_controller(game);
    game->disable_display = FALSE;
    GRAPH_SET_DOING_POINT(this, GAME_MAIN);
    PC_DIAG(10, "graph_main: calling game_main (exec=%p)\n", (void*)game->exec);
#ifdef TARGET_PC
    {
        static jmp_buf game_main_jmpbuf;
        pc_crash_set_jmpbuf(&game_main_jmpbuf);
        if (setjmp(game_main_jmpbuf) != 0) {
            fprintf(stderr, "[PC] CRASH in game_main! crash_addr=0x%llX data_addr=0x%llX doing_point=%d doing_point_specific=%d\n",
                   (unsigned long long)pc_crash_get_addr(),
                   (unsigned long long)pc_crash_get_data_addr(),
                   game->doing_point, game->doing_point_specific);
            g_pc_running = 0; /* halt after logging */
        } else {
            game_main(game);
        }
        pc_crash_set_jmpbuf(NULL);
    }
#else
    game_main(game);
#endif
    PC_DIAG(10, "graph_main: game_main returned, frame_counter=%d\n", this->frame_counter);
    GRAPH_SET_DOING_POINT(this, GAME_MAIN_FINISHED);
    if (ResetStatus < IRQ_RESET_DELAY) {
#ifdef TARGET_PC
        if (g_pc_frameskip_active) {
            extern void VIWaitForRetrace(void);
            VIWaitForRetrace();
            this->frame_counter++;
        } else
#endif
        if (skip_frame < GETREG(SREG, 3)) {
            skip_frame++;
            this->frame_counter++;
        } else if (game->disable_display == FALSE) {
            skip_frame = 0;
            if (graph_draw_finish(this) == FALSE) {
                GRAPH_SET_DOING_POINT(this, TASK_SET);
                graph_task_set00(this);
                GRAPH_SET_DOING_POINT(this, TASK_SET_FINISHED);
                this->frame_counter++;
                PC_DIAG(10, "graph2: task_set done, frame_counter=%d\n", this->frame_counter);

                if ((GETREG(SREG, 33) & 1) != 0) {
                    SETREG(SREG, 33, GETREG(SREG, 33) & ~1);
                }
            }
        }
    }

    PC_DIAG(10, "graph2: before audio+reset, frame_counter=%d\n", this->frame_counter);
    if (GETREG(SREG, 20) < 2) {
        GRAPH_SET_DOING_POINT(this, AUDIO);
#ifdef TARGET_PC
        {
            static jmp_buf audio_jmpbuf;
            pc_crash_set_jmpbuf(&audio_jmpbuf);
            if (setjmp(audio_jmpbuf) != 0) {
                fprintf(stderr, "[PC] CRASH in sAdo_GameFrame! addr=0x%08X data=0x%08X\n",
                       pc_crash_get_addr(), pc_crash_get_data_addr());
            } else {
                sAdo_GameFrame();
            }
            pc_crash_set_jmpbuf(NULL);
        }
#else
        sAdo_GameFrame();
#endif
        GRAPH_SET_DOING_POINT(this, AUDIO_FINISHED);
    }

    reset_check(this, game);
    PC_DIAG(10, "graph2: reset_check done\n");

    if (ResetStatus == IRQ_RESET_PRENMI && game->disable_prenmi == FALSE) {
        GAME_GOTO_NEXT(game, prenmi, PRENMI);
    }
}
#endif

#ifdef TARGET_PC
/* Update common_data.door_data with the player's actual current world position
 * and facing direction, so that restoring from a snapshot places the player at
 * their mid-game location rather than the last door-exit position.
 *
 * Called just before the snapshot is written to disk via g_pc_snapshot_pre_save_hook.
 * Uses only ACTOR* (no PLAYER_ACTOR needed) to avoid extra includes. */
static void pc_snapshot_update_player_doordata(void) {
    GAME_PLAY* play = (GAME_PLAY*)gamePT;
    if (play == NULL) return;
    if (!Common_Get(player_actor_exists)) return;

    ACTOR* player = play->actor_info.list[ACTOR_PART_PLAYER].actor;
    if (player == NULL) return;

    /* Capture world position into door_data so Scene_Proc_Player_Ptr restores
     * the player at their actual location instead of the house-door exit. */
    s16 px = (s16)player->world.position.x;
    s16 py = (s16)player->world.position.y;
    s16 pz = (s16)player->world.position.z;

    /* Quantize rotation.y to the nearest of 8 compass directions (0x2000 steps).
     * angle_table[i] = i * 45 degrees. */
    u8 ori = (u8)(((u16)(player->shape_info.rotation.y + 0x1000)) >> 13) & 7;

    common_data.door_data.exit_position.x = px;
    common_data.door_data.exit_position.y = py;
    common_data.door_data.exit_position.z = pz;
    common_data.door_data.exit_orientation = ori;
    /* next_scene_id must be non-zero for Scene_Proc_Player_Ptr to use it;
     * convention is current_scene + 1. */
    common_data.door_data.next_scene_id = (int)Save_Get(scene_no) + 1;
    /* extra_data = 0 → default INTRO spawn animation (standing idle). */
    common_data.door_data.extra_data = 0;

    printf("[SNAPSHOT] Player doordata updated: pos=(%d,%d,%d) ori=%d scene=%d\n",
           (int)px, (int)py, (int)pz, (int)ori, (int)Save_Get(scene_no));
}
#endif

extern void graph_proc(void* arg) {
    GRAPH* __graph = &graph_class;
    DLFTBL_GAME* dlftbl = &game_dlftbls[0];
    printf("[GRAPH] graph_proc entry\n");
#ifdef TARGET_PC
    printf("[GRAPH] Checking PC-specific boot logic\n");
    printf("[GRAPH] g_pc_model_viewer=%d\n", g_pc_model_viewer);
    printf("[GRAPH] pc_snapshot_was_restored()=%d\n", pc_snapshot_was_restored());

    if (g_pc_model_viewer) {
        printf("[GRAPH] Using model viewer (game_dlftbls[10])\n");
        dlftbl = &game_dlftbls[10]; /* model viewer */
    } else if (pc_snapshot_was_restored()) {
        /* Skip title/select: jump directly to the play state.
         * The arena was already loaded with snapshotted game state by OSInit().
         * play_init() will read world/player data from the restored arena.
         * Initialize BSS-resident globals that first_game/second_game/select would set. */
        printf("[RESTORE] ============================================\n");
        printf("[RESTORE] Snapshot was restored! Initializing BSS globals...\n");

        printf("[RESTORE] Calling init_rnd() (random seed init)...\n");
        init_rnd();
        printf("[RESTORE] init_rnd() complete\n");

        printf("[RESTORE] Calling __osInitialize_common() (OS common init)...\n");
        __osInitialize_common();
        printf("[RESTORE] __osInitialize_common() complete\n");

        /* common_data was already restored from the snapshot in pc_snapshot_try_restore(). */
        printf("[RESTORE] Calling mBGM_ct()...\n");
        mBGM_ct();
        printf("[RESTORE] mBGM_ct() complete\n");

        printf("[RESTORE] Calling mVibctl_ct()...\n");
        mVibctl_ct();
        printf("[RESTORE] mVibctl_ct() complete\n");

        /* first_game_init normally calls this to allocate ARAM blocks for
         * mail/diary/custom-design data. Without it, l_aram_block_p_table[]
         * stays NULL which breaks any code path that reads/writes ARAM data. */
        printf("[RESTORE] Calling mCD_save_data_aram_malloc()...\n");
        mCD_save_data_aram_malloc();
        printf("[RESTORE] mCD_save_data_aram_malloc() complete\n");

        /* second_game_init normally sets this after loading the save from disk.
         * The save is already in common_data (restored from snapshot), so mark
         * it loaded so common_data_reinit() and pc_save_reload() behave correctly
         * if ever called. */
        {
            extern int pc_save_loaded;
            pc_save_loaded = 1;
            printf("[RESTORE] pc_save_loaded set to 1\n");
        }

        printf("[RESTORE] Skipping to play state (game_dlftbls[2])\n");
        printf("[RESTORE] ============================================\n");
        dlftbl = &game_dlftbls[2];
    } else {
        printf("[GRAPH] Normal boot — starting from title screen (game_dlftbls[0])\n");
    }
#endif
#ifdef TARGET_PC
    /* Register the player-position pre-save hook unconditionally so snapshots
     * always capture the actual mid-game player location. */
    g_pc_snapshot_pre_save_hook = pc_snapshot_update_player_doordata;
#endif
    printf("[GRAPH] Calling graph_ct(&graph_class)...\n");
    graph_ct(&graph_class);
    printf("[GRAPH] graph_ct() complete\n");
#ifdef TARGET_PC
    double tick_accumulator = 0.0;
    extern int g_pc_fps_target;
#endif

    while (dlftbl != NULL) {
        printf("[GRAPH] ============================================\n");
        printf("[GRAPH] Starting new game state from dlftbl=%p\n", (void*)dlftbl);
        printf("[GRAPH] dlftbl->alloc_size=%zu\n", dlftbl->alloc_size);
        printf("[GRAPH] dlftbl->init=%p\n", (void*)dlftbl->init);

        size_t size = dlftbl->alloc_size;
        GAME* game = (GAME*)malloc(size);
        printf("[GRAPH] Allocated GAME structure at %p (%zu bytes)\n", (void*)game, size);

        game_class_p = game;
        bzero(game, size);
        printf("[GRAPH] Zeroed GAME structure\n");

        GRAPH_SET_DOING_POINT(__graph, GAME_CT);
        printf("[GRAPH] Calling game_ct(game, %p, __graph)...\n", (void*)dlftbl->init);
        printf("[GRAPH] (This may call play_init for restored snapshots)\n");
        game_ct(game, dlftbl->init, __graph);
        printf("[GRAPH] game_ct() returned successfully\n");

        emu64_refresh();
        GRAPH_SET_DOING_POINT(__graph, GAME_CT_FINISHED);
        printf("[GRAPH] ============================================\n");

        while (game_is_doing(game) && g_pc_running) {
#ifdef TARGET_PC
            /* Tick batching: run N logic ticks per visual frame for FPS targets < 60.
             * g_pc_frameskip_active=1 ticks skip all GL work and frame pacing. */
            {
                double ticks_per_visual;
                int ticks, t;
                if (g_pc_settings.fps_target == 7) {
                    /* Dynamic: arbitrary fps, use exact ratio (accumulator handles fractions) */
                    ticks_per_visual = (g_pc_fps_target > 0) ? 60.0 / (double)g_pc_fps_target : 1.0;
                } else {
                    switch (g_pc_fps_target) {
                        case 20: ticks_per_visual = 3.0; break;
                        case 30: ticks_per_visual = 2.0; break;
                        case 40: ticks_per_visual = 1.5; break;
                        case 50: ticks_per_visual = 1.2; break;
                        default: ticks_per_visual = 1.0; break;
                    }
                }
                tick_accumulator += ticks_per_visual;
                ticks = (int)tick_accumulator;
                tick_accumulator -= (double)ticks;
                if (ticks < 1) ticks = 1;
                for (t = 0; t < ticks; t++) {
                    g_pc_frameskip_active = (t < ticks - 1) ? 1 : 0;
                    PC_DIAG(10, "graph_proc: tick %d/%d frameskip=%d\n", t+1, ticks, g_pc_frameskip_active);
                    if (!dvderr_draw()) {
                        graph_main(__graph, game);
                    }
                    if (!game_is_doing(game) || !g_pc_running) break;
                }
                g_pc_frameskip_active = 0;
            }
#else
            PC_DIAG(10, "graph_proc: loop top, game=%p\n", (void*)game);
            if (!dvderr_draw()) {
                graph_main(__graph, game);
            }
#endif
        }

        dlftbl = game_get_next_game_dlftbl(game);
        GRAPH_SET_DOING_POINT(__graph, GAME_18);
        GRAPH_SET_DOING_POINT(__graph, GAME_DT);
        printf("[GRAPH] Calling game_dt (play_cleanup)...\n");
        game_dt(game);
        printf("[GRAPH] game_dt complete\n");
        GRAPH_SET_DOING_POINT(__graph, GAME_DT_FINISHED);
        free(game);
        game_class_p = NULL;
#ifdef TARGET_PC
        if (!g_pc_running) break;
#endif
    }

    graph_dt(__graph);
}
