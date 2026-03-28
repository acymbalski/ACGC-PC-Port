#ifndef M_ORDINANCE_OVL_H
#define M_ORDINANCE_OVL_H

#include "types.h"
#include "m_ordinance_ovl_h.h"
#include "m_submenu_ovl.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
    mOD_CURSOR_TIME_SHIFT,
    mOD_CURSOR_BELL_BOOM,
    mOD_CURSOR_BEAUTIFUL,

    mOD_CURSOR_NUM
};

/* sizeof(mOD_Ovl_c) == 0x10 */
struct ordinance_ovl_s {
    /* 0x00 */ int cursor_idx;
    /* 0x04 */ int initial_time_shift;
    /* 0x08 */ int initial_bell_boom;
    /* 0x0C */ int initial_beautiful;
};

extern void mOD_ordinance_ovl_set_proc(Submenu* submenu);
extern void mOD_ordinance_ovl_construct(Submenu* submenu);
extern void mOD_ordinance_ovl_destruct(Submenu* submenu);

#ifdef __cplusplus
}
#endif

#endif
