#if VERSION >= VER_DELUXE
/* fTC - Torch/Candle furniture type functions */
/* DX Deluxe Edition - draws flame effect display lists */

extern Gfx int_ftr_torch_body_model[];
extern Gfx int_ftr_torch_flame_model[];

static void fTC_ct(FTR_ACTOR* ftr_actor, u8* data) {
}

static void fTC_dt(FTR_ACTOR* ftr_actor, u8* data) {
}

static void fTC_mv(FTR_ACTOR* ftr_actor, ACTOR* my_room_actor, GAME* game, u8* data) {
}

static void fTC_dw(FTR_ACTOR* ftr_actor, ACTOR* my_room_actor, GAME* game, u8* data) {
    OPEN_DISP(game->graph);

    gSPMatrix(NEXT_POLY_OPA_DISP, _Matrix_to_Mtx_new(game->graph), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    gSPDisplayList(NEXT_POLY_OPA_DISP, int_ftr_torch_body_model);
    gSPDisplayList(NEXT_POLY_OPA_DISP, int_ftr_torch_flame_model);

    CLOSE_DISP(game->graph);
}
#endif /* VERSION >= VER_DELUXE */
