/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

/**
 * @file Sprite2D.h
 * Declares Sprite2D, class representing bitmap data
 * @author The GemRB Project
 */

#ifndef SPRITE2D_H
#define SPRITE2D_H

#include "RGBAColor.h"
#include "exports.h"

#include "Palette.h"
#include "TypeID.h"

#define RENDER_FLIP_HORIZONTAL	0x00000001
#define RENDER_FLIP_VERTICAL	0x00000002

namespace GemRB {

class AnimationFactory;

/**
 * @class Sprite2D
 * Class representing bitmap data.
 * Objects of this class are usually created by Video driver.
 */

class GEM_EXPORT Sprite2D {
public:
	static const TypeID ID;
private:
	int RefCount;
protected:
	bool freePixels;
public:
	int XPos, YPos, Width, Height, Bpp;

	bool BAM;
	bool RLE; // in theory this could apply to more than BAMs, but currently does not.
	ieDword renderFlags;
	const void* pixels;

	Sprite2D(int Width, int Height, int Bpp, const void* pixels);
	Sprite2D(const Sprite2D &obj);
	virtual Sprite2D* copy() const = 0;
	virtual ~Sprite2D();

	bool IsPixelTransparent(unsigned short x, unsigned short y) const;
	virtual Palette *GetPalette() const = 0;
	virtual const Color* GetPaletteColors() const = 0;
	virtual void SetPalette(Palette *pal) = 0;
	virtual Color GetPixel(unsigned short x, unsigned short y) const = 0;
	/* GetColorKey: either a px value or a palete index if sprite has a palette. */
	virtual ieDword GetColorKey() const = 0;
	/* SetColorKey: ieDword is either a px value or a palete index if sprite has a palette. */
	virtual void SetColorKey(ieDword) = 0;
	virtual bool ConvertFormatTo(int /*bpp*/, ieDword /*rmask*/, ieDword /*gmask*/,
							   ieDword /*bmask*/, ieDword /*amask*/) { return false; }; // not pure virtual!
	virtual void MakeUnused() {}
	void acquire() { ++RefCount; }
	void release();
};

}

#endif  // ! SPRITE2D_H
