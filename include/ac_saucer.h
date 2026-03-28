#ifndef AC_SAUCER_H
#define AC_SAUCER_H

#include "types.h"
#include "m_actor.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct saucer_actor_s SAUCER_ACTOR;

struct saucer_actor_s {
    /* 0x000 */ ACTOR actor_class;
    /* 0x174 */ f32 hover_y;
    /* 0x178 */ f32 spin_angle;
    /* 0x17C */ f32 hover_timer;
    /* 0x180 */ int state;
};

extern ACTOR_PROFILE Saucer_Profile;

#ifdef __cplusplus
}
#endif

#endif
