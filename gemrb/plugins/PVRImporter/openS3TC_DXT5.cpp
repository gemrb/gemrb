/////////////////////////////////////////////////////////////////////////////// 
// 
// Copyright (c) 2018 Nikola Bozovic. All rights reserved. 
// 
// This code is licensed under the MIT License (MIT). 
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN 
// THE SOFTWARE. 
// 
///////////////////////////////////////////////////////////////////////////////
//
// openS3TC_DXT5
//
// version  : v2018.09.23
// author   : Nikola Bozovic <nigerija@gmail.com>
// desc     : optimized software DXT5 (BC3) texture block decompression.
// note     : S3TC patent expired on October 2, 2017. 
//            And continuation patent expired on March 16, 2018.
//            S3TC support has landed in Mesa since then.
// changelog:
// * v2018.09.20: initial version.
//
// * v2018.09.23: optimized DXT5 (BC3) to use LUTs.
//
//   test 1920x1080 texture decode time on 3GHz CPU:
//      debug   : ~16.4 ms (vc2017 v141,vc2010 v100)
//        speed in :  ~126.4 mega bytes per sec
//        speed out:  ~505.8 mega bytes per sec
//        speed pix:  ~126.4 mega pixels per sec
//      release :  ~7.9 ms (vc2017 v141) 
//        speed in :  ~262.5 mega bytes per sec
//        speed out: ~1049.9 mega bytes per sec
//        speed pix:  ~262.5 mega pixels per sec
//      release :  ~7.7 ms (vc2010|vc2017 v100) 
//        speed in :  ~269.3 mega bytes per sec
//        speed out: ~1077.2 mega bytes per sec
//        speed pix:  ~269.3 mega pixels per sec
//   yes there is difference if we are using platform toolset
//   vs2010|vs2017 v100 against vs2017 v141, v141 slightly slower.
///////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

namespace openS3TC {

#pragma region "### DXT5 defs and vars ###"

// total bytes in C2(R,G,B) LUT's    :   98304
// total bytes in ALPHA LUT's        : 2097152
// ---------------------------------------------------
// total bytes in luts               : 2195456

// DXT5 (BC3) defines color shifting for texels
DXT5PixelFormat __DXT5_LUT_OutPixelFormat = DXT5PixelFormat_BGRA;

// DXT5 (BC3) defines shifting in precalculated alpha
uint32_t __DXT5_LUT_COLOR_SHIFT_A  = 24;

// DXT5 (BC3) defines shifting in precalculated [R,G,B] components
uint32_t __DXT5_LUT_COLOR_SHIFT_R  = 16;
uint32_t __DXT5_LUT_COLOR_SHIFT_G  = 8;
uint32_t __DXT5_LUT_COLOR_SHIFT_B  = 0;

// DXT5 (BC3) pre calculated values for alpha codes
uint32_t* __DXT5_LUT_COLOR_VALUE_A = nullptr;

// DXT5 (BC3) precalculated [R,G,B] components for all 4 codes's 
uint32_t* __DXT5_LUT_COLOR_VALUE_R = nullptr;
uint32_t* __DXT5_LUT_COLOR_VALUE_G = nullptr;
uint32_t* __DXT5_LUT_COLOR_VALUE_B = nullptr;

#pragma endregion

#pragma region "### DXT5 release internal memory ###"

// DXT5 (BC3) releases internal memory
void DXT5ReleaseLUTs()
{
  if (nullptr != __DXT5_LUT_COLOR_VALUE_A)
  {
    delete[] __DXT5_LUT_COLOR_VALUE_A;
    __DXT5_LUT_COLOR_VALUE_A = nullptr;
  }
  if (nullptr != __DXT5_LUT_COLOR_VALUE_R)
  {
    delete[] __DXT5_LUT_COLOR_VALUE_R;
    __DXT5_LUT_COLOR_VALUE_R = nullptr;
  }
  if (nullptr != __DXT5_LUT_COLOR_VALUE_G)
  {
    delete[] __DXT5_LUT_COLOR_VALUE_G;
    __DXT5_LUT_COLOR_VALUE_B = nullptr;
  }
  if (nullptr != __DXT5_LUT_COLOR_VALUE_B)
  {
    delete[] __DXT5_LUT_COLOR_VALUE_B;
    __DXT5_LUT_COLOR_VALUE_B = nullptr;
  }
}

#pragma endregion

#pragma region "### DXT5 build LUT(s) ###"

// builds static __DXT5_LUT_COLOR_VALUE_A[] look-up table
void __DXT5_LUT_COLOR_VALUE_A_Build()
{
  __DXT5_LUT_COLOR_VALUE_A = new uint32_t[524288]; // 8 * 256 * 256
  // where a[0..7]
  for (int a = 0; a <= 7; a++)
  {
    // where a0[0..255]
    for (int a0 = 0; a0 <= 255; a0++)
    {
      // where a1[0..255]
      for (int a1 = 0; a1 <= 255; a1++)
      {
        // if (a0 > a1) deliberatelly moved inside switch(a) so it can be switch jump optimized execution
        int index = (a0 << 3) | (a1 << 11) | (a); 
        switch (a)
        {
          case 0:
            __DXT5_LUT_COLOR_VALUE_A[index] = (uint32_t)((uint8_t)a0 << __DXT5_LUT_COLOR_SHIFT_A);
            break;
          case 1:
            __DXT5_LUT_COLOR_VALUE_A[index] = (uint32_t)((uint8_t)a1 << __DXT5_LUT_COLOR_SHIFT_A);
            break;
          case 2:
            if (a0 > a1)
              __DXT5_LUT_COLOR_VALUE_A[index] = (uint32_t)((uint8_t)(((6 * a0) + (/**/a1)) / 7) << __DXT5_LUT_COLOR_SHIFT_A);
            else
              __DXT5_LUT_COLOR_VALUE_A[index] = (uint32_t)((uint8_t)(((4 * a0) + (/**/a1)) / 5) << __DXT5_LUT_COLOR_SHIFT_A);
            break;
          case 3:
            if (a0 > a1)
              __DXT5_LUT_COLOR_VALUE_A[index] = (uint32_t)((uint8_t)(((5 * a0) + (2 * a1)) / 7) << __DXT5_LUT_COLOR_SHIFT_A);
            else
              __DXT5_LUT_COLOR_VALUE_A[index] = (uint32_t)((uint8_t)(((3 * a0) + (2 * a1)) / 5) << __DXT5_LUT_COLOR_SHIFT_A);
            break;
          case 4:
            if (a0 > a1)
              __DXT5_LUT_COLOR_VALUE_A[index] = (uint32_t)((uint8_t)(((4 * a0) + (3 * a1)) / 7) << __DXT5_LUT_COLOR_SHIFT_A);
            else
              __DXT5_LUT_COLOR_VALUE_A[index] = (uint32_t)((uint8_t)(((2 * a0) + (3 * a1)) / 5) << __DXT5_LUT_COLOR_SHIFT_A);
            break;
          case 5:
            if (a0 > a1)
              __DXT5_LUT_COLOR_VALUE_A[index] = (uint32_t)((uint8_t)(((3 * a0) + (4 * a1)) / 7)) << __DXT5_LUT_COLOR_SHIFT_A;
            else
              __DXT5_LUT_COLOR_VALUE_A[index] = (uint32_t)((uint8_t)(((/**/a0) + (4 * a1)) / 5)) << __DXT5_LUT_COLOR_SHIFT_A;
            break;
          case 6:
            if (a0 > a1)
              __DXT5_LUT_COLOR_VALUE_A[index] = (uint32_t)((uint8_t)(((2 * a0) + (5 * a1)) / 7)) << __DXT5_LUT_COLOR_SHIFT_A;
            else
              __DXT5_LUT_COLOR_VALUE_A[index] = 0;  // __DXT5_LUT_COLOR_SHIFT_A // no point
            break;
          case 7:
            if (a0 > a1)
              __DXT5_LUT_COLOR_VALUE_A[index] = (uint32_t)(((uint8_t)(((/**/a0) + (6 * a1)) / 7)) << __DXT5_LUT_COLOR_SHIFT_A);
            else
              __DXT5_LUT_COLOR_VALUE_A[index] = (uint32_t)((255) << __DXT5_LUT_COLOR_SHIFT_A);
            break;
        }// switch(a)
      } // a1
    } // a0
  } // a
}

// builds static __DXT5_LUT_COLOR_VALUE_RGB[R,G,B] look-up table's
void __DXT5_LUT_COLOR_VALUE_RGB_Build()
{
  // DXT5 (BC3) pre calculated values for r & b codes
  uint8_t __DXT5_LUT_4x8[32] = // 0x00 - 0x1f (0-31)
    { 
        0,   8,  16,  25,  33,  41,  49,  58, 
       66,  74,  82,  90,  99, 107, 115, 123, 
      132, 140, 148, 156, 164, 173, 181, 189, 
      197, 205, 214, 222, 230, 238, 247, 255 
    };

  // DXT5 (BC3) pre calculated values for g codes
  uint8_t __DXT5_LUT_8x8[64] = // 0x00 - 0x3f (0-63)
    {
        0,   4,   8,  12,  16,  20,  24,  28, 
       32,  36,  40,  45,  49,  53,  57,  61, 
       65,  69,  73,  77,  81,  85,  89,  93, 
       97, 101, 105, 109, 113, 117, 121, 125, 
      130, 134, 138, 142, 146, 150, 154, 158, 
      162, 166, 170, 174, 178, 182, 186, 190, 
      194, 198, 202, 206, 210, 214, 219, 223, 
      227, 231, 235, 239, 243, 247, 251, 255 
    };

  __DXT5_LUT_COLOR_VALUE_R = new uint32_t[4096];  // 4*32*32
  __DXT5_LUT_COLOR_VALUE_G = new uint32_t[16384]; // 4*64*64
  __DXT5_LUT_COLOR_VALUE_B = new uint32_t[4096];  // 4*32*32

  for (int cc0 = 0; cc0 < 32; cc0++)
  { 
    for (int cc1 = 0; cc1 < 32; cc1++)
    {
      int index = ((cc0 << 5) | cc1) << 2;
      __DXT5_LUT_COLOR_VALUE_R[index | 0] = (uint32_t)(((uint32_t)__DXT5_LUT_4x8[cc0]) << __DXT5_LUT_COLOR_SHIFT_R);
      __DXT5_LUT_COLOR_VALUE_B[index | 0] = (uint32_t)(((uint32_t)__DXT5_LUT_4x8[cc0]) << __DXT5_LUT_COLOR_SHIFT_B);
      __DXT5_LUT_COLOR_VALUE_R[index | 1] = (uint32_t)(((uint32_t)__DXT5_LUT_4x8[cc1]) << __DXT5_LUT_COLOR_SHIFT_R);
      __DXT5_LUT_COLOR_VALUE_B[index | 1] = (uint32_t)(((uint32_t)__DXT5_LUT_4x8[cc1]) << __DXT5_LUT_COLOR_SHIFT_B);
      // Each RGB image data block is encoded according to the BC1 formats, 
      // with the exception that the two code bits always use the non-transparent encodings. 
      // In other words, they are treated as though color0 > color1, 
      // regardless of the actual values of color0 and color1.   
      // p2 = ((2*c0)+(c1))/3
      __DXT5_LUT_COLOR_VALUE_R[index | 2] = (uint32_t)((uint32_t)((uint8_t)(((__DXT5_LUT_4x8[cc0] * 2) + (__DXT5_LUT_4x8[cc1])) / 3)) << __DXT5_LUT_COLOR_SHIFT_R);
      __DXT5_LUT_COLOR_VALUE_B[index | 2] = (uint32_t)((uint32_t)((uint8_t)(((__DXT5_LUT_4x8[cc0] * 2) + (__DXT5_LUT_4x8[cc1])) / 3)) << __DXT5_LUT_COLOR_SHIFT_B);
      // p3 = ((c0)+(2*c1))/3
      __DXT5_LUT_COLOR_VALUE_R[index | 3] = (uint32_t)((uint32_t)((uint8_t)(((__DXT5_LUT_4x8[cc0]) + (__DXT5_LUT_4x8[cc1] * 2)) / 3)) << __DXT5_LUT_COLOR_SHIFT_R);
      __DXT5_LUT_COLOR_VALUE_B[index | 3] = (uint32_t)((uint32_t)((uint8_t)(((__DXT5_LUT_4x8[cc0]) + (__DXT5_LUT_4x8[cc1] * 2)) / 3)) << __DXT5_LUT_COLOR_SHIFT_B);
    }
  }
  for (int cc0 = 0; cc0 < 64; cc0++)
  { 
    for (int cc1 = 0; cc1 < 64; cc1++)
    {
      int index = ((cc0 << 6) | cc1) << 2;
      __DXT5_LUT_COLOR_VALUE_G[index | 0] = (uint32_t)(((uint32_t)__DXT5_LUT_8x8[cc0]) << __DXT5_LUT_COLOR_SHIFT_G);
      __DXT5_LUT_COLOR_VALUE_G[index | 1] = (uint32_t)(((uint32_t)__DXT5_LUT_8x8[cc1]) << __DXT5_LUT_COLOR_SHIFT_G);      
      // Each RGB image data block is encoded according to the BC1 formats, 
      // with the exception that the two code bits always use the non-transparent encodings. 
      // In other words, they are treated as though color0 > color1, 
      // regardless of the actual values of color0 and color1.      
      // p2 = ((2*c0)+(c1))/3
      __DXT5_LUT_COLOR_VALUE_G[index | 2] = (uint32_t)((uint32_t)((uint8_t)(((__DXT5_LUT_8x8[cc0] * 2) + (__DXT5_LUT_8x8[cc1])) / 3)) << __DXT5_LUT_COLOR_SHIFT_G);
      // p3 = ((c0)+(2*c1))/3
      __DXT5_LUT_COLOR_VALUE_G[index | 3] = (uint32_t)((uint32_t)((uint8_t)(((__DXT5_LUT_8x8[cc0]) + (__DXT5_LUT_8x8[cc1] * 2)) / 3)) << __DXT5_LUT_COLOR_SHIFT_G);      
    }
  }
}

#pragma endregion

#pragma region "### DXT1 set output pixel format (rebuild LUTs if necessary) ###"

void DXT5SetOutputPixelFormat(DXT5PixelFormat pixelFormat)
{
  bool rebuildLut = (__DXT5_LUT_OutPixelFormat != pixelFormat);
  __DXT5_LUT_OutPixelFormat = pixelFormat;
  switch (__DXT5_LUT_OutPixelFormat)
  {
  case DXT5PixelFormat_ABGR:
    __DXT5_LUT_COLOR_SHIFT_A = 0;
    __DXT5_LUT_COLOR_SHIFT_B = 8;
    __DXT5_LUT_COLOR_SHIFT_G = 16;
    __DXT5_LUT_COLOR_SHIFT_R = 24;
    break;
  case DXT5PixelFormat_ARGB:
    __DXT5_LUT_COLOR_SHIFT_A = 0;
    __DXT5_LUT_COLOR_SHIFT_R = 8;
    __DXT5_LUT_COLOR_SHIFT_G = 16;
    __DXT5_LUT_COLOR_SHIFT_B = 24;
    break;
  case DXT5PixelFormat_RGBA:
    __DXT5_LUT_COLOR_SHIFT_R = 0;
    __DXT5_LUT_COLOR_SHIFT_G = 8;
    __DXT5_LUT_COLOR_SHIFT_B = 16;
    __DXT5_LUT_COLOR_SHIFT_A = 24;
    break;
  case DXT5PixelFormat_BGRA:
  default:
    __DXT5_LUT_COLOR_SHIFT_B = 0;
    __DXT5_LUT_COLOR_SHIFT_G = 8;
    __DXT5_LUT_COLOR_SHIFT_R = 16;
    __DXT5_LUT_COLOR_SHIFT_A = 24;
    break;
  }
  if (rebuildLut || nullptr == __DXT5_LUT_COLOR_VALUE_R)
  {
    __DXT5_LUT_COLOR_VALUE_A_Build();
    __DXT5_LUT_COLOR_VALUE_RGB_Build();
  }
}

#pragma endregion

#pragma region "### DXT5 decompress exported function ###"

/// <summary>
/// Decompresses all the blocks of a DXT1 (BC1) compressed texture and stores the resulting pixels in 'image'.
/// </summary>
/// <param name="width">Texture width.</param>
/// <param name="height">Texture height.</param>
/// <param name="p_input">pointer to compressed DXT1 blocks.</param>
/// <param name="p_output">pointer to the image where the decoded pixels will be stored.</param>
void DXT5Decompress(uint32_t width, uint32_t height, uint8_t* p_input, uint8_t* p_output)
{
  if (nullptr == __DXT5_LUT_COLOR_VALUE_A)
  {
    __DXT5_LUT_COLOR_VALUE_A_Build();
    __DXT5_LUT_COLOR_VALUE_RGB_Build();
  }

  // direct copy paste from c# code, not even comments changed
		
  uint8_t* source = (uint8_t*)p_input;
  uint32_t* target = (uint32_t*)p_output;
  uint32_t target_4scans = (width << 2);
  uint32_t x_block_count = (width + 3) >> 2;
  uint32_t y_block_count = (height + 3) >> 2;
  
  //############################################################
  if ((x_block_count << 2) != width || (y_block_count << 2) != height)
  {
    // for images that do not fit in 4x4 texel bounds
    goto ProcessWithCheckingTexelBounds;
  }
  //############################################################
  //ProcessWithoutCheckingTexelBounds:
  //
  // NOTICE: source and target ARE aligned as 4x4 texels
  //
  // target : advance by 4 scan lines
  for (uint32_t y_block = 0; y_block < y_block_count; y_block++, target+=target_4scans)
  {
    // texel: advance by 4 texels
    uint32_t* texel_x = target;
    for (uint32_t x_block = 0; x_block < x_block_count; x_block++, source+=16, texel_x+=4)
    {
      // read DXT5 (BC3) block data
      //uint8_t ac0 = *(uint8_t*)(source);            // 00    : a0       (8bit)
      //uint8_t ac1 = *(uint8_t*)(source + 1);        // 01    : a1       (8bit)
      uint64_t acfnlut = *(uint64_t*)(source + 2);  // 02-07 : afn LUT  (48bits) 4x4x3bits
      uint16_t cc0 = *(uint16_t*)(source + 8);    // 08-09 : cc0      (16bits)
      uint16_t cc1 = *(uint16_t*)(source + 10);   // 0a-0b : cc1      (16bits)
      uint32_t ccfnlut = *(uint32_t*)(source + 12);   // 0c-0f : ccfn LUT (32bits) 4x4x2bits

      // alpha code and color code [r,g,b] indexes to luts           
      uint32_t ccr = ((uint32_t)((cc0 & 0xf800) >> 4) | (uint32_t)((cc1 & 0xf800) >> 9));
      uint32_t ccg = ((uint32_t)((cc0 & 0x07E0) << 3) | (uint32_t)((cc1 & 0x07E0) >> 3));
      uint32_t ccb = ((uint32_t)((cc0 & 0x001F) << 7) | (uint32_t)((cc1 & 0x001F) << 2));
      //uint32_t ac = ((uint32_t)ac0 << 3) | ((uint32_t)ac1 << 11);
      uint32_t ac = (uint32_t)((*(uint16_t*)source) << 3);

      // process 4x4 color code
      uint32_t* texel = texel_x;
      for (uint32_t by = 0; by < 4; by++, texel += width)
      {
        for (uint32_t bx = 0; bx < 4; bx++, acfnlut>>=3, ccfnlut>>=2)
        {
          uint32_t acfn = (uint32_t)(acfnlut & 0x07);
          uint32_t ccfn = (uint32_t)(ccfnlut & 0x03);

          *(texel + bx) = (uint32_t)
            (
              __DXT5_LUT_COLOR_VALUE_A[ac | acfn] |
              __DXT5_LUT_COLOR_VALUE_R[ccr | ccfn] |
              __DXT5_LUT_COLOR_VALUE_G[ccg | ccfn] |
              __DXT5_LUT_COLOR_VALUE_B[ccb | ccfn]
            );
        }//bx
      }//by
    }//x_block
  }//y_block
	return;
  //
  //############################################################
  // NOTICE: source and target ARE NOT aligned to 4x4 texels, 
  //         We must check for End Of Image (EOI) in this case.
  //############################################################
  // lazy to write boundary separate processings.
  // Just end of image (EOI) pointer check only.
  // considering that I have encountered few images that are not
  // aligned to 4x4 texels, this should be almost never called.
  // takes ~500us (0.5ms) more time processing 2MB pixel images.
  //############################################################
  //
ProcessWithCheckingTexelBounds:
  uint32_t* EOI = target + (width * height);
  // target : advance by 4 scan lines
  for (uint32_t y_block = 0; y_block < y_block_count; y_block++, target += target_4scans)
  {
    uint32_t* texel_x = target;
    // texel: advance by 4 texels
    for (uint32_t x_block = 0; x_block < x_block_count; x_block++, source += 16, texel_x += 4)
    {
      // read DXT5 (BC3) block data
      //uint8_t ac0 = *(uint8_t*)(source);            // 00    : a0       (8bit)
      //uint8_t ac1 = *(uint8_t*)(source+ 1);         // 01    : a1       (8bit)
      uint64_t acfnlut = *(uint64_t*)(source + 2);  // 02-07 : afn LUT  (48bits) 4x4x3bits
      uint16_t cc0 = *(uint16_t*)(source + 8);    // 08-09 : cc0      (16bits)
      uint16_t cc1 = *(uint16_t*)(source + 10);   // 0a-0b : cc1      (16bits)
      uint32_t ccfnlut = *(uint32_t*)(source + 12);   // 0c-0f : ccfn LUT (32bits) 4x4x2bits

      // alpha code and color code [r,g,b] indexes to lut values           
      uint32_t ccr = ((uint32_t)((cc0 & 0xf800) >> 4) | (uint32_t)((cc1 & 0xf800) >> 9));
      uint32_t ccg = ((uint32_t)((cc0 & 0x07E0) << 3) | (uint32_t)((cc1 & 0x07E0) >> 3));
      uint32_t ccb = ((uint32_t)((cc0 & 0x001F) << 7) | (uint32_t)((cc1 & 0x001F) << 2));
      //uint32_t ac = ((uint32_t)ac0 << 3) | ((uint32_t)ac1 << 11);
      uint32_t ac = (uint32_t)((*(uint16_t*)source) << 3);

      // process 4x4 texels
      uint32_t* texel = texel_x;
      for (uint32_t by = 0; by < 4; by++, texel += width)
      {
        //############################################################
        // Check Y Bound (break: no more texels available for block)
        if (texel >= EOI) break;
        //############################################################
        for (uint32_t bx = 0; bx < 4; bx++, acfnlut >>= 3, ccfnlut >>= 2)
        {              
          //############################################################
          // Check X Bound (continue: need ac|ccfnlut to complete shift)
          if (texel + bx >= EOI) continue;
          //############################################################
          uint32_t acfn = (uint32_t)(acfnlut & 0x07);                
          uint32_t ccfn = (uint32_t)(ccfnlut & 0x03);

          *(texel + bx) = (uint32_t)
            (
              __DXT5_LUT_COLOR_VALUE_A[ac | acfn] |
              __DXT5_LUT_COLOR_VALUE_R[ccr | ccfn] |
              __DXT5_LUT_COLOR_VALUE_G[ccg | ccfn] |
              __DXT5_LUT_COLOR_VALUE_B[ccb | ccfn]
            );
        }//bx
      }//by
    }//x_block
  }//y_block
}

} //namespace openS3TC
