#include "m_bmg.h"

#if VERSION >= VER_DELUXE

#include "JSystem/JKernel/JKRAram.h"
#include "JSystem/JKernel/JKRDvdAramRipper.h"
#include "JSystem/JKernel/JKRHeap.h"
#include "dolphin/dvd.h"

#include <string.h>
#include "_mem.h"

/* BMG magic string "MESGbmg1" */
static const char sBMGMagicString[] = "MESGbmg1";
const char* BMGHeader::MAGIC = sBMGMagicString;

/* 128-byte aligned buffer for ARAM reads */
static u8 bmg_read_buf[128] ATTRIBUTE_ALIGN(32);

/* Global list of open BMG files */
static JGadget::TLinkList(BinaryMessage) sBMGList;

/* Forward declarations */
static BinaryMessage* find_bmg_from_name(const char* name);
static BinaryMessage* find_bmg_from_handle(u32 handle);

/* ==================== BinaryMessage class ==================== */

BinaryMessage::BinaryMessage() {
    mNode.pNext_ = NULL;
    mNode.pPrev_ = NULL;
    mName = NULL;
    mINF1Offset = 0;
    mEntrySize = 0;
    mDAT1Offset = 0;
    mDAT1Size = 0;
    mEntryCount = 0;
    mAramBaseAddr = 0;
    mAramBlock = NULL;
    mIsOpen = 0;
}

BinaryMessage::~BinaryMessage() {
    close();
}

int BinaryMessage::open(const char* name) {
    s32 entrynum;

    /* Already open */
    if (mIsOpen != 0) {
        return 0;
    }

    /* Find the file on disc */
    entrynum = DVDConvertPathToEntrynum((char*)name);
    if (entrynum < 0) {
        return 0;
    }

    /* Load file into ARAM */
    mAramBlock = JKRDvdAramRipper::loadToAram(entrynum, 0, EXPAND_SWITCH_DEFAULT, 0, 0);
    if (mAramBlock == NULL) {
        return 0;
    }

    /* Store ARAM address for handle lookup */
    mAramBaseAddr = mAramBlock->getAddress();

    /* Read and validate BMG header */
    readHeader();

    if (strncmp(mHeader.magic, BMGHeader::MAGIC, 8) != 0 || mHeader.sectionCount == 0) {
        close();
        return 0;
    }

    /* Read section headers */
    readINF1Header();
    readDAT1Header();

    /* Validate all required fields */
    if (mINF1Offset == 0 || mDAT1Offset == 0 || mEntrySize == 0 ||
        mDAT1Size == 0 || mEntryCount == 0) {
        close();
        return 0;
    }

    /* Mark as open */
    mName = name;
    mIsOpen = 1;
    return 1;
}

void BinaryMessage::close() {
    if (mAramBlock != NULL) {
        JKRAram::getAramHeap()->free(mAramBlock);
        mAramBlock = NULL;
    }

    mAramBaseAddr = 0;
    mIsOpen = 0;
}

int BinaryMessage::loadEntry(u32 index, u8* dst, u32 dstSize) {
    u32 infData[2];
    u32 msgOffset;
    u32 msgSize;

    /* Validate parameters */
    if (!mIsOpen || dst == NULL || dstSize == 0 || index >= mEntryCount) {
        return -1;
    }

    /* Read the INF1 entry for this message (8 bytes: offset pair) */
    alignedARAMRead(mINF1Offset + index * mEntrySize + 0x10, infData, 8);

    /* Compute message size */
    msgOffset = infData[0];
    if (index == mEntryCount - 1) {
        /* Last entry: size extends to end of DAT1 section */
        msgSize = mDAT1Size - msgOffset;
    } else {
        /* Size is difference between consecutive offsets */
        msgSize = infData[1] - msgOffset;
    }

    /* Clamp to destination buffer size */
    if (dstSize < msgSize) {
        msgSize = dstSize;
    }

    /* Read the message data from DAT1 section (skip 8-byte DAT1 header) */
    alignedARAMRead(mDAT1Offset + msgOffset + 8, dst, msgSize);

    return (int)msgSize;
}

u32 BinaryMessage::getSectionOffset(u32 magic) const {
    u32 numSections;
    u32 offset;
    u32 i;
    u32 sectionData[2];

    numSections = mHeader.sectionCount & 0xFFFF;
    offset = 0x20; /* Start after the 32-byte BMG file header */

    for (i = 0; i < numSections; i++) {
        /* Read section magic and size (8 bytes) */
        alignedARAMRead(offset, sectionData, 8);

        if (sectionData[0] == magic) {
            return offset;
        }

        /* Advance by section size */
        offset += sectionData[1];
    }

    return 0;
}

void BinaryMessage::alignedARAMRead(u32 offset, void* dst, u32 size) const {
    u32 alignedStart;
    u32 alignedEnd;
    u32 alignedSize;

    /* Align read boundaries to 32 bytes for ARAM DMA */
    alignedStart = offset & ~0x1F;
    alignedEnd = (offset + size + 0x1F) & ~0x1F;
    alignedSize = alignedEnd - alignedStart;

    /* Read aligned data from ARAM block into temp buffer */
    JKRAram::aramToMainRam(mAramBlock, bmg_read_buf, alignedSize, alignedStart,
                           EXPAND_SWITCH_DEFAULT, 0, NULL, -1, NULL);

    /* Copy the requested portion to the destination */
    memcpy(dst, bmg_read_buf + (offset - alignedStart), size);
}

void BinaryMessage::readHeader() {
    alignedARAMRead(0, &mHeader, sizeof(BMGHeader));
}

void BinaryMessage::readINF1Header() {
    u8 inf1Buf[16];

    /* Find INF1 section and read its header */
    mINF1Offset = getSectionOffset(0x494E4631); /* "INF1" */
    alignedARAMRead(mINF1Offset, inf1Buf, 0x10);

    /* Extract entry size and count from INF1 header */
    mEntrySize = *(u16*)(inf1Buf + 0x0A);
    mEntryCount = *(u16*)(inf1Buf + 0x08);
}

void BinaryMessage::readDAT1Header() {
    u8 dat1Buf[8];

    /* Find DAT1 section and read its header */
    mDAT1Offset = getSectionOffset(0x44415431); /* "DAT1" */
    alignedARAMRead(mDAT1Offset, dat1Buf, 8);

    /* Extract section data size */
    mDAT1Size = *(u32*)(dat1Buf + 4);
}

/* ==================== Free functions ==================== */

static BinaryMessage* find_bmg_from_name(const char* name) {
    JGadget::TLinkList(BinaryMessage)::iterator it = sBMGList.begin();

    for (; it != sBMGList.end(); ++it) {
        if (strncmp(it->mName, name, strlen(name)) == 0) {
            return &(*it);
        }
    }

    return NULL;
}

static BinaryMessage* find_bmg_from_handle(u32 handle) {
    JGadget::TLinkList(BinaryMessage)::iterator it = sBMGList.begin();

    for (; it != sBMGList.end(); ++it) {
        if (it->mAramBaseAddr == handle) {
            return &(*it);
        }
    }

    return NULL;
}

/* ==================== C API ==================== */

extern "C" u32 mBMG_open(const char* name) {
    BinaryMessage* bmg;

    /* Check if already loaded */
    bmg = find_bmg_from_name(name);
    if (bmg != NULL) {
        return bmg->mAramBaseAddr;
    }

    /* Allocate and open new BMG */
    bmg = new BinaryMessage();
    if (bmg == NULL) {
        return 0;
    }

    if (!bmg->open(name)) {
        delete bmg;
        return 0;
    }

    /* Add to global list */
    sBMGList.Push_back(bmg);

    return bmg->mAramBaseAddr;
}

/* @unused */
extern "C" void mBMG_close(u32 handle) {
    BinaryMessage* bmg;

    bmg = find_bmg_from_handle(handle);
    if (bmg != NULL) {
        sBMGList.Remove(bmg);
        delete bmg;
    }
}

extern "C" int mBMG_loadEntry(u32 handle, u32 index, u8* dst, u32 dstSize) {
    BinaryMessage* bmg;

    bmg = find_bmg_from_handle(handle);
    if (bmg != NULL) {
        return bmg->loadEntry(index, dst, dstSize);
    }

    return 0;
}

#endif /* VERSION >= VER_DELUXE */
