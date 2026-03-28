#include "types.h"
#if VERSION >= VER_DELUXE
#include "KBDXfer.h"

#include "dolphin/si.h"
#include "dolphin/os/OSInterrupt.h"

#define KBD_CMD_POLL 0x54000000
#define KBD_CMD_OUT_BYTES 1
#define KBD_CMD_IN_BYTES  8

#define KBD_EVENT_RING_SIZE 16

typedef struct KBDChan_s {
    BOOL enabled;
    BOOL connected;
    BOOL busy;
    KBDStatus status;
    KBDStatus prev_status;
    KBDCallback callback;
    u8 event_keys[KBD_EVENT_RING_SIZE];
    BOOL event_pressed[KBD_EVENT_RING_SIZE];
    int event_head;
    int event_tail;
} KBDChan;

static KBDChan sKBDChan[KBD_MAX_CHAN];
static BOOL sKBDInitialized;
static u32 sKBDCmdBlock;

static void __KBDDefaultCallback(s32 chan, KBDStatus* status) {
}

static void MakeStatus(s32 chan, void* data) {
    KBDChan* kc = &sKBDChan[chan];
    u8* bytes = (u8*)data;

    kc->status.err = KBD_ERR_NONE;
    kc->status.modifiers = bytes[0];
    kc->status.keys[0] = bytes[2];
    kc->status.keys[1] = bytes[3];
    kc->status.keys[2] = bytes[4];
    kc->connected = TRUE;
}

static BOOL KeyInArray(u8 key, u8* arr, int count) {
    int i;
    for (i = 0; i < count; i++) {
        if (arr[i] == key) return TRUE;
    }
    return FALSE;
}

static void PushEvent(KBDChan* kc, u8 key, BOOL pressed) {
    int next = (kc->event_head + 1) % KBD_EVENT_RING_SIZE;
    if (next == kc->event_tail) return; /* ring full, drop event */
    kc->event_keys[kc->event_head] = key;
    kc->event_pressed[kc->event_head] = pressed;
    kc->event_head = next;
}

static void UpdateEvents(s32 chan) {
    KBDChan* kc = &sKBDChan[chan];
    int i;

    if (kc->status.err != KBD_ERR_NONE) return;

    /* Detect released keys (in prev but not in current) */
    for (i = 0; i < KBD_MAX_KEYS; i++) {
        u8 key = kc->prev_status.keys[i];
        if (key != KBD_KEY_NONE && !KeyInArray(key, kc->status.keys, KBD_MAX_KEYS)) {
            PushEvent(kc, key, FALSE);
        }
    }

    /* Detect pressed keys (in current but not in prev) */
    for (i = 0; i < KBD_MAX_KEYS; i++) {
        u8 key = kc->status.keys[i];
        if (key != KBD_KEY_NONE && !KeyInArray(key, kc->prev_status.keys, KBD_MAX_KEYS)) {
            PushEvent(kc, key, TRUE);
        }
    }

    kc->prev_status = kc->status;
}

static void KBDSICallback(s32 chan, u32 sr, OSContext* context) {
    KBDChan* kc = &sKBDChan[chan];

    kc->busy = FALSE;

    if (sr & SI_ERROR_NO_RESPONSE) {
        kc->status.err = KBD_ERR_NO_CONTROLLER;
        kc->connected = FALSE;
    } else {
        u32 data[2];
        SIGetResponse(chan, data);
        MakeStatus(chan, data);
        UpdateEvents(chan);
    }

    if (kc->callback != NULL) {
        kc->callback(chan, &kc->status);
    }
}

static void KBDTypeAndStatusCallback(s32 chan, u32 type) {
    KBDChan* kc = &sKBDChan[chan];

    if ((type & SI_TYPE_MASK) == SI_TYPE_GC && (type & 0x00200000)) {
        kc->connected = TRUE;
        kc->enabled = TRUE;
    } else {
        kc->connected = FALSE;
        kc->enabled = FALSE;
    }
}

void KBDInit(void) {
    int i;

    if (sKBDInitialized) return;

    for (i = 0; i < KBD_MAX_CHAN; i++) {
        KBDChan* kc = &sKBDChan[i];
        kc->enabled = FALSE;
        kc->connected = FALSE;
        kc->busy = FALSE;
        kc->callback = &__KBDDefaultCallback;
        kc->event_head = 0;
        kc->event_tail = 0;
        kc->status.err = KBD_ERR_NOT_READY;

        /* Probe channel for keyboard */
        SIGetTypeAsync(i, &KBDTypeAndStatusCallback);
    }

    sKBDCmdBlock = KBD_CMD_POLL;
    sKBDInitialized = TRUE;
}

void KBDEnable(s32 chan) {
    if (chan < 0 || chan >= KBD_MAX_CHAN) return;
    sKBDChan[chan].enabled = TRUE;
}

void KBDDisable(s32 chan) {
    if (chan < 0 || chan >= KBD_MAX_CHAN) return;
    sKBDChan[chan].enabled = FALSE;
}

s32 KBDRead(KBDStatus status[KBD_MAX_CHAN], KBDCallback callback) {
    int i;
    s32 count = 0;

    if (!sKBDInitialized) return 0;

    for (i = 0; i < KBD_MAX_CHAN; i++) {
        KBDChan* kc = &sKBDChan[i];

        if (!kc->enabled || kc->busy) {
            if (status != NULL) {
                status[i].err = kc->enabled ? KBD_ERR_TRANSFER : KBD_ERR_NO_CONTROLLER;
            }
            continue;
        }

        kc->callback = (callback != NULL) ? callback : &__KBDDefaultCallback;
        kc->busy = TRUE;

        SITransfer(i, &sKBDCmdBlock, KBD_CMD_OUT_BYTES, &kc->status, KBD_CMD_IN_BYTES,
                   &KBDSICallback, 0);
        count++;
    }

    /* Copy current status to output */
    if (status != NULL) {
        for (i = 0; i < KBD_MAX_CHAN; i++) {
            status[i] = sKBDChan[i].status;
        }
    }

    return count;
}

BOOL KBDGetEvent(s32 chan, u8* key_out, BOOL* pressed_out) {
    KBDChan* kc;

    if (chan < 0 || chan >= KBD_MAX_CHAN) return FALSE;

    kc = &sKBDChan[chan];
    if (kc->event_tail == kc->event_head) return FALSE;

    *key_out = kc->event_keys[kc->event_tail];
    *pressed_out = kc->event_pressed[kc->event_tail];
    kc->event_tail = (kc->event_tail + 1) % KBD_EVENT_RING_SIZE;
    return TRUE;
}

BOOL KBDCheckEx(s32 chan) {
    if (chan < 0 || chan >= KBD_MAX_CHAN) return FALSE;
    return sKBDChan[chan].connected && sKBDChan[chan].enabled;
}
#endif /* VERSION >= VER_DELUXE */
