#ifndef M_BMG_H
#define M_BMG_H

#include "types.h"

#if VERSION >= VER_DELUXE

#ifdef __cplusplus

#include "JSystem/JGadget/linklist.h"

class JKRAramBlock;

/* BMG file header (32 bytes, "MESGbmg1") */
struct BMGHeader {
    static const char* MAGIC;

    char magic[8];     /* 0x00: "MESGbmg1" */
    u32 fileSize;      /* 0x08 */
    u32 sectionCount;  /* 0x0C */
    u8 padding[16];    /* 0x10 */
};

class BinaryMessage {
public:
    BinaryMessage();
    ~BinaryMessage();

    int open(const char* name);
    void close();
    int loadEntry(u32 index, u8* dst, u32 dstSize);

    u32 getSectionOffset(u32 magic) const;
    void alignedARAMRead(u32 offset, void* dst, u32 size) const;
    void readHeader();
    void readINF1Header();
    void readDAT1Header();

    /* Link list node - must be at offset 0 for TLinkList<BinaryMessage,0> */
    JGadget::TLinkListNode mNode;

    /* 0x08 */ const char* mName;
    /* 0x0C */ u32 mINF1Offset;
    /* 0x10 */ u32 mEntrySize;
    /* 0x14 */ u32 mDAT1Offset;
    /* 0x18 */ u32 mDAT1Size;
    /* 0x1C */ u32 mEntryCount;
    /* 0x20 */ BMGHeader mHeader;
    /* 0x40 */ u32 mAramBaseAddr;
    /* 0x44 */ JKRAramBlock* mAramBlock;
    /* 0x48 */ u8 mIsOpen;
};

extern "C" {
#endif /* __cplusplus */

extern u32 mBMG_open(const char* name);
extern int mBMG_loadEntry(u32 handle, u32 index, u8* dst, u32 dstSize);

#ifdef __cplusplus
}
#endif

#endif /* VERSION >= VER_DELUXE */

#endif /* M_BMG_H */
