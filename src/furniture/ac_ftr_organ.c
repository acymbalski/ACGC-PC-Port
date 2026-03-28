#if VERSION >= VER_DELUXE
/* fOG - Organ furniture type functions */
/* DX Deluxe Edition - organ that plays random pipe notes */

/* TODO: replace with actual Deluxe asset data */
Gfx int_ftr_organ_pipe0_model[] = { gsSPEndDisplayList() };
Gfx int_ftr_organ_pipe1_model[] = { gsSPEndDisplayList() };
Gfx int_ftr_organ_pipe2_model[] = { gsSPEndDisplayList() };
Gfx int_ftr_organ_pipe3_model[] = { gsSPEndDisplayList() };

static Gfx* fOG_pipe_model_table[] = {
    int_ftr_organ_pipe0_model,
    int_ftr_organ_pipe1_model,
    int_ftr_organ_pipe2_model,
    int_ftr_organ_pipe3_model,
};

static void fOG_ct(FTR_ACTOR* ftr_actor, u8* data) {
    u16 name;
    int idx;

    name = ftr_actor->name;
    idx = (name - 0x58C) & 3;
    ftr_actor->dynamic_work_s[0] = idx;
}

static void fOG_mv(FTR_ACTOR* ftr_actor, ACTOR* my_room_actor, GAME* game, u8* data) {
    aMR_contact_info_c* contact_info;

    if (ftr_actor->switch_changed_flag != 0) {
        contact_info = aMR_GetContactInfoLayer1();
        if (contact_info->contact_direction == aMR_CONTACT_DIR_FRONT) {
            f32 rnd;
            u16 se_id;

            rnd = fqrand();
            se_id = (u16)((int)(4.0f * rnd) + 0x16F);
            sAdo_OngenTrgStart(se_id, &ftr_actor->position);
        }
    }
}

static void fOG_dw(FTR_ACTOR* ftr_actor, ACTOR* my_room_actor, GAME* game, u8* data) {
    Gfx* pipe_model;

    pipe_model = fOG_pipe_model_table[ftr_actor->dynamic_work_s[0]];

    OPEN_DISP(game->graph);

    gSPMatrix(NEXT_POLY_OPA_DISP, _Matrix_to_Mtx_new(game->graph), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    gSPSegment(NEXT_POLY_OPA_DISP, G_MWO_SEGMENT_8, pipe_model);
    gSPDisplayList(NEXT_POLY_OPA_DISP, pipe_model);

    CLOSE_DISP(game->graph);
}
#endif /* VERSION >= VER_DELUXE */
