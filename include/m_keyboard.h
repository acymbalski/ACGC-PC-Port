#ifndef M_KEYBOARD_H
#define M_KEYBOARD_H

#include "types.h"
#include "KBDXfer.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GCKB_MAX_EVENTS 16

typedef struct gckb_state_s {
    BOOL enabled;
    KBDStatus status;
    KBDStatus prev_status;
    u8 event_keys[GCKB_MAX_EVENTS];
    u8 event_pressed[GCKB_MAX_EVENTS];
    int event_head;
    int event_tail;
} GCKB_State;

extern BOOL GCKB_IsAnyKeyboardEnabled(void);
extern BOOL GCKB_IsKeyboardEnabled(s32 chan);
extern void GCKB_ReadKeys(s32 chan, u8* keys, u8* modifiers);
extern BOOL GCKB_IsKeyDown(u8 scancode);
extern BOOL GCKB_IsKeyTriggered(u8 scancode);
extern void GCKB_Update(void);
extern void GCKB_UpdateOne(s32 chan);

#ifdef __cplusplus
}
#endif

#endif
