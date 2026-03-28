#include "types.h"
#if VERSION >= VER_DELUXE
#include "ac_monument.h"

#include "m_common_data.h"
#include "m_name_table.h"
#include "m_field_info.h"
#include "m_collision_bg.h"
#include "m_rcp.h"
#include "m_play.h"
#include "sys_matrix.h"
#include "m_lib.h"
#include "graph.h"

/* === External model/skeleton data (provided by object banks) === */

extern cKF_Skeleton_R_c cKF_bs_r_obj_monument_clock;
extern cKF_Animation_R_c cKF_ba_r_obj_monument_clock;
extern Gfx obj_monument_clock_model[];

extern cKF_Skeleton_R_c cKF_bs_r_obj_monument_flowerclock;
extern cKF_Animation_R_c cKF_ba_r_obj_monument_flowerclock;
extern Gfx obj_monument_flowerclock_model[];

extern cKF_Skeleton_R_c cKF_bs_r_obj_monument_windmill;
extern cKF_Animation_R_c cKF_ba_r_obj_monument_windmill;
extern Gfx obj_monument_windmill_model[];

extern Gfx obj_monument_fountain_model[];
extern Gfx obj_monument_statue_model[];
extern Gfx obj_monument_bench_model[];
extern Gfx obj_monument_streetlight_model[];
extern Gfx obj_monument_bell_model[];

/* === Forward declarations === */

static void aMNM_actor_ct(ACTOR* actor, GAME* game);
static void aMNM_actor_dt(ACTOR* actor, GAME* game);
static void aMNM_actor_init(ACTOR* actor, GAME* game);
static void aMNM_actor_move(ACTOR* actor, GAME* game);
static void aMNM_actor_draw_with_anime(ACTOR* actor, GAME* game);
static void aMNM_actor_draw_only_model(ACTOR* actor, GAME* game);

/* === Profile === */

/* clang-format off */
ACTOR_PROFILE Monument_Profile = {
    mAc_PROFILE_MONUMENT,
    ACTOR_PART_ITEM,
    ACTOR_STATE_NO_MOVE_WHILE_CULLED | ACTOR_STATE_NO_DRAW_WHILE_CULLED,
    EMPTY_NO,
    ACTOR_OBJ_BANK_KEEP,
    sizeof(MONUMENT_ACTOR),
    &aMNM_actor_ct,
    &aMNM_actor_dt,
    &aMNM_actor_init,
    NONE_ACTOR_PROC,
    NULL,
};
/* clang-format on */

/* === Monument type data === */

typedef struct monument_data_s {
    cKF_Skeleton_R_c* skeleton;
    cKF_Animation_R_c* animation;
    Gfx* model;
    int draw_mode;
    f32 cull_size;
} aMNM_data_c;

/* clang-format off */
static aMNM_data_c aMNM_type_data[aMNM_TYPE_NUM] = {
    { &cKF_bs_r_obj_monument_clock, &cKF_ba_r_obj_monument_clock, obj_monument_clock_model, aMNM_DRAW_WITH_ANIME, 600.0f },
    { &cKF_bs_r_obj_monument_flowerclock, &cKF_ba_r_obj_monument_flowerclock, obj_monument_flowerclock_model, aMNM_DRAW_WITH_ANIME, 600.0f },
    { &cKF_bs_r_obj_monument_windmill, &cKF_ba_r_obj_monument_windmill, obj_monument_windmill_model, aMNM_DRAW_WITH_ANIME, 800.0f },
    { NULL, NULL, obj_monument_fountain_model, aMNM_DRAW_ONLY_MODEL, 500.0f },
    { NULL, NULL, obj_monument_statue_model, aMNM_DRAW_ONLY_MODEL, 500.0f },
    { NULL, NULL, obj_monument_bench_model, aMNM_DRAW_ONLY_MODEL, 400.0f },
    { NULL, NULL, obj_monument_streetlight_model, aMNM_DRAW_ONLY_MODEL, 500.0f },
    { NULL, NULL, obj_monument_bell_model, aMNM_DRAW_ONLY_MODEL, 600.0f },
};
/* clang-format on */

/* === Background offset tables === */

static mCoBG_OffsetTable_c aMNM_height_table_22_ct[4] = {
    { 0x64, 12, 12, 12, 12, 12, 1 },
    { 0x64, 12, 12, 12, 12, 12, 0 },
    { 0x64, 12, 12, 12, 12, 12, 0 },
    { 0x64, 12, 12, 12, 12, 12, 1 },
};

static mCoBG_OffsetTable_c aMNM_height_table_22_dt[4] = {
    { 0x64, 0, 0, 0, 0, 0, 0 },
    { 0x64, 0, 0, 0, 0, 0, 0 },
    { 0x64, 0, 0, 0, 0, 0, 0 },
    { 0x64, 0, 0, 0, 0, 0, 0 },
};

static mCoBG_OffsetTable_c aMNM_height_table_33_ct[9] = {
    { 0x64, 12, 0, 12, 12, 12, 1 },
    { 0x64, 12, 12, 12, 12, 12, 0 },
    { 0x64, 12, 12, 12, 12, 0, 1 },
    { 0x64, 12, 12, 12, 12, 12, 0 },
    { 0x64, 12, 12, 12, 12, 12, 0 },
    { 0x64, 12, 12, 12, 12, 12, 0 },
    { 0x64, 12, 12, 0, 12, 12, 1 },
    { 0x64, 12, 12, 12, 12, 12, 0 },
    { 0x64, 12, 12, 12, 0, 12, 1 },
};

static mCoBG_OffsetTable_c aMNM_height_table_33_dt[9] = {
    { 0x64, 0, 0, 0, 0, 0, 0 },
    { 0x64, 0, 0, 0, 0, 0, 0 },
    { 0x64, 0, 0, 0, 0, 0, 0 },
    { 0x64, 0, 0, 0, 0, 0, 0 },
    { 0x64, 0, 0, 0, 0, 0, 0 },
    { 0x64, 0, 0, 0, 0, 0, 0 },
    { 0x64, 0, 0, 0, 0, 0, 0 },
    { 0x64, 0, 0, 0, 0, 0, 0 },
    { 0x64, 0, 0, 0, 0, 0, 0 },
};

/* ============================================================ */
/* aMNM_set_bgOffset (388B)                                     */
/* Set background collision height offsets around the monument  */
/* ============================================================ */
static void aMNM_set_bgOffset(MONUMENT_ACTOR* monument) {
    int type = monument->monument_type;
    int is_large;
    xyz_t pos;

    is_large = (type == aMNM_TYPE_WINDMILL || type == aMNM_TYPE_FOUNTAIN);

    if (is_large) {
        /* 3x3 grid */
        static f32 addX[3] = { -40.0f, 0.0f, 40.0f };
        static f32 addZ[3] = { -40.0f, 0.0f, 40.0f };

        mCoBG_OffsetTable_c* offset;
        int i;

        if (monument->bg_offset_set) {
            offset = aMNM_height_table_33_ct;
        } else {
            offset = aMNM_height_table_33_dt;
        }

        for (i = 0; i < 3; i++) {
            pos.z = monument->actor_class.home.position.z + addZ[i];

            pos.x = monument->actor_class.home.position.x + addX[0];
            mCoBG_SetPluss5PointOffset_file(pos, offset[0], __FILE__, __LINE__);

            pos.x = monument->actor_class.home.position.x + addX[1];
            mCoBG_SetPluss5PointOffset_file(pos, offset[1], __FILE__, __LINE__);

            pos.x = monument->actor_class.home.position.x + addX[2];
            mCoBG_SetPluss5PointOffset_file(pos, offset[2], __FILE__, __LINE__);

            offset += 3;
        }
    } else {
        /* 2x2 grid */
        static f32 addX2[2] = { -20.0f, 20.0f };
        static f32 addZ2[2] = { -20.0f, 20.0f };

        mCoBG_OffsetTable_c* offset;
        int i;

        if (monument->bg_offset_set) {
            offset = aMNM_height_table_22_ct;
        } else {
            offset = aMNM_height_table_22_dt;
        }

        for (i = 0; i < 2; i++) {
            pos.z = monument->actor_class.home.position.z + addZ2[i];

            pos.x = monument->actor_class.home.position.x + addX2[0];
            mCoBG_SetPluss5PointOffset_file(pos, offset[0], __FILE__, __LINE__);

            pos.x = monument->actor_class.home.position.x + addX2[1];
            mCoBG_SetPluss5PointOffset_file(pos, offset[1], __FILE__, __LINE__);

            offset += 2;
        }
    }
}

/* ============================================================ */
/* aMNM_setup_light (208B)                                      */
/* Set up a point light for the monument                        */
/* ============================================================ */
static void aMNM_setup_light(MONUMENT_ACTOR* monument, GAME* game) {
    GAME_PLAY* play = (GAME_PLAY*)game;
    Global_light* glight = &play->global_light;

    Light_point_ct(&monument->monument_light,
                   (s16)monument->actor_class.world.position.x,
                   (s16)(monument->actor_class.world.position.y + 40.0f),
                   (s16)monument->actor_class.world.position.z,
                   200, 200, 180, 200);

    monument->light_node = Global_light_list_new(game, glight, &monument->monument_light);
}

/* ============================================================ */
/* aMNM_draw_init_with_anime (208B)                             */
/* Initialize the skeleton/keyframe for animated monuments      */
/* ============================================================ */
static void aMNM_draw_init_with_anime(MONUMENT_ACTOR* monument) {
    int type = monument->monument_type;
    aMNM_data_c* data = &aMNM_type_data[type];
    cKF_SkeletonInfo_R_c* keyframe = &monument->keyframe;

    if (data->skeleton != NULL && data->animation != NULL) {
        cKF_SkeletonInfo_R_ct(keyframe, data->skeleton, data->animation,
                              monument->work_area, monument->morph_area);
        cKF_SkeletonInfo_R_init_standard_repeat(keyframe, data->animation, NULL);
        cKF_SkeletonInfo_R_play(keyframe);
        keyframe->frame_control.speed = 0.5f;
    }

    monument->draw_mode = aMNM_DRAW_WITH_ANIME;
    monument->actor_class.dw_proc = &aMNM_actor_draw_with_anime;
}

/* ============================================================ */
/* aMNM_draw_init_only_model (16B)                              */
/* Initialize draw mode for static (non-animated) monuments     */
/* ============================================================ */
static void aMNM_draw_init_only_model(MONUMENT_ACTOR* monument) {
    monument->draw_mode = aMNM_DRAW_ONLY_MODEL;
    monument->actor_class.dw_proc = &aMNM_actor_draw_only_model;
}

/* ============================================================ */
/* aMNM_actor_ct (420B)                                         */
/* Constructor — initialize the monument actor                  */
/* ============================================================ */
static void aMNM_actor_ct(ACTOR* actor, GAME* game) {
    MONUMENT_ACTOR* monument = (MONUMENT_ACTOR*)actor;
    GAME_PLAY* play = (GAME_PLAY*)game;
    int type;
    aMNM_data_c* data;

    /* Determine monument type from actor_specific field */
    type = actor->actor_specific;
    if (type < 0 || type >= aMNM_TYPE_NUM) {
        type = aMNM_TYPE_FOUNTAIN;
    }
    monument->monument_type = type;
    data = &aMNM_type_data[type];

    /* Get object bank RAM pointer */
    monument->bank_ram = play->object_exchange.banks[actor->data_bank_id].ram_start;

    /* Set up display parameters */
    actor->cull_width = data->cull_size;
    actor->cull_radius = data->cull_size;

    /* Get ground height */
    actor->world.position.y = mCoBG_GetBgY_OnlyCenter_FromWpos2(actor->world.position, 0.0f);

    /* Initialize animation data */
    monument->blade_angle = 0.0f;
    monument->bg_offset_set = TRUE;
    monument->light_node = NULL;

    /* Initialize draw based on type */
    if (data->draw_mode == aMNM_DRAW_WITH_ANIME) {
        aMNM_draw_init_with_anime(monument);
    } else {
        aMNM_draw_init_only_model(monument);
    }

    /* Set background collision offsets */
    aMNM_set_bgOffset(monument);

    /* Set up lighting */
    aMNM_setup_light(monument, game);
}

/* ============================================================ */
/* aMNM_actor_dt (100B)                                         */
/* Destructor — clean up monument resources                     */
/* ============================================================ */
static void aMNM_actor_dt(ACTOR* actor, GAME* game) {
    MONUMENT_ACTOR* monument = (MONUMENT_ACTOR*)actor;
    GAME_PLAY* play = (GAME_PLAY*)game;

    /* Clear background offsets */
    monument->bg_offset_set = FALSE;
    aMNM_set_bgOffset(monument);

    /* Free skeleton info if animated */
    if (monument->draw_mode == aMNM_DRAW_WITH_ANIME) {
        cKF_SkeletonInfo_R_dt(&monument->keyframe);
    }

    /* Remove light from global list */
    if (monument->light_node != NULL) {
        Global_light_list_delete(&play->global_light, monument->light_node);
        monument->light_node = NULL;
    }
}

/* ============================================================ */
/* aMNM_actor_init (104B)                                       */
/* Init — called after ct, sets FG and starts move loop         */
/* ============================================================ */
static void aMNM_actor_init(ACTOR* actor, GAME* game) {
    MONUMENT_ACTOR* monument = (MONUMENT_ACTOR*)actor;

    mFI_SetFG_common(EMPTY_NO, actor->home.position, FALSE);
    aMNM_actor_move(actor, game);
    actor->mv_proc = &aMNM_actor_move;
}

/* ============================================================ */
/* aMNM_actor_move (332B)                                       */
/* Main update — advance animation and rotate windmill blades   */
/* ============================================================ */
static void aMNM_actor_move(ACTOR* actor, GAME* game) {
    MONUMENT_ACTOR* monument = (MONUMENT_ACTOR*)actor;
    GAME_PLAY* play = (GAME_PLAY*)game;

    /* Advance skeleton animation for animated types */
    if (monument->draw_mode == aMNM_DRAW_WITH_ANIME) {
        cKF_SkeletonInfo_R_play(&monument->keyframe);
    }

    /* Update windmill blade rotation */
    if (monument->monument_type == aMNM_TYPE_WINDMILL) {
        monument->blade_angle += 200.0f;
        if (monument->blade_angle >= 65536.0f) {
            monument->blade_angle -= 65536.0f;
        }
    }

    /* Update light position to match actor */
    if (monument->light_node != NULL) {
        monument->monument_light.lights.point.x = (s16)monument->actor_class.world.position.x;
        monument->monument_light.lights.point.y = (s16)(monument->actor_class.world.position.y + 40.0f);
        monument->monument_light.lights.point.z = (s16)monument->actor_class.world.position.z;
    }
}

/* ============================================================ */
/* DwBefore callbacks for animated monuments                    */
/* ============================================================ */

/* aMNM_actor_draw_before_clock (136B)                          */
/* Rotates clock hand joints based on current time              */
static int aMNM_actor_draw_before_clock(GAME* game, cKF_SkeletonInfo_R_c* keyframe, int joint_idx,
                                        Gfx** joint_shape, u8* joint_flags, void* arg,
                                        s_xyz* joint_rot, xyz_t* joint_pos) {
    if (joint_idx == 2) {
        /* Hour hand — rotate based on time */
        joint_rot->z = DEG2SHORT_ANGLE2(90.0f) - Common_Get(time).rad_hour;
    } else if (joint_idx == 3) {
        /* Minute hand — rotate based on time */
        joint_rot->z = DEG2SHORT_ANGLE2(90.0f) - Common_Get(time).rad_min;
    }

    return TRUE;
}

/* aMNM_actor_draw_before_flowerclock (120B)                    */
/* Similar to clock but for the flower clock variant            */
static int aMNM_actor_draw_before_flowerclock(GAME* game, cKF_SkeletonInfo_R_c* keyframe, int joint_idx,
                                              Gfx** joint_shape, u8* joint_flags, void* arg,
                                              s_xyz* joint_rot, xyz_t* joint_pos) {
    if (joint_idx == 2) {
        /* Hour hand */
        joint_rot->z = DEG2SHORT_ANGLE2(90.0f) - Common_Get(time).rad_hour;
    } else if (joint_idx == 3) {
        /* Minute hand */
        joint_rot->z = DEG2SHORT_ANGLE2(90.0f) - Common_Get(time).rad_min;
    }

    return TRUE;
}

/* aMNM_actor_draw_before_windmill (68B)                        */
/* Rotates windmill blade joint based on accumulated angle      */
static int aMNM_actor_draw_before_windmill(GAME* game, cKF_SkeletonInfo_R_c* keyframe, int joint_idx,
                                           Gfx** joint_shape, u8* joint_flags, void* arg,
                                           s_xyz* joint_rot, xyz_t* joint_pos) {
    MONUMENT_ACTOR* monument = (MONUMENT_ACTOR*)arg;

    if (joint_idx == 1) {
        joint_rot->z = (s16)monument->blade_angle;
    }

    return TRUE;
}

/* ============================================================ */
/* aMNM_actor_draw_with_anime (200B)                            */
/* Draw callback for animated monument types                    */
/* ============================================================ */
static void aMNM_actor_draw_with_anime(ACTOR* actor, GAME* game) {
    MONUMENT_ACTOR* monument = (MONUMENT_ACTOR*)actor;
    cKF_SkeletonInfo_R_c* keyframe = &monument->keyframe;
    GRAPH* g = game->graph;
    Mtx* m;
    cKF_draw_callback before_cb;

    m = GRAPH_ALLOC_TYPE(g, Mtx, keyframe->skeleton->num_shown_joints);
    if (m == NULL) {
        return;
    }

    _texture_z_light_fog_prim(g);

    OPEN_DISP(g);
    Matrix_translate(actor->world.position.x, actor->world.position.y, actor->world.position.z, MTX_LOAD);
    Matrix_scale(0.01f, 0.01f, 0.01f, MTX_MULT);
    gSPMatrix(NEXT_POLY_OPA_DISP, _Matrix_to_Mtx_new(g), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    CLOSE_DISP(g);

    /* Select DwBefore callback based on type */
    before_cb = NULL;
    switch (monument->monument_type) {
        case aMNM_TYPE_CLOCK:
            before_cb = &aMNM_actor_draw_before_clock;
            break;
        case aMNM_TYPE_FLOWERCLOCK:
            before_cb = &aMNM_actor_draw_before_flowerclock;
            break;
        case aMNM_TYPE_WINDMILL:
            before_cb = &aMNM_actor_draw_before_windmill;
            break;
    }

    cKF_Si3_draw_R_SV(game, keyframe, m, before_cb, NULL, actor);
}

/* ============================================================ */
/* aMNM_actor_draw_only_model (780B)                            */
/* Draw callback for static monument types (no animation)       */
/* ============================================================ */
static void aMNM_actor_draw_only_model(ACTOR* actor, GAME* game) {
    MONUMENT_ACTOR* monument = (MONUMENT_ACTOR*)actor;
    GAME_PLAY* play = (GAME_PLAY*)game;
    GRAPH* g = game->graph;
    int type = monument->monument_type;
    Gfx* model;

    if (type < 0 || type >= aMNM_TYPE_NUM) {
        return;
    }

    model = aMNM_type_data[type].model;
    if (model == NULL) {
        return;
    }

    _texture_z_light_fog_prim(g);

    Setpos_HiliteReflect_init(&actor->world.position, play);

    OPEN_DISP(g);

    Matrix_push();
    Matrix_translate(actor->world.position.x, actor->world.position.y, actor->world.position.z, MTX_LOAD);
    Matrix_scale(0.01f, 0.01f, 0.01f, MTX_MULT);

    gSPMatrix(NEXT_POLY_OPA_DISP, _Matrix_to_Mtx_new(g), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    gDPPipeSync(NEXT_POLY_OPA_DISP);
    gDPSetPrimColor(NEXT_POLY_OPA_DISP, 0, 128, 255, 255, 255, 255);
    gSPDisplayList(NEXT_POLY_OPA_DISP, model);

    Matrix_pull();

    CLOSE_DISP(g);
}
#endif /* VERSION >= VER_DELUXE */
