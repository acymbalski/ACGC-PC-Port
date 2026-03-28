#ifndef AC_MONUMENT_H
#define AC_MONUMENT_H

#include "types.h"
#include "m_actor.h"
#include "c_keyframe.h"
#include "m_lights.h"

#ifdef __cplusplus
extern "C" {
#endif

#define aMNM_JOINT_NUM 15

enum monument_type {
    aMNM_TYPE_CLOCK,
    aMNM_TYPE_FLOWERCLOCK,
    aMNM_TYPE_WINDMILL,
    aMNM_TYPE_FOUNTAIN,
    aMNM_TYPE_STATUE,
    aMNM_TYPE_BENCH,
    aMNM_TYPE_STREETLIGHT,
    aMNM_TYPE_BELL,

    aMNM_TYPE_NUM
};

enum monument_draw_mode {
    aMNM_DRAW_WITH_ANIME,
    aMNM_DRAW_ONLY_MODEL,

    aMNM_DRAW_NUM
};

typedef struct actor_monument_s MONUMENT_ACTOR;

struct actor_monument_s {
    /* 0x000 */ ACTOR actor_class;
    /* 0x174 */ int monument_type;
    /* 0x178 */ int draw_mode;
    /* 0x17C */ cKF_SkeletonInfo_R_c keyframe;
    /* 0x1EC */ s_xyz work_area[aMNM_JOINT_NUM];
    /* 0x246 */ s_xyz morph_area[aMNM_JOINT_NUM];
    /* 0x2A0 */ Lights monument_light;
    /* 0x2B0 */ Light_list* light_node;
    /* 0x2B4 */ f32 blade_angle;
    /* 0x2B8 */ char* bank_ram;
    /* 0x2BC */ int bg_offset_set;
};

extern ACTOR_PROFILE Monument_Profile;

#ifdef __cplusplus
}
#endif

#endif
