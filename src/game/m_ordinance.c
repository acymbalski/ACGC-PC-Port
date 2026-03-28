#include "types.h"
#if VERSION >= VER_DELUXE
#include "m_ordinance.h"

#include "m_npc_schedule.h"

static u8 l_ordinance_flags;

/* Ordinance flags are stored in Save_t.ordinance_flags (offset 0x241A8).
 * Bit layout:
 *   [7:6] time_shift  (0=none, 1=early bird, 2=night owl)
 *   [5]   bell_boom
 *   [4]   beautiful
 *   [3:0] unused
 */

int mOR_GetTimeShift(void) {
    u8 flags = l_ordinance_flags;
    int shift = (flags >> 6) & 3;
    if (shift > mOR_TIME_SHIFT_MAX) {
        shift = mOR_TIME_SHIFT_MAX;
    }
    return shift;
}

int mOR_GetBellBoom(void) {
    u8 flags = l_ordinance_flags;
    return (flags >> 5) & 1;
}

int mOR_GetBeautiful(void) {
    u8 flags = l_ordinance_flags;
    return (flags >> 4) & 1;
}

void mOR_SetTimeShift(int value) {
    u8 flags = l_ordinance_flags;

    if (value < 0) {
        value = 0;
    } else if (value > mOR_TIME_SHIFT_MAX) {
        value = mOR_TIME_SHIFT_MAX;
    }

    /* Clear bits [7:6], insert new value */
    flags = (flags & 0x3F) | ((value & 3) << 6);
    l_ordinance_flags = flags;

    /* Recalculate all NPC schedules for the new time shift */
    mNPS_set_all_schedule_area();
}

void mOR_SetBellBoom(int value) {
    u8 flags = l_ordinance_flags;

    if (value) {
        flags |= (1 << 5);
    } else {
        flags &= ~(1 << 5);
    }

    l_ordinance_flags = flags;
}

void mOR_SetBeautiful(int value) {
    u8 flags = l_ordinance_flags;

    if (value) {
        flags |= (1 << 4);
    } else {
        flags &= ~(1 << 4);
    }

    l_ordinance_flags = flags;
}

int mOR_IsActive(void) {
    u8 flags = l_ordinance_flags;
    /* Any bit in [7:4] set means an ordinance is active */
    return ((flags >> 4) & 0xF) != 0;
}
#endif /* VERSION >= VER_DELUXE */
