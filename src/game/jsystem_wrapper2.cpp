#include "types.h"

#if VERSION >= VER_DELUXE

#include "JSystem/JKernel/JKRFileLoader.h"
#include "JSystem/JKernel/JKREnum.h"
#include "dolphin/dvd.h"

extern "C" {

void* JC_JKRFileLoader_readGlbResource(void* dst, int size, const char* name, int type) {
    return (void*)JKRFileLoader::readGlbResource(dst, (u32)size, name, (JKRExpandSwitch)type);
}

void JC_JKRFileLoader_changeDirectory(const char* path) {
    JKRFileLoader::changeDirectory(path);
}

u32 JC_JKRDvdToMainRam_getSize(const char* path) {
    DVDFileInfo fileInfo;
    s32 entrynum;
    u32 size;

    entrynum = DVDConvertPathToEntrynum((char*)path);
    if (entrynum < 0) {
        return 0;
    }

    if (DVDFastOpen(entrynum, &fileInfo) == FALSE) {
        return 0;
    }

    size = fileInfo.length;
    DVDClose(&fileInfo);

    return size;
}

} /* extern "C" */

#endif /* VERSION >= VER_DELUXE */
