/**
 * @file m_debug_cuyler.c
 * @brief Debug overlay module (Deluxe).
 *
 * Stub debug visualization system. Both functions are 4-byte stubs
 * (immediate return) in the Deluxe REL, indicating the debug
 * implementation was stripped for release.
 */

#include "types.h"

#if VERSION >= VER_DELUXE

#include "m_debug_cuyler.h"

/**
 * @brief Debug overlay update (stub).
 *
 * 4 bytes compiled (blr).
 */
void debug_cuyler_move(void) {
}

/**
 * @brief Debug overlay draw (stub).
 *
 * 4 bytes compiled (blr).
 */
void debug_cuyler_draw(void) {
}

#endif /* VERSION >= VER_DELUXE */
