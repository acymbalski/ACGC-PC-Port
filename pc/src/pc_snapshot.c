/* pc_snapshot.c - arena suspend/snapshot save and restore
 *
 * Save:    triggered by pc_snapshot_request_suspend() → detected at frame
 *          boundary in VIWaitForRetrace() → writes full arena to disk.
 *
 * Restore: called from OSInit() immediately after the arena is zeroed.
 *          Reads the file back into the arena so the game boots with
 *          pre-populated state.  graph_proc() then skips the title/select
 *          sequence and starts directly in the play state.
 *
 * NOTE (Phase 1 limitation): restoring skips first_game/second_game/select
 * boot stages.  BSS-resident globals that those stages normally initialise
 * (common_data, BGM state, etc.) will be zero on a restored boot.  This is
 * sufficient for resuming gameplay in most cases; edge cases can be addressed
 * in a later phase once real-world testing identifies them.
 */
#include "pc_platform.h"
#include "pc_snapshot.h"
#include <errno.h>

/* ---- Arena pointers (pc_os.c) ------------------------------------------- */
extern u8* pc_arena_base;
extern u8* pc_arena_end;

/* ---- Frame counter (pc_vi.c) --------------------------------------------- */
extern u32 pc_frame_counter;

/* ---- Running flag (pc_main.c) -------------------------------------------- */
extern int g_pc_running;

/* ---- Snapshot file header ------------------------------------------------- */
#define SNAP_MAGIC   0x41435353u  /* "ACSS" */
#define SNAP_VERSION 1u

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t ram_size;  /* byte count of arena payload */
    uint32_t crc32;     /* CRC32 of arena payload */
    uint64_t frame;     /* pc_frame_counter at snapshot */
} SnapHeader;

/* ---- State flags ---------------------------------------------------------- */
static int s_suspend_requested = 0;
static int s_restart_requested = 0;
static int s_was_restored      = 0;

/* ---- CRC32 (IEEE 802.3 polynomial) --------------------------------------- */
static uint32_t crc32_buf(const void* data, size_t len) {
    static uint32_t table[256];
    static int inited = 0;
    if (!inited) {
        uint32_t i;
        for (i = 0; i < 256; i++) {
            uint32_t c = i;
            int j;
            for (j = 0; j < 8; j++)
                c = (c & 1u) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
            table[i] = c;
        }
        inited = 1;
    }
    uint32_t crc = 0xFFFFFFFFu;
    const uint8_t* p = (const uint8_t*)data;
    size_t i;
    for (i = 0; i < len; i++)
        crc = table[(crc ^ p[i]) & 0xFFu] ^ (crc >> 8);
    return crc ^ 0xFFFFFFFFu;
}

/* ---- Public API ----------------------------------------------------------- */

void pc_snapshot_request_suspend(void) {
    if (s_suspend_requested) return;
    printf("[SUSPEND] Suspend requested — waiting for frame boundary\n");
    s_suspend_requested = 1;
}

void pc_snapshot_request_restart(void) {
    printf("[SUSPEND] Restart requested — deleting snapshot\n");
    remove(PC_SNAPSHOT_PATH);
    s_restart_requested = 1;
}

void pc_snapshot_check_frame_boundary(void) {
    if (s_restart_requested) {
        s_restart_requested = 0;
        printf("[SUSPEND] Exiting for restart\n");
        g_pc_running = 0;
        return;
    }

    if (!s_suspend_requested) return;
    s_suspend_requested = 0;

    if (!pc_arena_base || !pc_arena_end || pc_arena_end <= pc_arena_base) {
        fprintf(stderr, "[SNAPSHOT] Arena not ready, cannot save\n");
        return;
    }

    size_t ram_size = (size_t)(pc_arena_end - pc_arena_base);
    printf("[SNAPSHOT] Frame boundary reached — saving %zu bytes...\n", ram_size);

    uint32_t crc = crc32_buf(pc_arena_base, ram_size);

    FILE* f = fopen(PC_SNAPSHOT_PATH, "wb");
    if (!f) {
        fprintf(stderr, "[SNAPSHOT] Cannot open '%s' for writing: %s\n",
                PC_SNAPSHOT_PATH, strerror(errno));
        return;
    }

    SnapHeader hdr;
    hdr.magic    = SNAP_MAGIC;
    hdr.version  = SNAP_VERSION;
    hdr.ram_size = (uint32_t)ram_size;
    hdr.crc32    = crc;
    hdr.frame    = (uint64_t)pc_frame_counter;

    if (fwrite(&hdr, sizeof(hdr), 1, f) != 1) {
        fprintf(stderr, "[SNAPSHOT] Header write failed\n");
        fclose(f);
        remove(PC_SNAPSHOT_PATH);
        return;
    }

    size_t written = fwrite(pc_arena_base, 1, ram_size, f);
    fclose(f);

    if (written != ram_size) {
        fprintf(stderr, "[SNAPSHOT] Incomplete write (%zu/%zu) — deleting\n",
                written, ram_size);
        remove(PC_SNAPSHOT_PATH);
        return;
    }

    printf("[SNAPSHOT] Saved — %zu bytes, frame %llu, CRC=0x%08X → '%s'\n",
           ram_size, (unsigned long long)hdr.frame, crc, PC_SNAPSHOT_PATH);
}

int pc_snapshot_try_restore(void) {
    printf("[RESTORE] Checking for snapshot at '%s'\n", PC_SNAPSHOT_PATH);

    if (!pc_arena_base || !pc_arena_end || pc_arena_end <= pc_arena_base) {
        fprintf(stderr, "[RESTORE] Arena not ready\n");
        return 0;
    }

    FILE* f = fopen(PC_SNAPSHOT_PATH, "rb");
    if (!f) {
        printf("[RESTORE] No snapshot found — normal boot\n");
        return 0;
    }

    SnapHeader hdr;
    if (fread(&hdr, sizeof(hdr), 1, f) != 1) {
        fprintf(stderr, "[RESTORE] Header read failed — deleting\n");
        fclose(f);
        remove(PC_SNAPSHOT_PATH);
        return 0;
    }
    if (hdr.magic != SNAP_MAGIC) {
        fprintf(stderr, "[RESTORE] Bad magic 0x%08X — deleting\n", hdr.magic);
        fclose(f);
        remove(PC_SNAPSHOT_PATH);
        return 0;
    }
    if (hdr.version != SNAP_VERSION) {
        fprintf(stderr, "[RESTORE] Version mismatch (got %u, want %u) — deleting\n",
                hdr.version, SNAP_VERSION);
        fclose(f);
        remove(PC_SNAPSHOT_PATH);
        return 0;
    }

    size_t arena_size = (size_t)(pc_arena_end - pc_arena_base);
    if (hdr.ram_size != (uint32_t)arena_size) {
        fprintf(stderr, "[RESTORE] Size mismatch (snap=%u, arena=%zu) — deleting\n",
                hdr.ram_size, arena_size);
        fclose(f);
        remove(PC_SNAPSHOT_PATH);
        return 0;
    }

    printf("[RESTORE] Loading %u bytes (frame %llu, CRC=0x%08X)...\n",
           hdr.ram_size, (unsigned long long)hdr.frame, hdr.crc32);

    size_t nread = fread(pc_arena_base, 1, arena_size, f);
    fclose(f);

    if (nread != arena_size) {
        fprintf(stderr, "[RESTORE] Incomplete read (%zu/%zu) — deleting\n",
                nread, arena_size);
        memset(pc_arena_base, 0, arena_size);
        remove(PC_SNAPSHOT_PATH);
        return 0;
    }

    uint32_t crc = crc32_buf(pc_arena_base, arena_size);
    if (crc != hdr.crc32) {
        fprintf(stderr, "[RESTORE] CRC mismatch (computed=0x%08X expected=0x%08X) — deleting\n",
                crc, hdr.crc32);
        memset(pc_arena_base, 0, arena_size);
        remove(PC_SNAPSHOT_PATH);
        return 0;
    }

    printf("[RESTORE] Restore complete — resuming at frame %llu\n",
           (unsigned long long)hdr.frame);
    s_was_restored = 1;
    return 1;
}

int pc_snapshot_was_restored(void) {
    return s_was_restored;
}
