#if VERSION >= VER_DELUXE
/* fOD - Odor furniture type functions */
/* DX Deluxe Edition - furniture that plays scent/smoke melody */

static void fOD_ct(FTR_ACTOR* ftr_actor, u8* data) {
}

static void fOD_dt(FTR_ACTOR* ftr_actor, u8* data) {
}

static void fOD_dw(FTR_ACTOR* ftr_actor, ACTOR* my_room_actor, GAME* game, u8* data) {
}

static void fOD_mv(FTR_ACTOR* ftr_actor, ACTOR* my_room_actor, GAME* game, u8* data) {
    aMR_Clip_c* clip;

    clip = Common_Get(clip).my_room_clip;
    if (clip != NULL) {
        clip->sound_melody_proc(ftr_actor, my_room_actor, 16);
    }
}
#endif /* VERSION >= VER_DELUXE */
