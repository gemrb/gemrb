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

#include <cstddef>

#include "RGBAColor.h"
#include "exports.h"

#include "Palette.h"
#include "Region.h"
#include "TypeID.h"

namespace GemRB {

class AnimationFactory;

/**
 * @class Sprite2D
 * Class representing bitmap data.
 * Objects of this class are usually created by Video driver.
 */

class Sprite2D;
class GEM_EXPORT Sprite2D : public Held<Sprite2D> {
public:
	static const TypeID ID;
protected:
	bool freePixels;
	void* pixels;

public:
	Region Frame;
	int Bpp;

	bool BAM;
	ieDword renderFlags;

	Sprite2D(const Region&, int Bpp, void* pixels);
	Sprite2D(const Sprite2D &obj);
	virtual Holder<Sprite2D> copy() const = 0;
	~Sprite2D() override;

	bool IsPixelTransparent(const Point& p) const;

	virtual const void* LockSprite() const;
	virtual void* LockSprite();
	virtual void UnlockSprite() const;

	virtual PaletteHolder GetPalette() const = 0;
	virtual void SetPalette(PaletteHolder pal) = 0;
	virtual Color GetPixel(const Point&) const = 0;
	Color GetPixel(int x, int y) const;
	virtual bool HasTransparency() const = 0;
	/* GetColorKey: either a px value or a palete index if sprite has a palette. */
	virtual int32_t GetColorKey() const = 0;
	/* SetColorKey: ieDword is either a px value or a palete index if sprite has a palette. */
	virtual void SetColorKey(ieDword) = 0;
	virtual bool ConvertFormatTo(int /*bpp*/, ieDword /*rmask*/, ieDword /*gmask*/,
							   ieDword /*bmask*/, ieDword /*amask*/) { return false; }; // not pure virtual!
};

}

#endif  // ! SPRITE2D_H
