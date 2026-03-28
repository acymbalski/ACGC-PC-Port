#include "types.h"

#if VERSION >= VER_DELUXE

#include "dolphin/gx/GXEnum.h"

/* Bits-per-texel lookup table, indexed by GX texture format */
static u32 l_texel_bits[15];

/**
 * @brief Returns the number of bits per texel for a given GX texture format.
 *
 * @param format GX texture format (GX_TF_* or GX_TF_C*)
 * @return Bits per texel, or 0 for unknown formats
 **/
u32 mTex_GetTexelBits(int format) {
    switch (format) {
    case GX_TF_I4:
    case GX_TF_C4:
    case GX_TF_CMPR:
        return 4;
    case GX_TF_I8:
    case GX_TF_IA4:
    case GX_TF_C8:
        return 8;
    case GX_TF_IA8:
    case GX_TF_RGB565:
    case GX_TF_RGB5A3:
    case GX_TF_C14X2:
        return 16;
    case GX_TF_RGBA8:
        return 32;
    default:
        return 0;
    }
}

/**
 * @brief Calculates the size in bytes of texture data for a given format and dimensions.
 *
 * Rounds up to 32-byte alignment for GX hardware requirements.
 *
 * @param format GX texture format
 * @param width Texture width in texels
 * @param height Texture height in texels
 * @return Size in bytes, 32-byte aligned
 **/
u32 mTex_GetTexDataSize(int format, int width, int height) {
    u32 bits = mTex_GetTexelBits(format);
    u32 size = (width * height * bits) / 8;

    return ALIGN_NEXT(size, 32);
}

/**
 * @brief Initializes the texture utility module.
 *
 * Populates the bits-per-texel lookup table for all supported GX texture formats.
 **/
void mTex_Init(void) {
    l_texel_bits[GX_TF_I4] = 4;
    l_texel_bits[GX_TF_I8] = 8;
    l_texel_bits[GX_TF_IA4] = 8;
    l_texel_bits[GX_TF_IA8] = 16;
    l_texel_bits[GX_TF_RGB565] = 16;
    l_texel_bits[GX_TF_RGB5A3] = 16;
    l_texel_bits[GX_TF_RGBA8] = 32;
    l_texel_bits[GX_TF_C4] = 4;
    l_texel_bits[GX_TF_C8] = 8;
    l_texel_bits[GX_TF_C14X2] = 16;
    l_texel_bits[GX_TF_CMPR] = 4;
}

#endif /* VERSION >= VER_DELUXE */
