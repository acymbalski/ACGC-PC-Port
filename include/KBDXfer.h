#ifndef KBDXFER_H
#define KBDXFER_H

#include "types.h"
#include "dolphin/os.h"

#ifdef __cplusplus
extern "C" {
#endif

#define KBD_MAX_CHAN 4
#define KBD_MAX_KEYS 3

/* Error codes */
#define KBD_ERR_NONE          0
#define KBD_ERR_NO_CONTROLLER 1
#define KBD_ERR_NOT_READY     2
#define KBD_ERR_TRANSFER      3

/* Modifier flags */
#define KBD_MOD_LCTRL  (1 << 0)
#define KBD_MOD_LSHIFT (1 << 1)
#define KBD_MOD_LALT   (1 << 2)
#define KBD_MOD_LGUI   (1 << 3)
#define KBD_MOD_RCTRL  (1 << 4)
#define KBD_MOD_RSHIFT (1 << 5)
#define KBD_MOD_RALT   (1 << 6)
#define KBD_MOD_RGUI   (1 << 7)

#define KBD_MOD_SHIFT  (KBD_MOD_LSHIFT | KBD_MOD_RSHIFT)
#define KBD_MOD_CTRL   (KBD_MOD_LCTRL  | KBD_MOD_RCTRL)
#define KBD_MOD_ALT    (KBD_MOD_LALT   | KBD_MOD_RALT)

/* HID scan codes */
#define KBD_KEY_NONE      0x00
#define KBD_KEY_A         0x04
#define KBD_KEY_Z         0x1D
#define KBD_KEY_1         0x1E
#define KBD_KEY_0         0x27
#define KBD_KEY_ENTER     0x28
#define KBD_KEY_ESCAPE    0x29
#define KBD_KEY_BACKSPACE 0x2A
#define KBD_KEY_TAB       0x2B
#define KBD_KEY_SPACE     0x2C
#define KBD_KEY_F1        0x3A
#define KBD_KEY_F2        0x3B
#define KBD_KEY_F3        0x3C
#define KBD_KEY_F4        0x3D
#define KBD_KEY_F5        0x3E
#define KBD_KEY_RIGHT     0x4F
#define KBD_KEY_LEFT      0x50
#define KBD_KEY_DOWN      0x51
#define KBD_KEY_UP        0x52

typedef struct KBDStatus_s {
    u32 err;
    u8 modifiers;
    u8 keys[KBD_MAX_KEYS];
} KBDStatus;

typedef void (*KBDCallback)(s32 chan, KBDStatus* status);

extern void KBDInit(void);
extern void KBDEnable(s32 chan);
extern void KBDDisable(s32 chan);
extern s32  KBDRead(KBDStatus status[KBD_MAX_CHAN], KBDCallback callback);
extern BOOL KBDGetEvent(s32 chan, u8* key_out, BOOL* pressed_out);
extern BOOL KBDCheckEx(s32 chan);

#ifdef __cplusplus
}
#endif

#endif
