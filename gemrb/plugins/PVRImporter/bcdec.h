/* bcdec.h - v0.96
   provides functions to decompress blocks of BC compressed images
   written by Sergii "iOrange" Kudlai in 2022

   This library does not allocate memory and is trying to use as less stack as possible

   The library was never optimized specifically for speed but for the overall size
   it has zero external dependencies and is not using any runtime functions

   To use just:
   #define BCDEC_IMPLEMENTATION 1
   #include "bcdec.h"

   Supported BC formats:
   BC1 (also known as DXT1) + it's "binary alpha" variant BC1A (DXT1A)
   BC2 (also known as DXT3)
   BC3 (also known as DXT5)
   BC4 (also known as ATI1N)
   BC5 (also known as ATI2N)
   BC6H (HDR format)
   BC7

   BC1/BC2/BC3/BC7 are expected to decompress into 4*4 RGBA blocks 8bit per component (32bit pixel)
   BC4/BC5 are expected to decompress into 4*4 R/RG blocks 8bit per component (8bit and 16bit pixel)
   BC6H is expected to decompress into 4*4 RGB blocks of either 32bit float or 16bit "half" per
   component (96bit or 48bit pixel)

   For more info, issues and suggestions please visit https://github.com/iOrange/bcdec

   CREDITS:
      Aras Pranckevicius (@aras-p)      - BC1/BC3 decoders optimizations (up to 3x the speed)
                                        - BC6H/BC7 bits pulling routines optimizations
                                        - optimized BC6H by moving unquantize out of the loop
                                        - Split BC6H decompression function into 'half' and
                                          'float' variants

   bugfixes:
      @linkmauve

   LICENSE: See end of file for license information.
*/

#ifndef BCDEC_HEADER_INCLUDED
#define BCDEC_HEADER_INCLUDED

/* if BCDEC_STATIC causes problems, try defining BCDECDEF to 'inline' or 'static inline' */
#ifndef BCDECDEF
#ifdef BCDEC_STATIC
#define BCDECDEF    static
#else
#ifdef __cplusplus
#define BCDECDEF    extern "C"
#else
#define BCDECDEF    extern
#endif
#endif
#endif

/*  Used information sources:
    https://docs.microsoft.com/en-us/windows/win32/direct3d10/d3d10-graphics-programming-guide-resources-block-compression
    https://docs.microsoft.com/en-us/windows/win32/direct3d11/bc6h-format
    https://docs.microsoft.com/en-us/windows/win32/direct3d11/bc7-format
    https://docs.microsoft.com/en-us/windows/win32/direct3d11/bc7-format-mode-reference

    ! WARNING ! Khronos's BPTC partitions tables contain mistakes, do not use them!
    https://www.khronos.org/registry/DataFormat/specs/1.1/dataformat.1.1.html#BPTC

    ! Use tables from here instead !
    https://www.khronos.org/registry/OpenGL/extensions/ARB/ARB_texture_compression_bptc.txt

    Leaving it here as it's a nice read
    https://fgiesen.wordpress.com/2021/10/04/gpu-bcn-decoding/

    Fast half to float function from here
    https://gist.github.com/rygorous/2144712
*/

#define BCDEC_BC1_BLOCK_SIZE    8
#define BCDEC_BC3_BLOCK_SIZE    16

#define BCDEC_BC1_COMPRESSED_SIZE(w, h)     ((((w)>>2)*((h)>>2))*BCDEC_BC1_BLOCK_SIZE)
#define BCDEC_BC3_COMPRESSED_SIZE(w, h)     ((((w)>>2)*((h)>>2))*BCDEC_BC3_BLOCK_SIZE)

BCDECDEF void bcdec_bc1(const void* compressedBlock, void* decompressedBlock, int destinationPitch);
BCDECDEF void bcdec_bc3(const void* compressedBlock, void* decompressedBlock, int destinationPitch);


#ifdef BCDEC_IMPLEMENTATION

static void bcdec__color_block(const void* compressedBlock, void* decompressedBlock, int destinationPitch, int onlyOpaqueMode) {
    unsigned short c0, c1;
    unsigned int refColors[4]; /* 0xAABBGGRR */
    unsigned char* dstColors;
    unsigned int colorIndices;
    int i, j, idx;
    unsigned int r0, g0, b0, r1, g1, b1, r, g, b;

    c0 = ((unsigned short*)compressedBlock)[0];
    c1 = ((unsigned short*)compressedBlock)[1];

    /* Expand 565 ref colors to 888 */
    r0 = (((c0 >> 11) & 0x1F) * 527 + 23) >> 6;
    g0 = (((c0 >> 5)  & 0x3F) * 259 + 33) >> 6;
    b0 =  ((c0        & 0x1F) * 527 + 23) >> 6;
    refColors[0] = 0xFF000000 | (b0 << 16) | (g0 << 8) | r0;

    r1 = (((c1 >> 11) & 0x1F) * 527 + 23) >> 6;
    g1 = (((c1 >> 5)  & 0x3F) * 259 + 33) >> 6;
    b1 =  ((c1        & 0x1F) * 527 + 23) >> 6;
    refColors[1] = 0xFF000000 | (b1 << 16) | (g1 << 8) | r1;

    if (c0 > c1 || onlyOpaqueMode) {    /* Standard BC1 mode (also BC3 color block uses ONLY this mode) */
        /* color_2 = 2/3*color_0 + 1/3*color_1
           color_3 = 1/3*color_0 + 2/3*color_1 */
        r = (2 * r0 + r1 + 1) / 3;
        g = (2 * g0 + g1 + 1) / 3;
        b = (2 * b0 + b1 + 1) / 3;
        refColors[2] = 0xFF000000 | (b << 16) | (g << 8) | r;

        r = (r0 + 2 * r1 + 1) / 3;
        g = (g0 + 2 * g1 + 1) / 3;
        b = (b0 + 2 * b1 + 1) / 3;
        refColors[3] = 0xFF000000 | (b << 16) | (g << 8) | r;
    } else {                            /* Quite rare BC1A mode */
        /* color_2 = 1/2*color_0 + 1/2*color_1;
           color_3 = 0;                         */
        r = (r0 + r1 + 1) >> 1;
        g = (g0 + g1 + 1) >> 1;
        b = (b0 + b1 + 1) >> 1;
        refColors[2] = 0xFF000000 | (b << 16) | (g << 8) | r;

        refColors[3] = 0x00000000;
    }

    colorIndices = ((unsigned int*)compressedBlock)[1];

    /* Fill out the decompressed color block */
    dstColors = (unsigned char*)decompressedBlock;
    for (i = 0; i < 4; ++i) {
        for (j = 0; j < 4; ++j) {
            idx = colorIndices & 0x03;
            ((unsigned int*)dstColors)[j] = refColors[idx];
            colorIndices >>= 2;
        }

        dstColors += destinationPitch;
    }
}

static void bcdec__smooth_alpha_block(const void* compressedBlock, void* decompressedBlock, int destinationPitch, int pixelSize) {
    unsigned char* decompressed;
    unsigned char alpha[8];
    int i, j;
    unsigned long long block, indices;

    block = *(unsigned long long*)compressedBlock;
    decompressed = (unsigned char*)decompressedBlock;

    alpha[0] = block & 0xFF;
    alpha[1] = (block >> 8) & 0xFF;

    if (alpha[0] > alpha[1]) {
        /* 6 interpolated alpha values. */
        alpha[2] = (6 * alpha[0] +     alpha[1] + 1) / 7;   /* 6/7*alpha_0 + 1/7*alpha_1 */
        alpha[3] = (5 * alpha[0] + 2 * alpha[1] + 1) / 7;   /* 5/7*alpha_0 + 2/7*alpha_1 */
        alpha[4] = (4 * alpha[0] + 3 * alpha[1] + 1) / 7;   /* 4/7*alpha_0 + 3/7*alpha_1 */
        alpha[5] = (3 * alpha[0] + 4 * alpha[1] + 1) / 7;   /* 3/7*alpha_0 + 4/7*alpha_1 */
        alpha[6] = (2 * alpha[0] + 5 * alpha[1] + 1) / 7;   /* 2/7*alpha_0 + 5/7*alpha_1 */
        alpha[7] = (    alpha[0] + 6 * alpha[1] + 1) / 7;   /* 1/7*alpha_0 + 6/7*alpha_1 */
    }
    else {
        /* 4 interpolated alpha values. */
        alpha[2] = (4 * alpha[0] +     alpha[1] + 1) / 5;   /* 4/5*alpha_0 + 1/5*alpha_1 */
        alpha[3] = (3 * alpha[0] + 2 * alpha[1] + 1) / 5;   /* 3/5*alpha_0 + 2/5*alpha_1 */
        alpha[4] = (2 * alpha[0] + 3 * alpha[1] + 1) / 5;   /* 2/5*alpha_0 + 3/5*alpha_1 */
        alpha[5] = (    alpha[0] + 4 * alpha[1] + 1) / 5;   /* 1/5*alpha_0 + 4/5*alpha_1 */
        alpha[6] = 0x00;
        alpha[7] = 0xFF;
    }

    indices = block >> 16;
    for (i = 0; i < 4; ++i) {
        for (j = 0; j < 4; ++j) {
            decompressed[j * pixelSize] = alpha[indices & 0x07];
            indices >>= 3;
        }

        decompressed += destinationPitch;
    }
}

BCDECDEF void bcdec_bc1(const void* compressedBlock, void* decompressedBlock, int destinationPitch) {
    bcdec__color_block(compressedBlock, decompressedBlock, destinationPitch, 0);
}

BCDECDEF void bcdec_bc3(const void* compressedBlock, void* decompressedBlock, int destinationPitch) {
    bcdec__color_block(((char*)compressedBlock) + 8, decompressedBlock, destinationPitch, 1);
    bcdec__smooth_alpha_block(compressedBlock, ((char*)decompressedBlock) + 3, destinationPitch, 4);
}


#endif /* BCDEC_IMPLEMENTATION */

#endif /* BCDEC_HEADER_INCLUDED */

/* LICENSE:

This software is available under 2 licenses -- choose whichever you prefer.

------------------------------------------------------------------------------
ALTERNATIVE A - MIT License

Copyright (c) 2022 Sergii Kudlai

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

------------------------------------------------------------------------------
ALTERNATIVE B - The Unlicense

This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <https://unlicense.org>

*/
