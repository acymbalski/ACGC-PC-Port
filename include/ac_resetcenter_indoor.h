#ifndef AC_RESETCENTER_INDOOR_H
#define AC_RESETCENTER_INDOOR_H

#include "types.h"
#include "m_actor.h"
#include "m_lights.h"
#include "c_keyframe.h"

#ifdef __cplusplus
extern "C" {
#endif

#define aRI_LIGHT_NUM 2
#define aRI_MODE_NORMAL 0
#define aRI_MODE_ANGRY 1
#define aRI_MODE_CALM 2

typedef struct aRI_door_c {
    f32 alpha;
    f32 target_alpha;
    f32 speed;
    int state;
} aRI_door_c;

typedef struct aRI_light_c {
    Lights light;
    Light_list* light_list;
    int state;
    int target_state;
    f32 intensity;
    f32 target_intensity;
    xyz_t position;
} aRI_light_c;

typedef struct aRI_tv_c {
    int state;
    int timer;
    int frame;
    f32 flicker;
    rgba_t color;
} aRI_tv_c;

typedef struct resetcenter_indoor_actor_s {
    /* 0x000 */ ACTOR actor_class;
    /* 0x174 */ int mode;
    /* 0x178 */ int next_mode;
    /* 0x17C */ int mode_timer;
    /* 0x180 */ aRI_door_c door;
    /* 0x190 */ aRI_light_c lights[aRI_LIGHT_NUM];
    /* 0x??? */ aRI_tv_c tv;
    /* 0x??? */ u8 room_prim_r;
    /* 0x??? */ u8 room_prim_g;
    /* 0x??? */ u8 room_prim_b;
    /* 0x??? */ u8 pad;
} RESETCENTER_INDOOR_ACTOR;

extern ACTOR_PROFILE Resetcenter_Indoor_Profile;

#ifdef __cplusplus
}
#endif

#endif
