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
#include "m_common_data.h"
#include "m_house.h"
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
#define SNAP_VERSION 2u           /* v2: appends common_data after arena payload */

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t ram_size;         /* byte count of arena payload */
    uint32_t crc32;            /* CRC32 of arena payload */
    uint64_t frame;            /* pc_frame_counter at snapshot */
    uint64_t arena_base_addr;  /* original arena base address */
    uint32_t common_data_size; /* byte count of common_data payload (follows arena) */
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

/* ---- BSS pointer fixup ---------------------------------------------------- */

/* After restoring common_data, the now_private / now_home fields contain raw
 * pointer values from the save run.  These point into BSS (common_data itself).
 * If ASLR shifted the BSS base between runs the saved values are stale.
 * Recompute both fields from the restored player_no so they always reflect the
 * current process layout. */
static void pc_snapshot_fixup_common_data_ptrs(void) {
    u8 pno = common_data.player_no;
    printf("[RESTORE] Fixing up common_data derived pointers (player_no=%d)\n", (int)pno);
    if (pno < PLAYER_NUM) {
        common_data.now_private = &common_data.save.save.private_data[pno];
        common_data.now_home    = &common_data.save.save.homes[mHS_get_arrange_idx(pno)];
    } else {
        printf("[RESTORE]   player_no %d out of range, using slot 0\n", (int)pno);
        common_data.now_private = &common_data.save.save.private_data[0];
        common_data.now_home    = &common_data.save.save.homes[0];
    }
    printf("[RESTORE]   now_private=%p  now_home=%p\n",
           (void*)common_data.now_private, (void*)common_data.now_home);
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
        fprintf(stderr, "[SNAPSHOT] pc_arena_base=%p, pc_arena_end=%p\n",
                (void*)pc_arena_base, (void*)pc_arena_end);
        return;
    }

    size_t ram_size = (size_t)(pc_arena_end - pc_arena_base);
    printf("[SNAPSHOT] Frame boundary reached — saving %zu bytes...\n", ram_size);
    printf("[SNAPSHOT] Arena range: %p - %p\n", (void*)pc_arena_base, (void*)pc_arena_end);
    printf("[SNAPSHOT] Computing CRC32...\n");

    uint32_t crc = crc32_buf(pc_arena_base, ram_size);
    printf("[SNAPSHOT] CRC32 computed: 0x%08X\n", crc);

    FILE* f = fopen(PC_SNAPSHOT_PATH, "wb");
    if (!f) {
        fprintf(stderr, "[SNAPSHOT] Cannot open '%s' for writing: %s\n",
                PC_SNAPSHOT_PATH, strerror(errno));
        return;
    }
    printf("[SNAPSHOT] File opened for writing: '%s'\n", PC_SNAPSHOT_PATH);

    SnapHeader hdr;
    hdr.magic             = SNAP_MAGIC;
    hdr.version           = SNAP_VERSION;
    hdr.ram_size          = (uint32_t)ram_size;
    hdr.crc32             = crc;
    hdr.frame             = (uint64_t)pc_frame_counter;
    hdr.arena_base_addr   = (uint64_t)(uintptr_t)pc_arena_base;
    hdr.common_data_size  = (uint32_t)sizeof(common_data);

    printf("[SNAPSHOT] Header: magic=0x%08X version=%u ram_size=%u crc=0x%08X frame=%llu arena_base=0x%llX common_data_size=%u\n",
           hdr.magic, hdr.version, hdr.ram_size, hdr.crc32, (unsigned long long)hdr.frame,
           (unsigned long long)hdr.arena_base_addr, hdr.common_data_size);

    if (fwrite(&hdr, sizeof(hdr), 1, f) != 1) {
        fprintf(stderr, "[SNAPSHOT] Header write failed\n");
        fclose(f);
        remove(PC_SNAPSHOT_PATH);
        return;
    }
    printf("[SNAPSHOT] Header written (%zu bytes)\n", sizeof(hdr));

    size_t written = fwrite(pc_arena_base, 1, ram_size, f);
    printf("[SNAPSHOT] Arena data written: %zu/%zu bytes\n", written, ram_size);

    if (written != ram_size) {
        fprintf(stderr, "[SNAPSHOT] Incomplete write (%zu/%zu) — deleting\n",
                written, ram_size);
        fclose(f);
        remove(PC_SNAPSHOT_PATH);
        return;
    }

    size_t cd_written = fwrite(&common_data, 1, sizeof(common_data), f);
    printf("[SNAPSHOT] common_data written: %zu/%zu bytes (scene_no=%d)\n",
           cd_written, sizeof(common_data), common_data.save.save.scene_no);
    fclose(f);

    if (cd_written != sizeof(common_data)) {
        fprintf(stderr, "[SNAPSHOT] common_data write incomplete (%zu/%zu) — deleting\n",
                cd_written, sizeof(common_data));
        remove(PC_SNAPSHOT_PATH);
        return;
    }

    printf("[SNAPSHOT] Saved — arena=%zu + common_data=%zu bytes, frame %llu, CRC=0x%08X → '%s'\n",
           ram_size, sizeof(common_data), (unsigned long long)hdr.frame, crc, PC_SNAPSHOT_PATH);
    printf("[SNAPSHOT] Snapshot creation complete!\n");
}

uintptr_t pc_snapshot_peek_arena_addr(void) {
    FILE* f = fopen(PC_SNAPSHOT_PATH, "rb");
    if (!f) return 0;
    SnapHeader hdr;
    int ok = (fread(&hdr, sizeof(hdr), 1, f) == 1)
           && (hdr.magic == SNAP_MAGIC)
           && (hdr.version == SNAP_VERSION);
    fclose(f);
    if (!ok) return 0;
    printf("[RESTORE] Snapshot found — saved arena base: 0x%llX\n",
           (unsigned long long)hdr.arena_base_addr);
    return (uintptr_t)hdr.arena_base_addr;
}

int pc_snapshot_try_restore(void) {
    printf("[RESTORE] ============================================\n");
    printf("[RESTORE] Checking for snapshot at '%s'\n", PC_SNAPSHOT_PATH);

    if (!pc_arena_base || !pc_arena_end || pc_arena_end <= pc_arena_base) {
        fprintf(stderr, "[RESTORE] Arena not ready\n");
        fprintf(stderr, "[RESTORE] pc_arena_base=%p, pc_arena_end=%p\n",
                (void*)pc_arena_base, (void*)pc_arena_end);
        return 0;
    }
    printf("[RESTORE] Arena ready: %p - %p (%zu bytes)\n",
           (void*)pc_arena_base, (void*)pc_arena_end,
           (size_t)(pc_arena_end - pc_arena_base));

    FILE* f = fopen(PC_SNAPSHOT_PATH, "rb");
    if (!f) {
        printf("[RESTORE] No snapshot found (errno=%d) — normal boot\n", errno);
        return 0;
    }
    printf("[RESTORE] Snapshot file opened\n");

    SnapHeader hdr;
    size_t hdr_read = fread(&hdr, sizeof(hdr), 1, f);
    printf("[RESTORE] Header read: %zu items\n", hdr_read);
    if (hdr_read != 1) {
        fprintf(stderr, "[RESTORE] Header read failed (got %zu items) — deleting\n", hdr_read);
        fclose(f);
        remove(PC_SNAPSHOT_PATH);
        return 0;
    }
    printf("[RESTORE] Header: magic=0x%08X, version=%u, ram_size=%u, crc=0x%08X, frame=%llu, arena_base=0x%llX\n",
           hdr.magic, hdr.version, hdr.ram_size, hdr.crc32, (unsigned long long)hdr.frame,
           (unsigned long long)hdr.arena_base_addr);

    if (hdr.magic != SNAP_MAGIC) {
        fprintf(stderr, "[RESTORE] Bad magic 0x%08X (expected 0x%08X) — deleting\n",
                hdr.magic, SNAP_MAGIC);
        fclose(f);
        remove(PC_SNAPSHOT_PATH);
        return 0;
    }
    printf("[RESTORE] Magic check OK\n");

    if (hdr.version != SNAP_VERSION) {
        fprintf(stderr, "[RESTORE] Version mismatch (got %u, want %u) — deleting\n",
                hdr.version, SNAP_VERSION);
        fclose(f);
        remove(PC_SNAPSHOT_PATH);
        return 0;
    }
    printf("[RESTORE] Version check OK\n");

    size_t arena_size = (size_t)(pc_arena_end - pc_arena_base);
    if (hdr.ram_size != (uint32_t)arena_size) {
        fprintf(stderr, "[RESTORE] Size mismatch (snap=%u, arena=%zu) — deleting\n",
                hdr.ram_size, arena_size);
        fclose(f);
        remove(PC_SNAPSHOT_PATH);
        return 0;
    }
    printf("[RESTORE] Size check OK (%zu bytes)\n", arena_size);

    printf("[RESTORE] Loading %u bytes from file (frame %llu, expected CRC=0x%08X)...\n",
           hdr.ram_size, (unsigned long long)hdr.frame, hdr.crc32);

    size_t nread = fread(pc_arena_base, 1, arena_size, f);
    printf("[RESTORE] Read %zu bytes from file\n", nread);

    if (nread != arena_size) {
        fprintf(stderr, "[RESTORE] Incomplete read (%zu/%zu) — deleting\n",
                nread, arena_size);
        fclose(f);
        memset(pc_arena_base, 0, arena_size);
        remove(PC_SNAPSHOT_PATH);
        return 0;
    }
    printf("[RESTORE] Read check OK\n");

    printf("[RESTORE] Computing CRC32 of loaded data...\n");
    uint32_t crc = crc32_buf(pc_arena_base, arena_size);
    printf("[RESTORE] Computed CRC32: 0x%08X (expected 0x%08X)\n", crc, hdr.crc32);

    if (crc != hdr.crc32) {
        fprintf(stderr, "[RESTORE] CRC mismatch (computed=0x%08X expected=0x%08X) — deleting\n",
                crc, hdr.crc32);
        memset(pc_arena_base, 0, arena_size);
        remove(PC_SNAPSHOT_PATH);
        return 0;
    }
    printf("[RESTORE] CRC check OK\n");

    /* If the arena loaded at a different address than it was saved from, every
     * absolute pointer stored in the arena data is wrong.  Fix them by scanning
     * every 8-byte-aligned word: values that fall within the old arena range are
     * arena pointers and get shifted by the delta.  Values outside that range
     * (code pointers, small integers, floats, etc.) are left alone. */
    uintptr_t saved_base   = (uintptr_t)hdr.arena_base_addr;
    uintptr_t current_base = (uintptr_t)pc_arena_base;
    printf("[RESTORE] ============================================\n");
    printf("[RESTORE] ARENA BASE ADDRESS CHECK:\n");
    printf("[RESTORE] Snapshot saved at:   0x%llX\n", (unsigned long long)saved_base);
    printf("[RESTORE] Currently loaded at: 0x%llX\n", (unsigned long long)current_base);
    if (saved_base != current_base) {
        int64_t delta = (int64_t)(current_base - saved_base);
        size_t relocated = 0;
        uint64_t* words = (uint64_t*)pc_arena_base;
        size_t word_count = arena_size / sizeof(uint64_t);
        size_t i;
        printf("[RESTORE] Address mismatch — relocating arena pointers (delta=0x%llX)...\n",
               (unsigned long long)(uint64_t)delta);
        for (i = 0; i < word_count; i++) {
            uintptr_t v = (uintptr_t)words[i];
            if (v >= saved_base && v < saved_base + arena_size) {
                words[i] = (uint64_t)(v + (uintptr_t)(uint64_t)delta);
                relocated++;
            }
        }
        printf("[RESTORE] Relocation complete — adjusted %zu pointers\n", relocated);
    } else {
        printf("[RESTORE] Address match — no relocation needed\n");
    }
    printf("[RESTORE] ============================================\n");

    /* Restore common_data if present in this snapshot version */
    if (hdr.common_data_size > 0) {
        if (hdr.common_data_size != (uint32_t)sizeof(common_data)) {
            fprintf(stderr, "[RESTORE] common_data size mismatch (snap=%u, current=%zu) — skipping\n",
                    hdr.common_data_size, sizeof(common_data));
        } else {
            size_t cd_read = fread(&common_data, 1, sizeof(common_data), f);
            if (cd_read != sizeof(common_data)) {
                fprintf(stderr, "[RESTORE] common_data read incomplete (%zu/%u)\n",
                        cd_read, hdr.common_data_size);
            } else {
                printf("[RESTORE] common_data restored (scene_no=%d)\n",
                       common_data.save.save.scene_no);

                pc_snapshot_fixup_common_data_ptrs();

                /* Relocate any arena pointers embedded in common_data.
                 * common_data lives in BSS (not in the arena), so it was NOT
                 * covered by the arena relocation scan above.  Some fields
                 * point INTO the arena and must be shifted by the same delta.
                 * (now_private / now_home were already fixed up above because
                 * they point into BSS, not the arena.) */
                if (saved_base != current_base) {
                    int64_t delta = (int64_t)(current_base - saved_base);
                    uint64_t* words = (uint64_t*)&common_data;
                    size_t word_count = sizeof(common_data) / sizeof(uint64_t);
                    size_t i, cd_relocated = 0;
                    for (i = 0; i < word_count; i++) {
                        uintptr_t v = (uintptr_t)words[i];
                        if (v >= saved_base && v < saved_base + arena_size) {
                            words[i] = (uint64_t)(v + (uintptr_t)(uint64_t)delta);
                            cd_relocated++;
                        }
                    }
                    printf("[RESTORE] common_data pointer relocation: adjusted %zu pointers\n",
                           cd_relocated);
                }
            }
        }
    }
    fclose(f);

    printf("[RESTORE] Restore complete — resuming at frame %llu\n",
           (unsigned long long)hdr.frame);
    printf("[RESTORE] Setting s_was_restored=1\n");
    s_was_restored = 1;
    printf("[RESTORE] ============================================\n");
    return 1;
}

int pc_snapshot_was_restored(void) {
    return s_was_restored;
}
