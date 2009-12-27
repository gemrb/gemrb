/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $Id$
 *
 */

#ifndef IMAGEMGR_H
#define IMAGEMGR_H

#include "Resource.h"
#include "DataStream.h"
#include "Sprite2D.h"

class ImageFactory;

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

class GEM_EXPORT ImageMgr : public Resource {
public:
	static const TypeID ID;
public:
	ImageMgr(void);
	virtual ~ImageMgr(void);
	virtual bool Open(DataStream* stream, bool autoFree = true) = 0;
	// Swallows an image so it can be saved w/ PutImage() etc.
	// FIXME: should be abstract, but I don't want to implement it for all
	// image managers right now
	virtual bool OpenFromImage(Sprite2D* sprite, bool autoFree = true);
	virtual Sprite2D* GetImage() = 0;
	/** No descriptions */
	virtual void GetPalette(int index, int colors, Color* pal) = 0;
	/** Sets a Pixel in the image, only for searchmaps */
	virtual void SetPixelIndex(unsigned int x, unsigned int y, unsigned int idx) = 0;
	/** Gets a Pixel from the Image */
	virtual Color GetPixel(unsigned int x, unsigned int y) = 0;
	virtual unsigned int GetPixelIndex(unsigned int x, unsigned int y) = 0;
	virtual int GetWidth() = 0;
	virtual int GetHeight() = 0;
	// FIXME: should be abstract, but I don't want to implement it for all
	// image managers right now
	virtual void PutImage(DataStream *output);

	virtual ImageFactory* GetImageFactory(const char* ResRef) = 0;
protected:
	/** not virtual */
	Color GetPixelSum(unsigned int x, unsigned int y, unsigned int ratio);
};

#endif
