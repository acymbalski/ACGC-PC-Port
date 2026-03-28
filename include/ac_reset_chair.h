#ifndef AC_RESET_CHAIR_H
#define AC_RESET_CHAIR_H

#include "types.h"
#include "m_actor.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct reset_chair_actor_s RESET_CHAIR_ACTOR;

typedef void (*aRC_ACT_PROC)(RESET_CHAIR_ACTOR* chair, GAME* game);

enum {
    aRC_ACT_WAIT,
    aRC_ACT_SAILING,
    aRC_ACT_RETURN,
    aRC_ACT_HIDE,

    aRC_ACT_NUM
};

struct reset_chair_actor_s {
    /* 0x000 */ ACTOR actor_class;
    /* 0x174 */ int action;
    /* 0x178 */ aRC_ACT_PROC act_proc;
    /* 0x17C */ int timer;
    /* 0x180 */ xyz_t start_pos;
    /* 0x18C */ xyz_t target_pos;
    /* 0x198 */ f32 move_t;
};

extern ACTOR_PROFILE Reset_Chair_Profile;

#ifdef __cplusplus
}
#endif

#endif
