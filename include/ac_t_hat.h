#ifndef AC_T_HAT_H
#define AC_T_HAT_H

#include "types.h"
#include "m_actor.h"
#include "ac_tools.h"

#ifdef __cplusplus
extern "C" {
#endif

#if VERSION >= VER_DELUXE

extern ACTOR_PROFILE T_Hat_Profile;

typedef void (*HAT_PROC)(ACTOR*);

typedef struct t_hat_s {
    TOOLS_ACTOR tools_class;
    HAT_PROC proc;
    int current_id;
} HAT_ACTOR;

#endif /* VERSION >= VER_DELUXE */

#ifdef __cplusplus
}
#endif

#endif
