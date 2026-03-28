#ifndef M_EAPPLI_H
#define M_EAPPLI_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void mEA_InitLetterCardE();
extern int mEA_CheckLetterCardE(int card_no);
extern void mEA_SetLetterCardE(int card_no);
extern void mEA_GetCardDLProgram();
extern void mEA_CleanCardDLProgram();
extern u8* mEA_dl_carde_program_p();
extern size_t mEA_dl_carde_program_size();

#if VERSION >= VER_DELUXE
extern u32 mEA_getcrc32(u8* data, int size);
extern u16 mEA_getcrc16(u8* data, int size);
#endif

#ifdef __cplusplus
}
#endif

#endif
