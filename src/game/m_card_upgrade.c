#include "types.h"
#if VERSION >= VER_DELUXE
#include "m_common_data.h"

#include "m_card.h"
#include "m_eappli.h"
#include "m_flashrom.h"
#include "m_home.h"
#include "m_malloc.h"
#include "m_name_table.h"
#include "m_npc.h"
#include "m_private.h"
#include "m_string.h"
#include "libultra/libultra.h"

/* Current save version for Deluxe */
#define mCD_SAVE_VERSION 16

/* Forward declarations of static helpers */
static void mCD_UpgradeSaveVersion_v11_v12_convert_food_item(u16* item_p);
static void mCD_UpgradeSaveVersion_v11_v12_convert_food_items(u16* items, int count);
static int mCD_UpgradeSaveVersion_v13_v14_convert_homes(Save_t* save);
static int mCD_UpgradeSaveVersion_v13_v14_convert_cottage(Save_t* save);
static int mCD_UpgradeSaveVersion_v13_v14_convert_private(Save_t* save);

/* Function pointer table type */
typedef int (*UpgradeFunc)(Save_t*);

static int mCD_UpgradeSaveVersion_v5_v6(Save_t* save);
static int mCD_UpgradeSaveVersion_v6_v7(Save_t* save);
static int mCD_UpgradeSaveVersion_v7_v8(Save_t* save);
static int mCD_UpgradeSaveVersion_v8_v9(Save_t* save);
static int mCD_UpgradeSaveVersion_v9_v10(Save_t* save);
static int mCD_UpgradeSaveVersion_v10_v11(Save_t* save);
static int mCD_UpgradeSaveVersion_v11_v12(Save_t* save);
static int mCD_UpgradeSaveVersion_v12_v13(Save_t* save);
static int mCD_UpgradeSaveVersion_v13_v14(Save_t* save);
static int mCD_UpgradeSaveVersion_v14_v15(Save_t* save);
static int mCD_UpgradeSaveVersion_v15_v16(Save_t* save);

static UpgradeFunc upgrade_funcs[] = {
    NULL, /* v0 -> v1 */
    NULL, /* v1 -> v2 */
    NULL, /* v2 -> v3 */
    NULL, /* v3 -> v4 */
    NULL, /* v4 -> v5 */
    mCD_UpgradeSaveVersion_v5_v6,
    mCD_UpgradeSaveVersion_v6_v7,
    mCD_UpgradeSaveVersion_v7_v8,
    mCD_UpgradeSaveVersion_v8_v9,
    mCD_UpgradeSaveVersion_v9_v10,
    mCD_UpgradeSaveVersion_v10_v11,
    mCD_UpgradeSaveVersion_v11_v12,
    mCD_UpgradeSaveVersion_v12_v13,
    mCD_UpgradeSaveVersion_v13_v14,
    mCD_UpgradeSaveVersion_v14_v15,
    mCD_UpgradeSaveVersion_v15_v16,
};

static int mCD_UpgradeSaveVersion_v5_v6(Save_t* save) {
    u8* raw = (u8*)save;

    if (save->save_check.version != 5) {
        return 0;
    }

    bcopy(raw + 0x241a0, raw + 0x0a, 8);
    save->save_check.version = 6;
    return 1;
}

static int mCD_UpgradeSaveVersion_v6_v7(Save_t* save) {
    u8* raw = (u8*)save;
    int i;

    if (save->save_check.version != 6) {
        return 0;
    }

    for (i = 0; i < PLAYER_NUM; i++) {
        mHm_hs_c* home = (mHm_hs_c*)(raw + 0x9CE8 + i * sizeof(mHm_hs_c));
        mHm_CheckVersionAndUpdate(home);
    }

    {
        mHm_cottage_c* cottage = (mHm_cottage_c*)(raw + 0x22958);
        mHm_UpdateCottageVersion_V0_V1(cottage);
    }

    save->save_check.version = 7;
    return 1;
}

static int mCD_UpgradeSaveVersion_v7_v8(Save_t* save) {
    u8* raw = (u8*)save;
    u8* temp;
    u8* priv;
    int i;

    if (save->save_check.version != 7) {
        return 0;
    }

    temp = (u8*)zelda_malloc_align(0x2440, 32);
    priv = raw + 0x20;

    if (temp == NULL) {
        return 0;
    }

    for (i = 0; i < PLAYER_NUM; i++) {
        if (mPr_NullCheckPersonalID((PersonalID_c*)priv) == FALSE) {
            bcopy(priv, temp, 0x1108);
            bcopy(priv + 0x1108, temp + 0x1108, 0xac);
            bzero(temp + 0x11b4, 0xc);
            bcopy(priv + 0x11b4, temp + 0x11c8, 0xc);
            bcopy(priv + 0x11c0, temp + 0x11dc, 0xc);
            bcopy(priv + 0x11cc, temp + 0x11f0, 8);
            bcopy(priv + 0x11d4, temp + 0x11f8, 8);
            bcopy(priv + 0x11dc, temp + 0x1218, 0x50);

            {
                u32 bank = *(u32*)(priv + 0x122c);
                *(u32*)(temp + 0x1238) = bank;
                *(u32*)(temp + 0x123c) = 0;
            }

            bcopy(priv + 0x1240, temp + 0x1240, 0x1200);
            bcopy(temp, priv, 0x2440);
        }

        priv += 0x2440;
    }

    zelda_free(temp);
    save->save_check.version = 8;
    return 1;
}

static int mCD_UpgradeSaveVersion_v8_v9(Save_t* save) {
    u8* raw = (u8*)save;
    u8* dst;
    u8* src;

    dst = raw + 0x246bc;
    src = raw + 0x213a8;

    bcopy(src, dst, 0xd);
    bcopy(src + 0xd, dst + 0x32, 8);
    bcopy(src + 0x2a, dst + 0x96, 0x14);
    bcopy(src + 0x15, dst + 0x64, 0x14);
    bcopy(src + 0x43, dst + 0xaa, 4);
    bcopy(src + 0x3f, dst + 0x78, 4);
    bzero(src, 0x47);

    save->save_check.version = 9;
    return 1;
}

static int mCD_UpgradeSaveVersion_v9_v10(Save_t* save) {
    u8* raw = (u8*)save;
    u8* priv;
    int i;
    u32 zero;

    zero = 0;
    priv = raw + 0x20;

    for (i = 0; i < PLAYER_NUM; i++) {
        if (mPr_NullCheckPersonalID((PersonalID_c*)priv) == FALSE) {
            u8 val;
            u32 trophy;

            val = *(u8*)(priv + 0x14);
            val = (val & ~0x02) | ((val << 1) & 0x02);
            *(u8*)(priv + 0x14) = val;

            *(u32*)(priv + 0x2414) = zero;

            trophy = *(u32*)(priv + 0x23dc);
            if (trophy & 0x10) {
                trophy = (trophy & ~0x10) | 0x80000000;
                *(u32*)(priv + 0x23dc) = trophy;
            }
        }

        priv += 0x2440;
    }

    *(u16*)(raw + 0x13ca0) = 0x5872;

    save->save_check.version = 10;
    return 1;
}

static int mCD_UpgradeSaveVersion_v10_v11(Save_t* save) {
    u8* raw = (u8*)save;
    u8* animal_base;
    u8* island_animal;
    int i;

    animal_base = raw + 0x17438;
    island_animal = raw + 0x23440;

    for (i = 0; i < ANIMAL_NUM_MAX + 1; i++) {
        u8* animal;

        if (i == ANIMAL_NUM_MAX) {
            animal = island_animal;
        } else {
            animal = animal_base + i * sizeof(Animal_c);
        }

        if (mNpc_CheckFreeAnimalPersonalID((AnmPersonalID_c*)animal) == FALSE) {
            bzero(animal + 0x900, 0x1c);
        }
    }

    for (i = 0; i < PLAYER_NUM; i++) {
        u8* dest = raw + i * 0x26b0 + 0xC384;
        mString_Load_StringFromRom(dest, 0xa, 0x213);
    }

    *(u16*)(raw + 0x13ca0) = 0x5872;

    save->save_check.version = 11;
    return 1;
}

static void mCD_UpgradeSaveVersion_v11_v12_convert_food_item(u16* item_p) {
    u16 item = *item_p;

    if (item >= ITM_FOOD_START) {
        u16 item_u = item;

        if (item_u <= (ITM_FOOD_START + 100)) {
            int food_idx = (int)item - ITM_FOOD_START;
            *item_p = mNT_FoodCount2FoodItem(food_idx, 1);
        }
    }
}

static void mCD_UpgradeSaveVersion_v11_v12_convert_food_items(u16* items, int count) {
    int i;

    for (i = 0; i < count; i++) {
        mCD_UpgradeSaveVersion_v11_v12_convert_food_item(items + i);
    }
}

static int mCD_UpgradeSaveVersion_v11_v12(Save_t* save) {
    u8* raw = (u8*)save;
    u8* temp;
    int i;
    int j;

    temp = (u8*)zelda_malloc(0x20);
    if (temp == NULL) {
        return 0;
    }

    bzero(temp, 0x20);

    for (i = 0; i < PLAYER_NUM; i++) {
        u8* priv = raw + 0x20 + i * 0x2440;
        u8* item_src = priv + 0x1238;
        u8* cur;
        int k;

        cur = item_src;
        for (k = 0; k < 8; k++) {
            u16 val;

            val = mEA_getcrc16(cur, 8);
            *(u16*)(temp + k * 4) = val;
            *(u16*)(temp + k * 4 + 2) = *(u16*)(cur + 8);
            cur += 0xa;
        }

        bcopy(temp, item_src, 0x20);
    }

    zelda_free(temp);

    for (i = 0; i < PLAYER_NUM; i++) {
        u8* priv = raw + 0x20 + i * 0x2440;

        mCD_UpgradeSaveVersion_v11_v12_convert_food_items((u16*)(priv + 0x88), 0xf);

        for (j = 0; j < 10; j++) {
            mCD_UpgradeSaveVersion_v11_v12_convert_food_item((u16*)(priv + 0x52c + j * 0x12a));
        }
    }

    for (i = 0; i < PLAYER_NUM; i++) {
        u8* home = raw + i * 0x26b0;
        u8* home_data;

        home_data = home + 0x9CE8;

        mCD_UpgradeSaveVersion_v11_v12_convert_food_items((u16*)(home_data + 0x678), 0x100);

        for (j = 0; j < 3; j++) {
            u8* floor_base = home_data + j * 0x458;
            mCD_UpgradeSaveVersion_v11_v12_convert_food_items((u16*)(floor_base + 0x38), 0x100);
            mCD_UpgradeSaveVersion_v11_v12_convert_food_items((u16*)(floor_base + 0x260), 0x100);
        }

        for (j = 0; j < 10; j++) {
            mCD_UpgradeSaveVersion_v11_v12_convert_food_item((u16*)(home_data + 0x1a5c + j * 0x12a));
        }

        for (j = 0; j < 4; j++) {
            mCD_UpgradeSaveVersion_v11_v12_convert_food_item((u16*)(home_data + 0x25d4 + j * 8));
        }
    }

    {
        u8* shop = raw + 0x22db8;
        mCD_UpgradeSaveVersion_v11_v12_convert_food_items((u16*)shop, 0x220);
    }

    {
        u8* area1 = raw + 0x22960;
        mCD_UpgradeSaveVersion_v11_v12_convert_food_items((u16*)area1, 0x100);
    }

    {
        u8* area2 = raw + 0x22b88;
        mCD_UpgradeSaveVersion_v11_v12_convert_food_items((u16*)area2, 0x100);
    }

    for (i = 0; i < 2; i++) {
        u8* event = raw + 0x22554 + i * 0x200;
        mCD_UpgradeSaveVersion_v11_v12_convert_food_items((u16*)event, 0x100);
    }

    for (i = 0; i < 6; i++) {
        u8* island_area = raw + i * 0xa00;
        for (j = 0; j < 5; j++) {
            u8* sub = island_area + 0x137a8 + j * 0x200;
            mCD_UpgradeSaveVersion_v11_v12_convert_food_items((u16*)sub, 0x100);
        }
    }

    {
        u8* police = raw + 0x20ed0;
        mCD_UpgradeSaveVersion_v11_v12_convert_food_items((u16*)police, 0x14);
    }

    save->save_check.version = 12;
    return 1;
}

static int mCD_UpgradeSaveVersion_v12_v13(Save_t* save) {
    u8* raw = (u8*)save;

    mCD_UpgradeSaveVersion_v11_v12_convert_food_item((u16*)(raw + 0x20688));

    save->save_check.version = 13;
    return 1;
}

static int mCD_UpgradeSaveVersion_v13_v14_convert_homes(Save_t* save) {
    u8* raw = (u8*)save;
    u8* temp;
    int i;

    temp = (u8*)zelda_malloc(0x26b0);
    if (temp == NULL) {
        return 0;
    }

    for (i = 0; i < PLAYER_NUM; i++) {
        u8* home = raw + 0x9CE8 + i * 0x26b0;

        bcopy(home, temp, 0x2698);
        bcopy(home + 0x2698, temp + 0x269c, 0xa);
        bcopy(temp, home, 0x26b0);
    }

    zelda_free(temp);
    return 1;
}

static int mCD_UpgradeSaveVersion_v13_v14_convert_cottage(Save_t* save) {
    u8* raw = (u8*)save;
    u8* temp;
    u8* cottage;

    temp = (u8*)zelda_malloc(0x8c8);
    if (temp == NULL) {
        return 0;
    }

    cottage = raw + 0x22958;

    bcopy(cottage, temp, 0x8a0);
    bcopy(cottage + 0x8b0, temp + 0x8a0, 0xa);
    bcopy(cottage + 0x8bc, temp + 0x8ac, 8);
    bcopy(temp, cottage, 0x8c8);

    zelda_free(temp);
    return 1;
}

static int mCD_UpgradeSaveVersion_v13_v14_convert_private(Save_t* save) {
    u8* raw = (u8*)save;
    u8* temp;
    int i;

    temp = (u8*)zelda_malloc_align(0x2440, 32);
    if (temp == NULL) {
        return 0;
    }

    for (i = 0; i < PLAYER_NUM; i++) {
        u8* priv = raw + 0x20 + i * 0x2440;

        bzero(temp, 0x2440);

        bcopy(priv, temp, 0x1108);
        bcopy(priv + 0x1108, temp + 0x1108, 0xb8);
        bcopy(priv + 0x11c0, temp + 0x11c8, 0xc);
        bcopy(priv + 0x11cc, temp + 0x11dc, 0xc);
        bcopy(priv + 0x11d8, temp + 0x11f0, 8);
        bcopy(priv + 0x11e0, temp + 0x11f8, 8);
        bcopy(priv + 0x1240, temp + 0x1240, 0x1200);
        bcopy(priv + 0x11e8, temp + 0x1218, 0x20);
        bcopy(priv + 0x1238, temp + 0x1238, 0x1208);

        bcopy(temp, priv, 0x2440);
    }

    zelda_free(temp);
    return 1;
}

static int mCD_UpgradeSaveVersion_v13_v14(Save_t* save) {
    if (mCD_UpgradeSaveVersion_v13_v14_convert_homes(save) == FALSE) {
        return 0;
    }

    if (mCD_UpgradeSaveVersion_v13_v14_convert_cottage(save) == FALSE) {
        return 0;
    }

    if (mCD_UpgradeSaveVersion_v13_v14_convert_private(save) == FALSE) {
        return 0;
    }

    save->save_check.version = 14;
    return 1;
}

static int mCD_UpgradeSaveVersion_v14_v15(Save_t* save) {
    u8* raw = (u8*)save;
    u8 temp_buf[0x28];
    u8* npc_tbl;

    bcopy(raw + 0x2134e, temp_buf, 0x28);

    npc_tbl = raw + 0x21322;
    bzero(npc_tbl, 0x5c);

    bcopy(temp_buf, npc_tbl, 0x28);

    save->save_check.version = 15;
    return 1;
}

static int mCD_UpgradeSaveVersion_v15_v16(Save_t* save) {
    u8* raw = (u8*)save;
    int i;
    u8* priv;

    priv = raw + 0x20;
    for (i = 0; i < PLAYER_NUM; i++) {
        u32* flags = (u32*)(priv + 0x23fc);
        *flags = *flags & ~0x00000006;
        priv += 0x2440;
    }

    save->save_check.version = 16;
    return 1;
}

extern void mCD_UpgradeSaveVersion(void) {
    Save_t* save = &Common_Get(save).save;
    u16 land_id;
    int version;

    land_id = save->land_info.id;

    if (mFRm_CheckSaveData_common(&save->save_check, land_id) == FALSE) {
        return;
    }

    version = save->save_check.version;
    while (version != mCD_SAVE_VERSION) {
        if (version < 5) {
            break;
        }

        if (version > mCD_SAVE_VERSION) {
            break;
        }

        if (upgrade_funcs[version] != NULL) {
            if (upgrade_funcs[version](save) == FALSE) {
                break;
            }
        }

        version = save->save_check.version;
    }
}
#endif /* VERSION >= VER_DELUXE */
