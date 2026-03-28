#include "types.h"
#if VERSION >= VER_DELUXE
#include "m_keyboard.h"

static GCKB_State sGCKB[KBD_MAX_CHAN];
static BOOL sGCKB_Initialized;

static void GCKB_ReadCallback(s32 chan, KBDStatus* status) {
    if (chan < 0 || chan >= KBD_MAX_CHAN) return;

    if (status->err == KBD_ERR_NONE) {
        sGCKB[chan].enabled = TRUE;
        sGCKB[chan].status = *status;
    } else if (status->err == KBD_ERR_NO_CONTROLLER) {
        sGCKB[chan].enabled = FALSE;
    }
}

BOOL GCKB_IsAnyKeyboardEnabled(void) {
    int i;
    for (i = 0; i < KBD_MAX_CHAN; i++) {
        if (sGCKB[i].enabled) return TRUE;
    }
    return FALSE;
}

BOOL GCKB_IsKeyboardEnabled(s32 chan) {
    if (chan < 0 || chan >= KBD_MAX_CHAN) return FALSE;
    return sGCKB[chan].enabled;
}

void GCKB_ReadKeys(s32 chan, u8* keys, u8* modifiers) {
    if (chan < 0 || chan >= KBD_MAX_CHAN) return;

    if (sGCKB[chan].enabled) {
        int i;
        for (i = 0; i < KBD_MAX_KEYS; i++) {
            keys[i] = sGCKB[chan].status.keys[i];
        }
        *modifiers = sGCKB[chan].status.modifiers;
    } else {
        int i;
        for (i = 0; i < KBD_MAX_KEYS; i++) {
            keys[i] = KBD_KEY_NONE;
        }
        *modifiers = 0;
    }
}

BOOL GCKB_IsKeyDown(u8 scancode) {
    int i, j;
    for (i = 0; i < KBD_MAX_CHAN; i++) {
        if (sGCKB[i].enabled) {
            for (j = 0; j < KBD_MAX_KEYS; j++) {
                if (sGCKB[i].status.keys[j] == scancode) return TRUE;
            }
        }
    }
    return FALSE;
}

BOOL GCKB_IsKeyTriggered(u8 scancode) {
    int i, j;
    for (i = 0; i < KBD_MAX_CHAN; i++) {
        if (sGCKB[i].enabled) {
            BOOL down_now = FALSE;
            BOOL down_prev = FALSE;
            for (j = 0; j < KBD_MAX_KEYS; j++) {
                if (sGCKB[i].status.keys[j] == scancode) down_now = TRUE;
                if (sGCKB[i].prev_status.keys[j] == scancode) down_prev = TRUE;
            }
            if (down_now && !down_prev) return TRUE;
        }
    }
    return FALSE;
}

void GCKB_UpdateOne(s32 chan) {
    GCKB_State* gs;
    u8 key;
    BOOL pressed;

    if (chan < 0 || chan >= KBD_MAX_CHAN) return;

    gs = &sGCKB[chan];
    gs->prev_status = gs->status;

    /* Drain events from KBDXfer into our ring buffer */
    while (KBDGetEvent(chan, &key, &pressed)) {
        int next = (gs->event_head + 1) % GCKB_MAX_EVENTS;
        if (next == gs->event_tail) break; /* full */
        gs->event_keys[gs->event_head] = key;
        gs->event_pressed[gs->event_head] = pressed;
        gs->event_head = next;
    }
}

void GCKB_Update(void) {
    int i;

    if (!sGCKB_Initialized) {
        KBDInit();
        sGCKB_Initialized = TRUE;
    }

    /* Poll all channels */
    KBDRead(NULL, &GCKB_ReadCallback);

    /* Update per-channel state */
    for (i = 0; i < KBD_MAX_CHAN; i++) {
        if (sGCKB[i].enabled) {
            GCKB_UpdateOne(i);
        }
    }
}
#endif /* VERSION >= VER_DELUXE */
