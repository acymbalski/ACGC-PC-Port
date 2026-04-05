#include "ac_structure.h"

#include "m_play.h"
#include "m_name_table.h"
#include "m_malloc.h"
#include "m_common_data.h"

static void aSTR_actor_ct(ACTOR* actor, GAME* game);
static void aSTR_actor_dt(ACTOR* actor, GAME* game);
static void aSTR_actor_move(ACTOR* actor, GAME* game);

ACTOR_PROFILE Structure_Profile = { mAc_PROFILE_STRUCTURE,
                                    ACTOR_PART_CONTROL,
                                    ACTOR_STATE_NO_MOVE_WHILE_CULLED | ACTOR_STATE_NO_DRAW_WHILE_CULLED,
                                    EMPTY_NO,
                                    ACTOR_OBJ_BANK_KEEP,
                                    sizeof(STRUCTURE_CONTROL_ACTOR),

                                    &aSTR_actor_ct,
                                    &aSTR_actor_dt,
                                    &aSTR_actor_move,
                                    NONE_ACTOR_PROC,
                                    NULL };

static u8 aSTR_overlay[aSTR_ACTOR_TBL_COUNT][aSTR_OVERLAY_SIZE];
static STRUCTURE_ACTOR aSTR_actor_cl[aSTR_ACTOR_TBL_COUNT];

#include "../src/actor/ac_structure_clip.c_inc"

static void aSTR_actor_ct(ACTOR* actor, GAME* game) {
    STRUCTURE_CONTROL_ACTOR* structure = (STRUCTURE_CONTROL_ACTOR*)actor;

    aSTR_init_clip_area();
    structure->str_door_name = Common_Get(door_data).door_actor_name;
    structure->reset = Common_Get(door_data).exit_type;
#ifdef TARGET_PC
    printf("[STRUCTURE_CTRL] init: door_name=0x%04X next_scene_id=%d exit_type=%d\n",
           (unsigned)Common_Get(door_data).door_actor_name,
           (int)Common_Get(door_data).next_scene_id,
           (int)Common_Get(door_data).exit_type);
#endif
}

static void aSTR_actor_dt(ACTOR* actor, GAME* game) {
    aSTR_free_clip_area();
}

static void aSTR_check_door_data(STRUCTURE_CONTROL_ACTOR* actor, GAME* game) {
    static int request[2] = { 4, 5 };

    GAME_PLAY* play = (GAME_PLAY*)game;

    if (ITEM_NAME_GET_TYPE(actor->str_door_name) == NAME_TYPE_STRUCT) {
        STRUCTURE_ACTOR* str_actor =
            (STRUCTURE_ACTOR*)Actor_info_fgName_search(&play->actor_info, actor->str_door_name, ACTOR_PART_ITEM);

        if (str_actor != NULL && str_actor->request_type == 0) {
#ifdef TARGET_PC
            printf("[STR_DOOR_CHECK] Found STRUCT 0x%04X, setting request_type=%d\n",
                   (unsigned)actor->str_door_name, request[actor->reset == TRUE]);
#endif
            str_actor->request_type = request[actor->reset == TRUE];
            actor->str_door_name = EMPTY_NO;
        }
#ifdef TARGET_PC
        else if (str_actor != NULL) {
            printf("[STR_DOOR_CHECK] Found STRUCT 0x%04X but already has request_type=%d (skipping)\n",
                   (unsigned)actor->str_door_name, str_actor->request_type);
        } else {
            printf("[STR_DOOR_CHECK] STRUCT 0x%04X not found in actor_info!\n",
                   (unsigned)actor->str_door_name);
        }
#endif
    } else {
#ifdef TARGET_PC
        printf("[STR_DOOR_CHECK] str_door_name=0x%04X not a STRUCT (type=%d)\n",
               (unsigned)actor->str_door_name,
               actor->str_door_name == EMPTY_NO ? -1 : (int)ITEM_NAME_GET_TYPE(actor->str_door_name));
#endif
        actor->str_door_name = EMPTY_NO;
    }
}

static void aSTR_actor_move(ACTOR* actor, GAME* game) {
    STRUCTURE_CONTROL_ACTOR* structure = (STRUCTURE_CONTROL_ACTOR*)actor;
    switch (mFI_GetFieldId()) {
        case mFI_FIELD_FG:
            aSTR_check_door_data(structure, game);
            break;
    }
}
