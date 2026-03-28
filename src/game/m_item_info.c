/**
 * @file m_item_info.c
 * @brief Item information database module (Deluxe).
 *
 * Loads and provides access to the item_info.bin database which contains
 * metadata (category, price, flags, etc.) and names for all items.
 */

#include "types.h"

#if VERSION >= VER_DELUXE

#include "m_item_info.h"
#include "m_malloc.h"
#include "m_name_table.h"
#include "dolphin/dvd.h"

/* Database file path on disc */
static const char mIF_db_path[] = "/item_info.bin";

/* Pointer to loaded database */
static u8* mIF_database;

/* Number of records loaded */
static int mIF_num_records;

/**
 * @brief Loads the item database from DVD.
 *
 * Opens item_info.bin from the disc, allocates memory, and reads the
 * entire file into RAM. 132 bytes compiled.
 */
void mIF_LoadItemDatabase(void) {
    DVDFileInfo fileInfo;

    if (DVDOpen((char*)mIF_db_path, &fileInfo)) {
        u32 size = fileInfo.length;
        u32 aligned_size = ALIGN_NEXT(size, 32);

        mIF_database = (u8*)zelda_malloc_align(aligned_size, 32);
        if (mIF_database != NULL) {
            DVDReadPrio(&fileInfo, mIF_database, (s32)aligned_size, 0, 2);
            mIF_num_records = size / mIF_RECORD_SIZE;
        }
        DVDClose(&fileInfo);
    }
}

/**
 * @brief Initializes the item info module.
 *
 * Clears state and loads the database. 68 bytes compiled.
 */
void mIF_Init(void) {
    mIF_database = NULL;
    mIF_num_records = 0;
    mIF_LoadItemDatabase();
}

/**
 * @brief Converts an item ID to a database index.
 *
 * Maps item IDs from various ranges (furniture, paper, tools, fish,
 * cloth, etc/misc, carpet, wall, food, environment/seeds, minidisks,
 * diary, tickets, insects, lucky bags, turnips) to sequential indices
 * into the item_info.bin database. 224 bytes compiled.
 *
 * @param item Item ID (mActor_name_t)
 * @return Database index, or -1 if the item has no entry
 */
int mIF_Item2Idx(mActor_name_t item) {
    u16 id = (u16)item;

    /* Furniture: 0x1000-0x1FFF, 4 rotational variants per entry */
    if (id >= FTR0_START && id < ITM_PAPER_START) {
        return (id - FTR0_START) >> 2;
    }

    /* Accumulate furniture entries: (0x2000 - 0x1000) / 4 = 1024 */
#define mIF_FTR_COUNT 1024

    /* Paper: 0x2000-0x20FF */
    if (id >= ITM_PAPER_START && id < ITM_PAPER_END) {
        return mIF_FTR_COUNT + (id - ITM_PAPER_START);
    }

#define mIF_PAPER_COUNT 256

    /* Money: 0x2100-0x2103 */
    if (id >= ITM_MONEY_START && id <= ITM_MONEY_END) {
        return mIF_FTR_COUNT + mIF_PAPER_COUNT + (id - ITM_MONEY_START);
    }

#define mIF_MONEY_COUNT 4

    /* Tools: 0x2200-0x2243 */
    if (id >= ITM_TOOL_START && id < ITM_TOOL_END) {
        return mIF_FTR_COUNT + mIF_PAPER_COUNT + mIF_MONEY_COUNT + (id - ITM_TOOL_START);
    }

#define mIF_TOOL_COUNT (ITM_TOOL_END - ITM_TOOL_START)

    /* Fish: 0x2300-0x2327 */
    if (id >= ITM_FISH_START && id < ITM_FISH_END) {
        return mIF_FTR_COUNT + mIF_PAPER_COUNT + mIF_MONEY_COUNT + mIF_TOOL_COUNT + (id - ITM_FISH_START);
    }

#define mIF_FISH_COUNT (ITM_FISH_END - ITM_FISH_START)

    /* Cloth: 0x2400-0x24FF */
    if (id >= ITM_CLOTH_START && id < ITM_CLOTH_END) {
        return mIF_FTR_COUNT + mIF_PAPER_COUNT + mIF_MONEY_COUNT + mIF_TOOL_COUNT + mIF_FISH_COUNT + (id - ITM_CLOTH_START);
    }

#define mIF_CLOTH_COUNT (ITM_CLOTH_END - ITM_CLOTH_START)

    /* Etc: 0x2500-0x2531 */
    if (id >= ITM_ETC_START && id < ITM_ETC_END) {
        return mIF_FTR_COUNT + mIF_PAPER_COUNT + mIF_MONEY_COUNT + mIF_TOOL_COUNT + mIF_FISH_COUNT + mIF_CLOTH_COUNT + (id - ITM_ETC_START);
    }

#define mIF_ETC_COUNT (ITM_ETC_END - ITM_ETC_START)

    /* Carpet: 0x2600-0x2647 */
    if (id >= ITM_CARPET_START && id < ITM_CARPET_END) {
        return mIF_FTR_COUNT + mIF_PAPER_COUNT + mIF_MONEY_COUNT + mIF_TOOL_COUNT + mIF_FISH_COUNT + mIF_CLOTH_COUNT + mIF_ETC_COUNT + (id - ITM_CARPET_START);
    }

#define mIF_CARPET_COUNT (ITM_CARPET_END - ITM_CARPET_START)

    /* Wall: 0x2700-0x2747 */
    if (id >= ITM_WALL_START && id < ITM_WALL_END) {
        return mIF_FTR_COUNT + mIF_PAPER_COUNT + mIF_MONEY_COUNT + mIF_TOOL_COUNT + mIF_FISH_COUNT + mIF_CLOTH_COUNT + mIF_ETC_COUNT + mIF_CARPET_COUNT + (id - ITM_WALL_START);
    }

#define mIF_WALL_COUNT (ITM_WALL_END - ITM_WALL_START)

    /* Food: 0x2800-0x2808 */
    if (id >= ITM_FOOD_START && id < ITM_FOOD_END) {
        return mIF_FTR_COUNT + mIF_PAPER_COUNT + mIF_MONEY_COUNT + mIF_TOOL_COUNT + mIF_FISH_COUNT + mIF_CLOTH_COUNT + mIF_ETC_COUNT + mIF_CARPET_COUNT + mIF_WALL_COUNT + (id - ITM_FOOD_START);
    }

#define mIF_FOOD_COUNT (ITM_FOOD_END - ITM_FOOD_START)

    /* Minidisk: 0x2A00-0x2A37 */
    if (id >= ITM_MINIDISK_START && id < ITM_MINIDISK_END) {
        return mIF_FTR_COUNT + mIF_PAPER_COUNT + mIF_MONEY_COUNT + mIF_TOOL_COUNT + mIF_FISH_COUNT + mIF_CLOTH_COUNT + mIF_ETC_COUNT + mIF_CARPET_COUNT + mIF_WALL_COUNT + mIF_FOOD_COUNT + (id - ITM_MINIDISK_START);
    }

#define mIF_MINIDISK_COUNT (ITM_MINIDISK_END - ITM_MINIDISK_START)

    /* Diary: 0x2B00-0x2B10 */
    if (id >= ITM_DIARY_START && id < ITM_DIARY_END) {
        return mIF_FTR_COUNT + mIF_PAPER_COUNT + mIF_MONEY_COUNT + mIF_TOOL_COUNT + mIF_FISH_COUNT + mIF_CLOTH_COUNT + mIF_ETC_COUNT + mIF_CARPET_COUNT + mIF_WALL_COUNT + mIF_FOOD_COUNT + mIF_MINIDISK_COUNT + (id - ITM_DIARY_START);
    }

#define mIF_DIARY_COUNT (ITM_DIARY_END - ITM_DIARY_START)

    /* Ticket: 0x2C00-0x2C5F */
    if (id >= ITM_TICKET_START && id <= ITM_TICKET_END) {
        return mIF_FTR_COUNT + mIF_PAPER_COUNT + mIF_MONEY_COUNT + mIF_TOOL_COUNT + mIF_FISH_COUNT + mIF_CLOTH_COUNT + mIF_ETC_COUNT + mIF_CARPET_COUNT + mIF_WALL_COUNT + mIF_FOOD_COUNT + mIF_MINIDISK_COUNT + mIF_DIARY_COUNT + (id - ITM_TICKET_START);
    }

#define mIF_TICKET_COUNT (ITM_TICKET_END - ITM_TICKET_START + 1)

    /* Insect: 0x2D00-0x2D2D */
    if (id >= ITM_INSECT_START && id < ITM_INSECT_ALL_END) {
        return mIF_FTR_COUNT + mIF_PAPER_COUNT + mIF_MONEY_COUNT + mIF_TOOL_COUNT + mIF_FISH_COUNT + mIF_CLOTH_COUNT + mIF_ETC_COUNT + mIF_CARPET_COUNT + mIF_WALL_COUNT + mIF_FOOD_COUNT + mIF_MINIDISK_COUNT + mIF_DIARY_COUNT + mIF_TICKET_COUNT + (id - ITM_INSECT_START);
    }

#define mIF_INSECT_COUNT (ITM_INSECT_ALL_END - ITM_INSECT_START)

    /* Lucky bag: 0x2E00-0x2E02 */
    if (id >= ITM_HUKUBUKURO_START && id <= ITM_HUKUBUKURO_END) {
        return mIF_FTR_COUNT + mIF_PAPER_COUNT + mIF_MONEY_COUNT + mIF_TOOL_COUNT + mIF_FISH_COUNT + mIF_CLOTH_COUNT + mIF_ETC_COUNT + mIF_CARPET_COUNT + mIF_WALL_COUNT + mIF_FOOD_COUNT + mIF_MINIDISK_COUNT + mIF_DIARY_COUNT + mIF_TICKET_COUNT + mIF_INSECT_COUNT + (id - ITM_HUKUBUKURO_START);
    }

#define mIF_HUKUBUKURO_COUNT (ITM_HUKUBUKURO_END - ITM_HUKUBUKURO_START + 1)

    /* Turnips (kabu): 0x2F00-0x2F04 */
    if (id >= ITM_KABU_START && id <= ITM_KABU_END) {
        return mIF_FTR_COUNT + mIF_PAPER_COUNT + mIF_MONEY_COUNT + mIF_TOOL_COUNT + mIF_FISH_COUNT + mIF_CLOTH_COUNT + mIF_ETC_COUNT + mIF_CARPET_COUNT + mIF_WALL_COUNT + mIF_FOOD_COUNT + mIF_MINIDISK_COUNT + mIF_DIARY_COUNT + mIF_TICKET_COUNT + mIF_INSECT_COUNT + mIF_HUKUBUKURO_COUNT + (id - ITM_KABU_START);
    }

    return -1;
}

/**
 * @brief Gets item info by database index.
 *
 * Returns a pointer to the raw record data in the database. 48 bytes compiled.
 *
 * @param idx Database index (from mIF_Item2Idx)
 * @return Pointer to the record, or NULL if index is out of range
 */
u8* mIF_GetInfo(int idx) {
    if (mIF_database == NULL || idx < 0 || idx >= mIF_num_records) {
        return NULL;
    }

    return &mIF_database[idx * mIF_RECORD_SIZE];
}

/**
 * @brief Gets item info for a given item ID.
 *
 * Convenience function combining Item2Idx and GetInfo. 36 bytes compiled.
 *
 * @param item Item ID
 * @return Pointer to the record, or NULL if not found
 */
u8* mIF_GetInfoForItem(mActor_name_t item) {
    int idx = mIF_Item2Idx(item);
    return mIF_GetInfo(idx);
}

#endif /* VERSION >= VER_DELUXE */
