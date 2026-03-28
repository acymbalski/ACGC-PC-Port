#ifndef M_STORAGE_OVL_H
#define M_STORAGE_OVL_H

#include "types.h"
#include "m_storage_ovl_h.h"
#include "m_submenu_ovl.h"

#ifdef __cplusplus
extern "C" {
#endif

#if VERSION >= VER_DELUXE

#define mSO_PAGE_NUM 3
#define mSO_ITEMS_PER_PAGE 15
#define mSO_TOTAL_ITEMS 45

enum {
    mSO_MODE_PUTIN,
    mSO_MODE_TAKEOUT
};

struct storage_ovl_s {
    int cursor_idx;
    int current_page;
    int target_page;
    f32 scroll_y;
    f32 target_scroll_y;
    int mode;
    int initial_setup;
};

extern void mSO_storage_ovl_set_proc(Submenu* submenu);
extern void mSO_storage_ovl_construct(Submenu* submenu);
extern void mSO_storage_ovl_destruct(Submenu* submenu);

#endif /* VERSION >= VER_DELUXE */

#ifdef __cplusplus
}
#endif

#endif
