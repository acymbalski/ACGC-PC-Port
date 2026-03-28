#if VERSION >= VER_DELUXE
/* Deluxe: fKY - KY furniture type callbacks */
/* Complex interactive furniture: switch + contact + melody + periodic SE */
/* Reconstruction — 396 bytes in original, no ct/dt/dw (move-only type) */

static void fKY_mv(FTR_ACTOR* ftr_actor, ACTOR* my_room_actor, GAME* game, u8* data) {
    aMR_contact_info_c* contact_info;
    aMR_Clip_c* clip;
    s16 timer;

    /* Skip during birth/death transitions */
    if (!aFTR_CAN_PLAY_SE(ftr_actor)) {
        return;
    }

    /* Continuous ambient SE when switched on */
    if (ftr_actor->switch_bit != FALSE) {
        sAdo_OngenPos((u32)ftr_actor, 0x4B, &ftr_actor->position);
    }

    /* Handle switch change: on/off SE + melody start */
    if (ftr_actor->switch_changed_flag != FALSE) {
        if (ftr_actor->switch_bit != FALSE) {
            sAdo_OngenTrgStart(0x16, &ftr_actor->position);
            ftr_actor->dynamic_work_s[0] = 1;
            ftr_actor->dynamic_work_s[1] = 0;
            ftr_actor->dynamic_work_s[2] = 0;

            clip = Common_Get(clip).my_room_clip;
            if (clip != NULL) {
                clip->sound_melody_proc(ftr_actor, my_room_actor, 16);
            }
        } else {
            sAdo_OngenTrgStart(0x17, &ftr_actor->position);
            ftr_actor->dynamic_work_s[0] = 0;
            ftr_actor->dynamic_work_s[1] = 0;
            ftr_actor->dynamic_work_s[2] = 0;
        }
    }

    /* Contact interaction: directional SE triggers */
    if (ftr_actor->switch_changed_flag != FALSE) {
        contact_info = aMR_GetContactInfoLayer1();
        if (contact_info != NULL) {
            switch (contact_info->contact_direction) {
                case aMR_CONTACT_DIR_FRONT:
                    sAdo_OngenTrgStart(0x16F, &ftr_actor->position);
                    ftr_actor->dynamic_work_s[2] = 1;
                    break;
                case aMR_CONTACT_DIR_BACK:
                    sAdo_OngenTrgStart(0x170, &ftr_actor->position);
                    ftr_actor->dynamic_work_s[2] = 2;
                    break;
                case aMR_CONTACT_DIR_LEFT:
                    sAdo_OngenTrgStart(0x171, &ftr_actor->position);
                    ftr_actor->dynamic_work_s[2] = 3;
                    break;
                case aMR_CONTACT_DIR_RIGHT:
                    sAdo_OngenTrgStart(0x172, &ftr_actor->position);
                    ftr_actor->dynamic_work_s[2] = 4;
                    break;
            }
        }
    }

    /* Active state: periodic SE and auto-shutoff timer */
    if (ftr_actor->dynamic_work_s[0] != 0) {
        timer = ftr_actor->dynamic_work_s[1];
        timer++;
        ftr_actor->dynamic_work_s[1] = timer;

        if (ftr_actor->dynamic_work_s[2] != 0 && (timer % 30) == 0) {
            sAdo_OngenTrgStart(0x475, &ftr_actor->position);
        }

        if (timer >= 180) {
            ftr_actor->dynamic_work_s[0] = 0;
            ftr_actor->dynamic_work_s[1] = 0;
            ftr_actor->dynamic_work_s[2] = 0;
        }
    }
}
#endif /* VERSION >= VER_DELUXE */
