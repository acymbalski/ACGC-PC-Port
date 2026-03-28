#if VERSION >= VER_DELUXE
/* Deluxe: SumSpika01 - Summer Spika (speaker) furniture callbacks */

static void aSumSpika01_ct(FTR_ACTOR* ftr_actor, u8* data) {
}

static void aSumSpika01_mv(FTR_ACTOR* ftr_actor, ACTOR* my_room_actor, GAME* game, u8* data) {
    if (Common_Get(clip).my_room_clip != NULL) {
        (*Common_Get(clip).my_room_clip->mini_disk_common_move_proc)(ftr_actor, my_room_actor, game, 0.0f, 0.0f);
    }
}

static void aSumSpika01_dw(FTR_ACTOR* ftr_actor, ACTOR* my_room_actor, GAME* game, u8* data) {
}

static void aSumSpika01_dt(FTR_ACTOR* ftr_actor, u8* data) {
}
#endif /* VERSION >= VER_DELUXE */
