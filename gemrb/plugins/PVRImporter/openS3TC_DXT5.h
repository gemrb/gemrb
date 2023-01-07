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

#pragma once 

#include "stdafx.h"

namespace openS3TC {

typedef enum DXT5PixelFormat
{
  /// <summary>4 byte texel:|B|G|R|A| (also default if incorrect pixel format specified.)</summary>
  DXT5PixelFormat_BGRA = 0,
  /// <summary>4 byte texel:|R|G|B|A|</summary>
  DXT5PixelFormat_RGBA = 1,
  /// <summary>4 byte texel:|A|R|G|B|</summary>
  DXT5PixelFormat_ARGB = 2,
  /// <summary>4 byte texel:|A|B|G|R|</summary>
  DXT5PixelFormat_ABGR = 3,
} DXT5PixelFormat;

extern "C"
{
  void DXT5ReleaseLUTs();
  void DXT5SetOutputPixelFormat(DXT5PixelFormat pixelFormat);  
  void DXT5Decompress(uint32_t width, uint32_t height, uint8_t* p_input, uint8_t* p_output);
}

} //namespace openS3TC
