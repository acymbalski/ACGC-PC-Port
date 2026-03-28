#if VERSION >= VER_DELUXE
/* fIN - Irori (Japanese sunken hearth) furniture type functions */
/* DX Deluxe Edition - animated hearth with skeleton keyframe */

extern cKF_Skeleton_R_c cKF_bs_r_int_irori;
extern cKF_Animation_R_c cKF_ba_r_int_irori;

static void fIN_ct(FTR_ACTOR* ftr_actor, u8* data) {
    cKF_SkeletonInfo_R_c* keyframe;

    keyframe = &ftr_actor->keyframe;
    cKF_SkeletonInfo_R_ct(keyframe, &cKF_bs_r_int_irori, &cKF_ba_r_int_irori, ftr_actor->joint,
                          ftr_actor->morph);
    cKF_SkeletonInfo_R_init_standard_repeat(keyframe, &cKF_ba_r_int_irori, NULL);
    keyframe->frame_control.speed = 0.5f;
    cKF_SkeletonInfo_R_play(keyframe);
}

static void fIN_dt(FTR_ACTOR* ftr_actor, u8* data) {
}

static void fIN_mv(FTR_ACTOR* ftr_actor, ACTOR* my_room_actor, GAME* game, u8* data) {
    cKF_SkeletonInfo_R_c* keyframe;
    f32 delta;
    f32 delta2;
    f32* extra_f;

    keyframe = &ftr_actor->keyframe;
    cKF_SkeletonInfo_R_play(keyframe);
    keyframe->frame_control.speed = 0.5f;

    /* Check if fire Y position crossed zero to trigger sizzle SE */
    extra_f = (f32*)&ftr_actor->layer;
    delta = ftr_actor->dynamic_work_f[1] - ftr_actor->dynamic_work_f[0];
    if (delta < 0.0f) {
        delta2 = *extra_f - ftr_actor->dynamic_work_f[1];
        if (delta2 > 0.0f) {
            sAdo_OngenTrgStart(0x475, &ftr_actor->position);
        }
    }
}

static xyz_t fIN_zero_pos = { 0.0f, 0.0f, 0.0f };

static int fIN_DwBefore(GAME* game, cKF_SkeletonInfo_R_c* keyframe, int joint_idx, Gfx** joint_shape,
                        u8* joint_flags, void* arg, s_xyz* joint_rot, xyz_t* joint_pos) {
    return TRUE;
}

static int fIN_DwAfter(GAME* game, cKF_SkeletonInfo_R_c* keyframe, int joint_idx, Gfx** joint_shape,
                       u8* joint_flags, void* arg, s_xyz* joint_rot, xyz_t* joint_pos) {
    FTR_ACTOR* ftr_actor = (FTR_ACTOR*)arg;
    xyz_t pos;
    f32* extra_f;

    if (ftr_actor == NULL) {
        return TRUE;
    }

    /* Skip during birth/bye/death/birth_wait states */
    if (ftr_actor->state == aFTR_STATE_BIRTH || ftr_actor->state == aFTR_STATE_BYE ||
        ftr_actor->state == aFTR_STATE_DEATH || ftr_actor->state == aFTR_STATE_BIRTH_WAIT) {
        return TRUE;
    }

    pos = fIN_zero_pos;

    if (joint_idx == 5) {
        xyz_t result;

        Matrix_Position(&pos, &result);
        /* Shift Y position history: f[0] = f[1], f[1] = extra, extra = new y */
        extra_f = (f32*)&ftr_actor->layer;
        ftr_actor->dynamic_work_f[0] = ftr_actor->dynamic_work_f[1];
        ftr_actor->dynamic_work_f[1] = *extra_f;
        *extra_f = result.y;
    }

    return TRUE;
}

static void fIN_dw(FTR_ACTOR* ftr_actor, ACTOR* my_room_actor, GAME* game, u8* data) {
    cKF_SkeletonInfo_R_c* keyframe;
    Mtx* mtx;

    keyframe = &ftr_actor->keyframe;
    mtx = ftr_actor->skeleton_mtx[game->frame_counter & 1];
    cKF_Si3_draw_R_SV(game, keyframe, mtx, &fIN_DwBefore, &fIN_DwAfter, ftr_actor);
}
#endif /* VERSION >= VER_DELUXE */
