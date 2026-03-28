#ifndef AC_NPC_RESET_H
#define AC_NPC_RESET_H

#include "types.h"
#include "m_actor.h"
#include "ac_npc.h"

#if VERSION >= VER_DELUXE

#ifdef __cplusplus
extern "C" {
#endif

typedef struct npc_reset_actor_s NPC_RESET_ACTOR;

typedef void (*aRST_THINK_PROC)(NPC_RESET_ACTOR* actor, GAME_PLAY* play);
typedef void (*aRST_TALK_PROC)(NPC_RESET_ACTOR* actor, GAME_PLAY* play);

/* Sailing states */
enum {
    aRST_SAILING_IDLE,
    aRST_SAILING_APPROACH,
    aRST_SAILING_ARRIVE,
    aRST_SAILING_DEPART,

    aRST_SAILING_NUM
};

/* Think indices */
enum {
    aRST_THINK_NORMAL_WAIT,
    aRST_THINK_READY_SAILING,
    aRST_THINK_SAILING,
    aRST_THINK_SETTLEMENT,
    aRST_THINK_HIDE,
    aRST_THINK_RETURN,
    aRST_THINK_RESET_AINOTE,
    aRST_THINK_RACKET_AINOTE,
    aRST_THINK_HUNT,
    aRST_THINK_REMAIN,
    aRST_THINK_TIMER,
    aRST_THINK_START,
    aRST_THINK_START2,
    aRST_THINK_START3,
    aRST_THINK_END,
    aRST_THINK_TIMER_SAILING,

    aRST_THINK_NUM
};

/* Talk states */
enum {
    aRST_TALK_BYEBYE,
    aRST_TALK_EXIT,
    aRST_TALK_STOP_PLAYER,
    aRST_TALK_AINOTE_0,
    aRST_TALK_AINOTE_1,
    aRST_TALK_END_WAIT,
    aRST_TALK_END_WAIT3,
    aRST_TALK_4_1,
    aRST_TALK_4_2,
    aRST_TALK_4_3,
    aRST_TALK_4_4,
    aRST_TALK_4_5,

    aRST_TALK_NUM
};

struct npc_reset_actor_s {
    /* 0x000 */ NPC_ACTOR npc_class;
    /* Sailing data */
    /* 0x994 */ int sailing_state;
    /* 0x998 */ int sailing_timer;
    /* 0x99C */ xyz_t sailing_start;
    /* 0x9A8 */ xyz_t sailing_target;
    /* 0x9B4 */ f32 sailing_t;
    /* Think/talk state */
    /* 0x9B8 */ int think_idx;
    /* 0x9BC */ aRST_THINK_PROC think_proc;
    /* 0x9C0 */ int talk_idx;
    /* 0x9C4 */ aRST_TALK_PROC talk_proc;
    /* 0x9C8 */ int talk_end;
    /* Timer */
    /* 0x9CC */ int timer;
    /* 0x9D0 */ int timer2;
    /* Look range */
    /* 0x9D4 */ f32 look_range;
    /* Flags */
    /* 0x9D8 */ u8 force_talk_flag;
    /* 0x9D9 */ u8 norm_talk_flag;
    /* 0x9DA */ u8 head_request_set;
    /* 0x9DB */ u8 sailing_done;
};

extern ACTOR_PROFILE Npc_Reset_Profile;

#ifdef __cplusplus
}
#endif

#endif /* VERSION >= VER_DELUXE */

#endif /* AC_NPC_RESET_H */
