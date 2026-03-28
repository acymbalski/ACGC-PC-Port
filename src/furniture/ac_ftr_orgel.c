#if VERSION >= VER_DELUXE
/* fOP - Music box/Orgel furniture type functions */
/* DX Deluxe Edition - plays music when active and player is interacting */

static void fOP_ct(FTR_ACTOR* ftr_actor, u8* data) {
}

static void fOP_dt(FTR_ACTOR* ftr_actor, u8* data) {
}

static void fOP_dw(FTR_ACTOR* ftr_actor, ACTOR* my_room_actor, GAME* game, u8* data) {
}

static void fOP_mv(FTR_ACTOR* ftr_actor, ACTOR* my_room_actor, GAME* game, u8* data) {
    aMR_contact_info_c* contact_info;

    if (ftr_actor->switch_changed_flag != 0) {
        contact_info = aMR_GetContactInfoLayer1();
        if (contact_info->contact_direction == aMR_CONTACT_DIR_FRONT) {
            sAdo_OngenTrgStart(0x47C, &ftr_actor->position);
        }
    }
}
#endif /* VERSION >= VER_DELUXE */
