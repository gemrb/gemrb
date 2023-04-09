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

#include "Resource.h"
#include "Sprite2D.h"
#include "Streams/DataStream.h"

namespace GemRB {

class ImageFactory;

/**
 * Base class for Image plugins.
 */
class GEM_EXPORT ImageMgr : public Resource {
public:
	static const TypeID ID;
	
public:
	/** Returns a \ref Sprite2D containing the image. */
	virtual Holder<Sprite2D> GetSprite2D() = 0;

	virtual Holder<Sprite2D> GetSprite2D(Region&&) = 0;
	/**
	 * Returns image palette.
	 *
	 * @param[in] colors Number of colors to return.
	 * @param[out] pal Array to fill with colors.
	 *
	 * This does nothing if there is no palette.
	 */
	virtual int GetPalette(int colors, Color* pal);
	
	Size GetSize() const { return size; }

	/**
	 * Returns a \ref ImageFactory for the current image.
	 *
	 * @param[in] ResRef name of image represented by factory.
	 */
	std::shared_ptr<ImageFactory> GetImageFactory(const ResRef& ref);
protected:
	Size size;
};

}

#endif
