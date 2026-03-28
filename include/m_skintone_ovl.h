#ifndef M_SKINTONE_OVL_H
#define M_SKINTONE_OVL_H

#include "types.h"
#include "m_skintone_ovl_h.h"
#include "m_submenu_ovl.h"

#ifdef __cplusplus
extern "C" {
#endif

#if VERSION >= VER_DELUXE

#define mST_TONE_NUM 8
#define mST_TONE_COLS 2
#define mST_TONE_ROWS 4

struct skintone_ovl_s {
    int cursor_idx;
    int initial_rank;
};

extern void mST_skintone_ovl_set_proc(Submenu* submenu);
extern void mST_skintone_ovl_construct(Submenu* submenu);
extern void mST_skintone_ovl_destruct(Submenu* submenu);

#endif /* VERSION >= VER_DELUXE */

#ifdef __cplusplus
}
#endif

#endif
