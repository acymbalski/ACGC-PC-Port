#ifndef M_ITEM_INFO_H
#define M_ITEM_INFO_H

#include "types.h"
#include "m_actor_type.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Size of each record in item_info.bin */
#define mIF_RECORD_SIZE 36

/* Total number of records in the database */
#define mIF_RECORD_NUM 2704

/* Offset of the name field within each record */
#define mIF_NAME_OFFSET 20

/* Name field length */
#define mIF_NAME_LEN 16

extern void mIF_LoadItemDatabase(void);
extern void mIF_Init(void);
extern int mIF_Item2Idx(mActor_name_t item);
extern u8* mIF_GetInfo(int idx);
extern u8* mIF_GetInfoForItem(mActor_name_t item);

#ifdef __cplusplus
}
#endif

#endif
