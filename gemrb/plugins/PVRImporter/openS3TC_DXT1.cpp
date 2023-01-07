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
// openS3TC_DXT1
//
// version  : v2018.09.23
// author   : Nikola Bozovic <nigerija@gmail.com>
// desc     : LUT optimized software DXT1 (BC1) texture block decompression.
// note     : S3TC patent expired on October 2, 2017. 
//            And continuation patent expired on March 16, 2018.
//            S3TC support has landed in Mesa since then.
// changelog:
// * v2018.09.20: initial version.
//
// * v2018.09.23: optimized DXT1 (BC1) to use LUTs.
//
//   test 1920x1080 texture decode time on 3GHz CPU:
//      debug   : ~11.1 ms (vc2017 v141,vc2010 v100)
//        speed in :  ~124.5 mega bytes per sec
//        speed out:  ~747.2 mega bytes per sec
//        speed pix:  ~186.8 mega pixels per sec
//      release :  ~5.8 ms (vc2017 v141) 
//        speed in :  ~238.3 mega bytes per sec
//        speed out: ~1430.8 mega bytes per sec
//        speed pix:  ~357.5 mega pixels per sec
//      release :  ~5.2 ms (vc2010|vc2017 v100) 
//        speed in :  ~265.9 mega bytes per sec
//        speed out: ~1595.0 mega bytes per sec
//        speed pix:  ~398.8 mega pixels per sec
//   yes there is difference if we are using platform toolset
//   vs2010|vs2017 v100 against vs2017 v141, v141 is ~11% slower.
///////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

namespace openS3TC {

#pragma region "### DXT1 const, vars, decl, forw defs ###"

// total bytes in COLOR(A,R,G,B) static LUT's :   196608

//forward declarations
void __DXT1_LUT_Build();

DXT1PixelFormat __DXT1_LUT_OutPixelFormat = DXT1PixelFormat_BGRA;

// DXT1 (BC1) defines shifting in precalculated alpha
uint32_t __DXT1_LUT_COLOR_SHIFT_A  = 24;
// DXT1 (BC1) defines shifting in precalculated [R,G,B] components
uint32_t __DXT1_LUT_COLOR_SHIFT_R  = 16;
uint32_t __DXT1_LUT_COLOR_SHIFT_G  = 8;
uint32_t __DXT1_LUT_COLOR_SHIFT_B  = 0;

// DXT1 (BC1) pre calculated values for alpha codes
uint32_t __DXT1_LUT_COLOR_VALUE_A = (uint32_t)(0xff << __DXT1_LUT_COLOR_SHIFT_A);

// DXT1 (BC1) precalculated [R,G,B] components for all 4 codes's 
uint32_t* __DXT1_LUT_COLOR_VALUE_R = nullptr;
uint32_t* __DXT1_LUT_COLOR_VALUE_G = nullptr;
uint32_t* __DXT1_LUT_COLOR_VALUE_B = nullptr;

#pragma endregion

// DXT1 (BC1) releases internal memory
void DXT1ReleaseLUTs()
{
  if (nullptr != __DXT1_LUT_COLOR_VALUE_R)
  {
    delete[] __DXT1_LUT_COLOR_VALUE_R;
    __DXT1_LUT_COLOR_VALUE_R = nullptr;
  }
  if (nullptr != __DXT1_LUT_COLOR_VALUE_G)
  {
    delete[] __DXT1_LUT_COLOR_VALUE_G;
    __DXT1_LUT_COLOR_VALUE_G = nullptr;
  }
  if (nullptr != __DXT1_LUT_COLOR_VALUE_B)
  {
    delete[] __DXT1_LUT_COLOR_VALUE_B;
    __DXT1_LUT_COLOR_VALUE_B = nullptr;
  }
}

#pragma region "### DXT1 set output pixel format (rebuild LUTs if necessary) ###"

void DXT1SetOutputPixelFormat(DXT1PixelFormat pixelFormat)
{
  bool rebuildLut = (__DXT1_LUT_OutPixelFormat != pixelFormat);
  __DXT1_LUT_OutPixelFormat = pixelFormat;
  switch (__DXT1_LUT_OutPixelFormat)
  {
  case DXT1PixelFormat_ABGR:
    __DXT1_LUT_COLOR_SHIFT_A = 0;
    __DXT1_LUT_COLOR_SHIFT_B = 8;
    __DXT1_LUT_COLOR_SHIFT_G = 16;
    __DXT1_LUT_COLOR_SHIFT_R = 24;
    break;
  case DXT1PixelFormat_ARGB:
    __DXT1_LUT_COLOR_SHIFT_A = 0;
    __DXT1_LUT_COLOR_SHIFT_R = 8;
    __DXT1_LUT_COLOR_SHIFT_G = 16;
    __DXT1_LUT_COLOR_SHIFT_B = 24;
    break;
  case DXT1PixelFormat_RGBA:
    __DXT1_LUT_COLOR_SHIFT_R = 0;
    __DXT1_LUT_COLOR_SHIFT_G = 8;
    __DXT1_LUT_COLOR_SHIFT_B = 16;
    __DXT1_LUT_COLOR_SHIFT_A = 24;
    break;
  case DXT1PixelFormat_BGRA:
  default:
    __DXT1_LUT_COLOR_SHIFT_B = 0;
    __DXT1_LUT_COLOR_SHIFT_G = 8;
    __DXT1_LUT_COLOR_SHIFT_R = 16;
    __DXT1_LUT_COLOR_SHIFT_A = 24;
    break;
  }
  if (rebuildLut || nullptr == __DXT1_LUT_COLOR_VALUE_R)
  {
    __DXT1_LUT_Build();
  }
}

#pragma endregion

#pragma region "### DXT1 build LUTs ###"

// builds static Build_LUT_DXT1_COLOR_[R,G,B] look-up table's
void __DXT1_LUT_Build()
{
  // DXT1 (BC1) pre calculated values for r & b codes
    uint8_t __LUT_DXT1_4x8[32] = // 0x00 - 0x1f (0-31)
    { 
        0,   8,  16,  25,  33,  41,  49,  58, 
       66,  74,  82,  90,  99, 107, 115, 123, 
      132, 140, 148, 156, 164, 173, 181, 189, 
      197, 205, 214, 222, 230, 238, 247, 255 
    };

  // DXT1 (BC1) pre calculated values for g codes
    uint8_t __LUT_DXT1_8x8[64] = // 0x00 - 0x3f (0-63)
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
  
  __DXT1_LUT_COLOR_VALUE_A = (uint32_t)(0xff << __DXT1_LUT_COLOR_SHIFT_A);
  if (nullptr == __DXT1_LUT_COLOR_VALUE_R)
  {
    __DXT1_LUT_COLOR_VALUE_R = new uint32_t[8192];  // 4*2*32*32 (4:code)*(2:alpha)*(32:c0(r)matrix4x8)*(32:c1(r)matrix4x8)
    __DXT1_LUT_COLOR_VALUE_G = new uint32_t[32768]; // 4*2*64*64 (4:code)*(2:alpha)*(64:c0(g)matrix8x8)*(64:c1(g)matrix8x8)
    __DXT1_LUT_COLOR_VALUE_B = new uint32_t[8192];  // 4*2*32*32 (4:code)*(2:alpha)*(32:c0(b)matrix4x8)*(32:c1(b)matrix4x8)
  }

  for (int ac = 0; ac <= 1; ac++)
  {
    for (int cc0 = 0; cc0 < 32; cc0++)
    {
      for (int cc1 = 0; cc1 < 32; cc1++)
      {
        int index = ((cc0 << 6) | (cc1 << 1) | (ac)) << 2; // 2 bits for ccfn
        __DXT1_LUT_COLOR_VALUE_R[index | 0] = __DXT1_LUT_COLOR_VALUE_A | (uint32_t)(((uint32_t)__LUT_DXT1_4x8[cc0]) << __DXT1_LUT_COLOR_SHIFT_R);
        __DXT1_LUT_COLOR_VALUE_B[index | 0] = __DXT1_LUT_COLOR_VALUE_A | (uint32_t)(((uint32_t)__LUT_DXT1_4x8[cc0]) << __DXT1_LUT_COLOR_SHIFT_B);
        __DXT1_LUT_COLOR_VALUE_R[index | 1] = __DXT1_LUT_COLOR_VALUE_A | (uint32_t)(((uint32_t)__LUT_DXT1_4x8[cc1]) << __DXT1_LUT_COLOR_SHIFT_R);
        __DXT1_LUT_COLOR_VALUE_B[index | 1] = __DXT1_LUT_COLOR_VALUE_A | (uint32_t)(((uint32_t)__LUT_DXT1_4x8[cc1]) << __DXT1_LUT_COLOR_SHIFT_B);
        if (cc0 > cc1)
        {
          // p2 = ((2*c0)+(c1))/3
          __DXT1_LUT_COLOR_VALUE_R[index | 2] = __DXT1_LUT_COLOR_VALUE_A | (uint32_t)((uint32_t)((uint8_t)(((__LUT_DXT1_4x8[cc0] * 2) + (__LUT_DXT1_4x8[cc1])) / 3)) << __DXT1_LUT_COLOR_SHIFT_R);
          __DXT1_LUT_COLOR_VALUE_B[index | 2] = __DXT1_LUT_COLOR_VALUE_A | (uint32_t)((uint32_t)((uint8_t)(((__LUT_DXT1_4x8[cc0] * 2) + (__LUT_DXT1_4x8[cc1])) / 3)) << __DXT1_LUT_COLOR_SHIFT_B);
          // p3 = ((c0)+(2*c1))/3              
          __DXT1_LUT_COLOR_VALUE_R[index | 3] = __DXT1_LUT_COLOR_VALUE_A | (uint32_t)((uint32_t)((uint8_t)(((__LUT_DXT1_4x8[cc0]) + (__LUT_DXT1_4x8[cc1] * 2)) / 3)) << __DXT1_LUT_COLOR_SHIFT_R);
          __DXT1_LUT_COLOR_VALUE_B[index | 3] = __DXT1_LUT_COLOR_VALUE_A | (uint32_t)((uint32_t)((uint8_t)(((__LUT_DXT1_4x8[cc0]) + (__LUT_DXT1_4x8[cc1] * 2)) / 3)) << __DXT1_LUT_COLOR_SHIFT_B);
        }
        else // c0 <= c1
        {
          // p2 = (c0/2)+(c1/2)
          __DXT1_LUT_COLOR_VALUE_R[index | 2] = __DXT1_LUT_COLOR_VALUE_A | (uint32_t)((uint32_t)((uint8_t)(((__LUT_DXT1_4x8[cc0] / 2) + (__LUT_DXT1_4x8[cc1] / 2)))) << __DXT1_LUT_COLOR_SHIFT_R);
          __DXT1_LUT_COLOR_VALUE_B[index | 2] = __DXT1_LUT_COLOR_VALUE_A | (uint32_t)((uint32_t)((uint8_t)(((__LUT_DXT1_4x8[cc0] / 2) + (__LUT_DXT1_4x8[cc1] / 2)))) << __DXT1_LUT_COLOR_SHIFT_B);
          if (ac == 0)
          {
            // p3 = (color0 + 2*color1) / 3
            __DXT1_LUT_COLOR_VALUE_R[index | 3] = __DXT1_LUT_COLOR_VALUE_A | (uint32_t)((uint32_t)((uint8_t)(((__LUT_DXT1_4x8[cc0]) + (__LUT_DXT1_4x8[cc1] * 2)) / 3)) << __DXT1_LUT_COLOR_SHIFT_R);
            __DXT1_LUT_COLOR_VALUE_B[index | 3] = __DXT1_LUT_COLOR_VALUE_A | (uint32_t)((uint32_t)((uint8_t)(((__LUT_DXT1_4x8[cc0]) + (__LUT_DXT1_4x8[cc1] * 2)) / 3)) << __DXT1_LUT_COLOR_SHIFT_B);
          }
          else // tr == 1
          {
            // p3 == 0
            __DXT1_LUT_COLOR_VALUE_R[index | 3] = 0; // transparent black
            __DXT1_LUT_COLOR_VALUE_B[index | 3] = 0; // transparent black
          }
        }
      }//cc1
    }//cc0
  }//ac
  for (int ac = 0; ac <= 1; ac++)
  {
    for (int cc0 = 0; cc0 < 64; cc0++)
    {
      for (int cc1 = 0; cc1 < 64; cc1++)
      {
        int index = ((cc0 << 7) | (cc1 << 1) | (ac)) << 2; // 2 bits for ccfn
        __DXT1_LUT_COLOR_VALUE_G[index | 0] = __DXT1_LUT_COLOR_VALUE_A | (uint32_t)(((uint32_t)__LUT_DXT1_8x8[cc0]) << __DXT1_LUT_COLOR_SHIFT_G);
        __DXT1_LUT_COLOR_VALUE_G[index | 1] = __DXT1_LUT_COLOR_VALUE_A | (uint32_t)(((uint32_t)__LUT_DXT1_8x8[cc1]) << __DXT1_LUT_COLOR_SHIFT_G);
        if (cc0 > cc1)
        {
          // p2 = ((2*c0)+(c1))/3
          __DXT1_LUT_COLOR_VALUE_G[index | 2] = __DXT1_LUT_COLOR_VALUE_A | (uint32_t)((uint32_t)((uint8_t)(((__LUT_DXT1_8x8[cc0] * 2) + (__LUT_DXT1_8x8[cc1])) / 3)) << __DXT1_LUT_COLOR_SHIFT_G);
          // p3 = ((c0)+(2*c1))/3
          __DXT1_LUT_COLOR_VALUE_G[index | 3] = __DXT1_LUT_COLOR_VALUE_A | (uint32_t)((uint32_t)((uint8_t)(((__LUT_DXT1_8x8[cc0]) + (__LUT_DXT1_8x8[cc1] * 2)) / 3)) << __DXT1_LUT_COLOR_SHIFT_G);
        }
        else // c0 <= c1
        {
          // p2 = (c0/2)+(c1/2)
          __DXT1_LUT_COLOR_VALUE_G[index | 2] = __DXT1_LUT_COLOR_VALUE_A | (uint32_t)((uint32_t)((uint8_t)(((__LUT_DXT1_8x8[cc0] / 2) + (__LUT_DXT1_8x8[cc1]) / 2))) << __DXT1_LUT_COLOR_SHIFT_G);
          if (ac == 0)
          {
            // p3 = (color0 + 2*color1) / 3
            __DXT1_LUT_COLOR_VALUE_G[index | 3] = __DXT1_LUT_COLOR_VALUE_A | (uint32_t)((uint32_t)((uint8_t)(((__LUT_DXT1_8x8[cc0] / 2) + (__LUT_DXT1_8x8[cc1]) / 2))) << __DXT1_LUT_COLOR_SHIFT_G);
          }
          else
          {
            // p3 == 0
            __DXT1_LUT_COLOR_VALUE_G[index | 3] = 0; // transparent black
          }
        }
      }//cc1
    }//cc0
  }//ac
}
#pragma endregion

#pragma region "### DXT1 decompress ###"

/// <summary>
/// Decompresses all the blocks of a DXT1 (BC1) compressed texture and stores the resulting pixels in 'image'.
/// </summary>
/// <param name="width">Texture width.</param>
/// <param name="height">Texture height.</param>
/// <param name="p_input">pointer to compressed DXT1 blocks.</param>
/// <param name="p_output">pointer to the image where the decompressed pixels will be stored.</param>
void DXT1Decompress(uint32_t width, uint32_t height, uint8_t* p_input, uint8_t* p_output)
{
  if (nullptr == __DXT1_LUT_COLOR_VALUE_R)
  {
    __DXT1_LUT_Build();
  }

  // direct copy paste from c# code, not even comments changed

  uint8_t * source = (uint8_t *)p_input; // block size: 64bit
  uint32_t* target = (uint32_t*)p_output;
  uint32_t target_4scans = (width << 2);
  uint32_t x_block_count = (width + 3) / 4;
  uint32_t y_block_count = (height + 3) / 4;
  
  if (x_block_count * 4 != width || y_block_count * 4 != height)
  {
    // for images that do not fit in 4x4 texel bounds
    goto ProcessWithCheckingTexelBounds;
  }
  
  // NOTICE: source and target ARE aligned as 4x4 texels

  // target : advance by 4 scan lines
  for (uint32_t y_block = 0; y_block < y_block_count; y_block++, target+=target_4scans)
  {
    // texel: advance by 4 texels
    uint32_t* texel_x = target;
    for (uint32_t x_block = 0; x_block < x_block_count; x_block++, source+=8, texel_x+=4)
    {
      // read DXT1 (BC1) block data
      uint16_t cc0 = *(uint16_t*)(source);      // 00-01 : cc0       (16bits)
      uint16_t cc1 = *(uint16_t*)(source + 2);  // 02-03 : cc1       (16bits)
      uint32_t ccfnlut = *(uint32_t*)(source + 4);  // 04-07 : ccfn LUT  (32bits) 4x4x2bits
      uint32_t ac = (uint32_t)(cc0 > cc1 ? 0 : 4);

      // color code [r,g,b] indexes to luts 
      uint32_t ccr = ((uint32_t)((cc0 & 0xf800) >> 3) | (uint32_t)((cc1 & 0xf800) >> 8)) | ac;
      uint32_t ccg = ((uint32_t)((cc0 & 0x07E0) << 4) | (uint32_t)((cc1 & 0x07E0) >> 2)) | ac;
      uint32_t ccb = ((uint32_t)((cc0 & 0x001F) << 8) | (uint32_t)((cc1 & 0x001F) << 3)) | ac;

      // process 4x4 texels
      uint32_t* texel = texel_x;
      for (uint32_t by = 0; by < 4; by++, texel += width) // next line
      {        
        for (int bx = 0; bx < 4; bx++, ccfnlut >>= 2)
        {
          uint32_t ccfn = (ccfnlut & 0x03);

          *(texel + bx) = (uint32_t)
            (
              __DXT1_LUT_COLOR_VALUE_R[ccr | ccfn] |
              __DXT1_LUT_COLOR_VALUE_G[ccg | ccfn] |
              __DXT1_LUT_COLOR_VALUE_B[ccb | ccfn]
            );          
        }//bx        
      }//by
    }//x_block
  }//y_block
  return;
ProcessWithCheckingTexelBounds:
  //
  // NOTICE: source and target ARE NOT aligned to 4x4 texels, 
  //         We must check for End Of Image (EOI) in this case.
  //
  // lazy to write boundary separate processings.
  // Just end of image (EOI) pointer check only.
  // considering that I haven't encountered any image that is not
  // aligned to 4x4 texel this almost never should be called.
  // and takes 0.5~1 ms more time processing 2MB pixel images.
  //
  uint32_t* EOI = target + (width * height);
  // target : advance by 4 scan lines
  for (uint32_t y_block = 0; y_block < y_block_count; y_block++, target += target_4scans)
  {
    uint32_t* texel_x = target;
    // texel: advance by 4 texels
    for (uint32_t x_block = 0; x_block < x_block_count; x_block++, source += 8, texel_x += 4)
    {
      // read DXT1 (BC1) block data
      uint16_t cc0 = *(uint16_t*)(source);      // 00-01 : cc0       (16bits)
      uint16_t cc1 = *(uint16_t*)(source + 2);  // 02-03 : cc1       (16bits)
      uint32_t ccfnlut = *(uint32_t*)(source + 4);  // 04-07 : ccfn LUT  (32bits) 4x4x2bits
      uint32_t ac = (uint32_t)(cc0 > cc1 ? 0 : 4);

      // color code [r,g,b] indexes to lut(s)
      uint32_t ccr = ((uint32_t)((cc0 & 0xf800) >> 3) | (uint32_t)((cc1 & 0xf800) >> 8)) | ac;
      uint32_t ccg = ((uint32_t)((cc0 & 0x07E0) << 4) | (uint32_t)((cc1 & 0x07E0) >> 2)) | ac;
      uint32_t ccb = ((uint32_t)((cc0 & 0x001F) << 8) | (uint32_t)((cc1 & 0x001F) << 3)) | ac;

      // process 4x4 texels
      uint32_t* texel = texel_x;
      for (uint32_t by = 0; by < 4; by++, texel += width) // next line
      {
        //############################################################
        // Check Y Bound (break: no more texels available for block)
        if (texel >= EOI) break;
        //############################################################
        for (int bx = 0; bx < 4; bx++, ccfnlut >>= 2)
        {
          //############################################################
          // Check X Bound (continue: need ccfnlut to complete shift)
          if (texel + bx >= EOI) continue;
          //############################################################
          uint32_t ccfn = (ccfnlut & 0x03);

          *(texel + bx) = (uint32_t)
            (
              __DXT1_LUT_COLOR_VALUE_R[ccr | ccfn] |
              __DXT1_LUT_COLOR_VALUE_G[ccg | ccfn] |
              __DXT1_LUT_COLOR_VALUE_B[ccb | ccfn]
            );
        }//bx        
      }//by
    }//x_block
  }//y_block
}

#pragma endregion

} //namespace openS3TC
