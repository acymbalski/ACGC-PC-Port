#include "types.h"
#if VERSION >= VER_DELUXE
#include "ac_design.h"

#include "m_field_info.h"
#include "m_field_make.h"
#include "m_common_data.h"
#include "m_player_lib.h"
#include "m_collision_bg.h"
#include "dolphin/ar.h"
#include "MSL_C/printf.h"

static void Design_Actor_ct(ACTOR* actorx, GAME* game);
static void Design_Actor_dt(ACTOR* actorx, GAME* game);
static void Design_Actor_move(ACTOR* actorx, GAME* game);
static void Design_Actor_draw(ACTOR* actorx, GAME* game);

static DESIGN_ACTOR* design_actor_p;

static f32 l_design_unit_size = 0.0f;

/* clang-format off */
ACTOR_PROFILE Design_Actor_Profile = {
    mAc_PROFILE_MONUMENT,
    ACTOR_PART_BG,
    ACTOR_STATE_NO_DRAW_WHILE_CULLED | ACTOR_STATE_NO_MOVE_WHILE_CULLED,
    EMPTY_NO,
    ACTOR_OBJ_BANK_KEEP,
    sizeof(DESIGN_ACTOR),
    &Design_Actor_ct,
    &Design_Actor_dt,
    &Design_Actor_move,
    &Design_Actor_draw,
    NULL,
};
/* clang-format on */

static int aDS_CheckValidBlock(int bx, int bz) {
    if (bx <= 0 || bz <= 0) {
        return FALSE;
    }

    if (bx - 1 < FG_BLOCK_X_NUM && bz - 1 < FG_BLOCK_Z_NUM) {
        return TRUE;
    }

    if ((bx == 4 || bx == 5) && bz == 8) {
        return TRUE;
    }

    return FALSE;
}

static void Design_Actor_ct(ACTOR* actorx, GAME* game) {
    DESIGN_ACTOR* this = (DESIGN_ACTOR*)actorx;
    int i;

    this->actor_class.gravity = l_design_unit_size;
    this->total_design_count = 0;

    for (i = 0; i < aDS_BLOCK_NUM; i++) {
        this->blocks[i].block_x = -1;
        this->blocks[i].block_z = -1;
        this->blocks[i].data = &this->design_data[i][0];
        this->blocks[i].count = 0;
        this->blocks[i].valid = 1;
        this->aram_buf[i] = ARAlloc(aDS_ARAM_SIZE);
    }

    design_actor_p = this;
}

static void Design_Actor_dt(ACTOR* actorx, GAME* game) {
    DESIGN_ACTOR* this = (DESIGN_ACTOR*)actorx;
    int i;
    u32 len;

    for (i = 0; i < aDS_BLOCK_NUM; i++) {
        if (this->aram_buf[i] != 0) {
            ARFree(&len);
            this->aram_buf[i] = 0;
        }
    }

    design_actor_p = NULL;
}

aDS_block_info_c* aDS_FindDesignBlockForUnit(int ut_x, int ut_z, int* out_local_x, int* out_local_z) {
    int local_x;
    int local_z;
    int bx;
    int bz;
    int i;

    if (design_actor_p == NULL) {
        return NULL;
    }

    local_x = ut_x & (UT_X_NUM - 1);
    bx = ut_x >> 4;
    if (ut_x < 0 && local_x != 0) {
        bx++;
    }
    *out_local_x = local_x;

    local_z = ut_z & (UT_Z_NUM - 1);
    bz = ut_z >> 4;
    if (ut_z < 0 && local_z != 0) {
        bz++;
    }
    *out_local_z = local_z;

    for (i = 0; i < aDS_BLOCK_NUM; i++) {
        if (design_actor_p->blocks[i].block_x == bx &&
            design_actor_p->blocks[i].block_z == bz) {
            return &design_actor_p->blocks[i];
        }
    }

    return NULL;
}

int aDS_place(int ut_x, int ut_z, u8 pattern_idx) {
    int local_x;
    int local_z;
    aDS_block_info_c* block;
    u8* entry;
    xyz_t center_wpos;
    xyz_t temp_wpos;
    int result_x;
    int result_z;
    int block_num;
    int ut_idx;
    u8 height;
    f32 base_y;
    int player_no;

    if (design_actor_p == NULL) {
        return FALSE;
    }

    block = aDS_FindDesignBlockForUnit(ut_x, ut_z, &local_x, &local_z);
    if (block == NULL) {
        return FALSE;
    }

    /* Calculate entry index: (local_z * 16 + local_x) * 8 bytes per entry */
    entry = block->data + (local_x + local_z * UT_X_NUM) * aDS_DESIGN_ENTRY_SIZE;

    /* Check if slot is already occupied */
    if ((s8)entry[0] != -1) {
        return FALSE;
    }

    /* Get center world position for the block */
    mFI_BkandUtNum2CenterWpos(&center_wpos, block->block_x, block->block_z, 0, 0);
    temp_wpos = center_wpos;

    /* Check collision/placement validity */
    base_y = mFI_UtNum2BaseHeight(block->block_x * UT_X_NUM, block->block_z * UT_Z_NUM);
    temp_wpos.y = base_y + mFI_UtNum2BaseHeight(0, 0);

    mFI_Wpos2UtNum_inBlock(&result_x, &result_z, temp_wpos);
    if (result_x != 0 || result_z != 0) {
        printf("aDS_place: invalid position %d %d\n", result_x, result_z);
        return FALSE;
    }

    /* Set design entry data */
    player_no = Common_Get(player_no);
    entry[0] = (u8)player_no;
    entry[1] = pattern_idx;

    /* Calculate height value */
    block_num = mFI_GetBlockNum(block->block_x, block->block_z);
    ut_idx = local_x + local_z * UT_X_NUM;

    {
        mFM_block_info_c* block_info_p;
        f32 base_height_f;

        base_height_f = mFI_UtNum2BaseHeight(block->block_x * UT_X_NUM, block->block_z * UT_Z_NUM);
        block_info_p = mFI_GetBlockTopP();
        height = block_info_p[block_num].bg_info.keep_h[local_z][local_x];
        entry[2] = (u8)((int)(l_design_unit_size * (f32)height + base_height_f) & 0xFF);
        entry[3] = (u8)(((int)(l_design_unit_size * (f32)height + base_height_f) >> 8) & 0xFF);
    }

    entry[4] = 0;
    entry[5] = 0;
    entry[6] = 0;
    entry[7] = 0;

    /* Increment counts */
    design_actor_p->total_design_count++;
    block->count++;

    return TRUE;
}

int aDS_remove(int ut_x, int ut_z) {
    int local_x;
    int local_z;
    aDS_block_info_c* block;
    u8* entry;

    if (design_actor_p == NULL) {
        return FALSE;
    }

    block = aDS_FindDesignBlockForUnit(ut_x, ut_z, &local_x, &local_z);
    if (block == NULL) {
        return FALSE;
    }

    entry = block->data + (local_x + local_z * UT_X_NUM) * aDS_DESIGN_ENTRY_SIZE;

    if ((s8)entry[0] == -1) {
        return FALSE;
    }

    entry[0] = 0xFF;
    entry[1] = 0;

    design_actor_p->total_design_count--;
    block->count--;

    return TRUE;
}

static void aDS_UpdateDataForPlayerBlock(aDS_block_info_c* block) {
    mActor_name_t* fg_items;
    u8* entry;
    int count;
    int i;
    int block_num;
    f32 base_y;
    mFM_block_info_c* block_info_p;
    u8* keep_h_p;

    fg_items = mFI_BkNum2UtFGTop_field(block->block_x, block->block_z);
    count = 0;

    if (fg_items == NULL) {
        block->count = count;
        return;
    }

    entry = block->data;
    base_y = l_design_unit_size + mFI_UtNum2BaseHeight(block->block_x * UT_X_NUM, block->block_z * UT_Z_NUM);
    block_num = mFI_GetBlockNum(block->block_x, block->block_z);
    block_info_p = mFI_GetBlockTopP();
    keep_h_p = &block_info_p[block_num].bg_info.keep_h[0][0];

    for (i = 0; i < aDS_ENTRIES_PER_BLOCK; i++) {
        mActor_name_t item = fg_items[i];

        if (item >= aDS_DESIGN_FG_START && item <= aDS_DESIGN_FG_END) {
            int design_idx;
            int pattern_sub;
            s16 height_val;

            design_idx = (int)(item - aDS_DESIGN_FG_START) >> 3;
            pattern_sub = item & 7;
            entry[0] = (u8)design_idx;
            count++;
            entry[1] = (u8)pattern_sub;
            height_val = (s16)(l_design_unit_size * (f32)keep_h_p[i] + base_y);
            entry[2] = (u8)(height_val & 0xFF);
            entry[3] = (u8)((height_val >> 8) & 0xFF);
            entry[4] = 0;
            entry[5] = 0;
            entry[6] = 0;
        } else {
            entry[0] = 0xFF;
        }

        entry += aDS_DESIGN_ENTRY_SIZE;
    }

    block->valid = 0;
    block->count = count;
}

static void aDS_UpdateBlockOrder(DESIGN_ACTOR* this, GAME* game) {
    PLAYER_ACTOR* player;
    int bx;
    int bz;
    int ut_x;
    int ut_z;
    mActor_name_t* fg_p;
    int i;
    int j;
    int new_bx[aDS_BLOCK_NUM];
    int new_bz[aDS_BLOCK_NUM];

    player = GET_PLAYER_ACTOR_GAME(game);
    fg_p = mFI_Wpos2BkandUtNuminBlock_ref(&bx, &bz, &ut_x, &ut_z, &player->actor_class.world.position);

    if (fg_p == NULL) {
        return;
    }

    if (!aDS_CheckValidBlock(bx, bz)) {
        return;
    }

    /* Compute the neighboring blocks around the player */
    {
        /* The 3 neighboring blocks relative to player position */
        static int neighbor_offsets[][2] = {
            { 0, 0 },
            { -1, 0 },
            { 0, -1 },
        };

        int base_idx;
        int sub_x;
        int sub_z;

        sub_x = (ut_x >= 8) ? 1 : 0;
        sub_z = (ut_z >= 8) ? 2 : 0;
        base_idx = sub_x + sub_z;

        for (i = 0; i < 3; i++) {
            new_bx[i] = bx + neighbor_offsets[base_idx * 3 + i][1];
            new_bz[i] = bz + neighbor_offsets[base_idx * 3 + i][0];
        }
    }

    /* Update block assignments */
    for (i = 0; i < aDS_BLOCK_NUM; i++) {
        aDS_block_info_c* cur_block = &this->blocks[i];

        /* Check if current block already matches a needed block */
        if (cur_block->block_x == new_bx[i] && cur_block->block_z == new_bz[i]) {
            continue;
        }

        /* Search for a matching block in remaining slots */
        {
            int found = FALSE;

            for (j = i + 1; j < aDS_BLOCK_NUM; j++) {
                aDS_block_info_c* other = &this->blocks[j];

                if (other->block_x == new_bx[i] && other->block_z == new_bz[i]) {
                    /* Swap blocks i and j */
                    aDS_block_info_c temp;

                    temp.block_x = cur_block->block_x;
                    temp.block_z = cur_block->block_z;
                    temp.data = cur_block->data;
                    temp.count = cur_block->count;
                    temp.valid = cur_block->valid;

                    cur_block->block_x = other->block_x;
                    cur_block->block_z = other->block_z;
                    cur_block->data = other->data;
                    cur_block->count = other->count;
                    cur_block->valid = other->valid;

                    other->block_x = temp.block_x;
                    other->block_z = temp.block_z;
                    other->data = temp.data;
                    other->count = temp.count;
                    other->valid = temp.valid;

                    found = TRUE;
                    break;
                }
            }

            if (!found) {
                /* Assign this block to the new coordinates */
                cur_block->block_x = new_bx[i];
                cur_block->block_z = new_bz[i];
                cur_block->valid = 1;
            }
        }
    }

    /* Update data for valid blocks and recount total */
    this->total_design_count = 0;
    for (i = 0; i < aDS_BLOCK_NUM; i++) {
        aDS_block_info_c* blk = &this->blocks[i];

        if (blk->valid != 0) {
            if (aDS_CheckValidBlock(blk->block_x, blk->block_z)) {
                aDS_UpdateDataForPlayerBlock(blk);
            }
        }

        this->total_design_count += blk->count;
    }
}

static void Design_Actor_move(ACTOR* actorx, GAME* game) {
    DESIGN_ACTOR* this = (DESIGN_ACTOR*)actorx;

    GET_PLAYER_ACTOR_GAME(game);
    aDS_UpdateBlockOrder(this, game);
}

static void Design_Actor_draw(ACTOR* actorx, GAME* game) {
    /* TODO: Design_Actor_draw implementation (580 bytes) */
    /* This function handles rendering design textures on the ground */
}
#endif /* VERSION >= VER_DELUXE */
