/* pc_snapshot.h - suspend/snapshot save and restore */
#ifndef PC_SNAPSHOT_H
#define PC_SNAPSHOT_H

#ifdef __cplusplus
extern "C" {
#endif

/* Snapshot file written relative to cwd */
#define PC_SNAPSHOT_PATH "ac_snapshot.bin"

/* Request suspend at the next render frame boundary */
void pc_snapshot_request_suspend(void);

/* Request restart: delete snapshot and exit the game loop (triggers relaunch by shell/launcher) */
void pc_snapshot_request_restart(void);

/* Called from VIWaitForRetrace() at each render frame boundary.
 * If a suspend was requested, saves the arena to disk here. */
void pc_snapshot_check_frame_boundary(void);

/* Called from OSInit() BEFORE the arena is allocated.
 * Returns the saved arena base address if a valid snapshot header exists, 0 otherwise.
 * Used to attempt allocation at the same address so arena pointers remain valid. */
uintptr_t pc_snapshot_peek_arena_addr(void);

/* Called from OSInit() after the arena is zeroed and pointers are set.
 * Returns 1 if a snapshot was found and loaded into the arena, 0 otherwise. */
int pc_snapshot_try_restore(void);

/* Returns 1 after a successful pc_snapshot_try_restore() call. */
int pc_snapshot_was_restored(void);

/* Optional pre-save hook: called just before the snapshot is written to disk.
 * Set this (e.g. from graph.c) to update common_data fields from live actor
 * state — in particular, common_data.door_data.exit_position should be set
 * to the player's actual world position so restore spawns them correctly.
 * NULL (default) means no hook. */
extern void (*g_pc_snapshot_pre_save_hook)(void);

#ifdef __cplusplus
}
#endif

#endif /* PC_SNAPSHOT_H */
