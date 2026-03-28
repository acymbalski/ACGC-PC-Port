#ifndef AC_DESIGN_H
#define AC_DESIGN_H

#include "types.h"
#include "m_actor.h"

#ifdef __cplusplus
extern "C" {
#endif

#define aDS_BLOCK_NUM 4
#define aDS_DATA_SIZE 0x800
#define aDS_ARAM_SIZE 0x4000
#define aDS_DESIGN_ENTRY_SIZE 8
#define aDS_ENTRIES_PER_BLOCK 256

#define aDS_DESIGN_FG_START 0xB000
#define aDS_DESIGN_FG_END 0xB01F

typedef struct design_block_info_s {
    /* 0x00 */ int block_x;
    /* 0x04 */ int block_z;
    /* 0x08 */ u8* data;
    /* 0x0C */ s16 count;
    /* 0x0E */ u16 valid;
} aDS_block_info_c;

typedef struct design_actor_s DESIGN_ACTOR;

struct design_actor_s {
    /* 0x0000 */ ACTOR actor_class;
    /* 0x0174 */ int total_design_count;
    /* 0x0178 */ aDS_block_info_c blocks[aDS_BLOCK_NUM];
    /* 0x01B8 */ u8 design_data[aDS_BLOCK_NUM][aDS_DATA_SIZE];
    /* 0x21B8 */ u32 aram_buf[aDS_BLOCK_NUM];
};

extern ACTOR_PROFILE Design_Actor_Profile;

extern aDS_block_info_c* aDS_FindDesignBlockForUnit(int ut_x, int ut_z, int* out_local_x, int* out_local_z);
extern int aDS_place(int ut_x, int ut_z, u8 pattern_idx);
extern int aDS_remove(int ut_x, int ut_z);

#ifdef __cplusplus
}
#endif

#endif
