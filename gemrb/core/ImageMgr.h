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
 *
 */

#ifndef IMAGEMGR_H
#define IMAGEMGR_H

#include "exports.h"

#include "Bitmap.h"
#include "Image.h"
#include "Resource.h"
#include "Sprite2D.h"
#include "System/DataStream.h"

namespace GemRB {

class ImageFactory;

/**
 * Base class for Image plugins.
 */
class GEM_EXPORT ImageMgr : public Resource {
public:
	static const TypeID ID;
public:
	ImageMgr(void);
	virtual ~ImageMgr(void);
	/** Returns a \ref Sprite2D containing the image. */
	virtual Sprite2D* GetSprite2D() = 0;
	virtual Image* GetImage();
	virtual Bitmap* GetBitmap();
	/**
	 * Returns image palette.
	 *
	 * @param[in] colors Number of colors to return.
	 * @param[out] pal Array to fill with colors.
	 *
	 * This does nothing if there is no palette.
	 */
	virtual void GetPalette(int colors, Color* pal);
	/** Returns the width of the image */
	virtual int GetWidth() = 0;
	/** Returns the height of the image */
	virtual int GetHeight() = 0;
	/**
	 * Returns a \ref ImageFactory for the current image.
	 *
	 * @param[in] ResRef name of image represented by factory.
	 */
	ImageFactory* GetImageFactory(const char* ResRef);
};

}

#endif
