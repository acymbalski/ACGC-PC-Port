#if VERSION >= VER_DELUXE
/* Deluxe: fILP - ILP (Item Lamp) furniture type callbacks */
/* Plays positional SE and draws lamp with texture scrolling */

extern Gfx int_ftr_ilp_body_model[];
extern Gfx int_ftr_ilp_shade_model[];

static void fILP_mv(FTR_ACTOR* ftr_actor, ACTOR* my_room_actor, GAME* game, u8* data) {
    if (ftr_actor->state != aFTR_STATE_BIRTH &&
        ftr_actor->state != aFTR_STATE_BYE &&
        ftr_actor->state != aFTR_STATE_DEATH &&
        ftr_actor->state != aFTR_STATE_BIRTH_WAIT) {
        sAdo_OngenPos((u32)ftr_actor, 0x4B, &ftr_actor->position);
    }
}

static void fILP_dw(FTR_ACTOR* ftr_actor, ACTOR* my_room_actor, GAME* game, u8* data) {
    GAME_PLAY* play;
    Gfx* scroll_gfx;
    u32 ctr;

    play = (GAME_PLAY*)game;

    if (ftr_actor->ctr_type == aFTR_CTR_TYPE_GAME_PLAY) {
        ctr = play->game_frame;
    } else {
        ctr = game->frame_counter;
    }

    scroll_gfx = tex_scroll2_dolphin(game->graph, 0, ctr * 5, 16, 16);
    if (scroll_gfx != NULL) {
        OPEN_DISP(game->graph);

        gSPMatrix(NEXT_POLY_OPA_DISP, _Matrix_to_Mtx_new(game->graph), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
        gSPDisplayList(NEXT_POLY_OPA_DISP, int_ftr_ilp_body_model);
        gSPSegment(NEXT_POLY_OPA_DISP, G_MWO_SEGMENT_8, scroll_gfx);
        gSPDisplayList(NEXT_POLY_OPA_DISP, int_ftr_ilp_shade_model);

        CLOSE_DISP(game->graph);
    }
}
#endif /* VERSION >= VER_DELUXE */
