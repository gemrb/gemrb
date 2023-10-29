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
#include "Video/Pixels.h"
#include "Region.h"
#include "TypeID.h"

namespace GemRB {

// Note: not all these flags make sense together.
// Specifically: BlitFlags::GREY overrides BlitFlags::SEPIA
enum BlitFlags : uint32_t {
	NONE = 0,
	HALFTRANS = 2,
	// blend modes are mutually exclusive
	BLENDED = 8, // dstRGB = (srcRGB * srcA) + (dstRGB * (1-srcA)), dstA = srcA + (dstA * (1-srcA))
	ADD = 0x10, //  dstRGB = (srcRGB * srcA) + dstRGB
	MOD = 0x20, // dstRGB = srcRGB * dstRGB
	MUL = 0x40, // dstRGBA = (srcRGBA * dstRGBA) + (dstRGBA * (1 - srcA))
	ONE_MINUS_DST = 0x80, // dstRGBA = (srcRGBA * (1 - dstRGBA)) + (dstRGBA)
	DST = 0x100, // dstRGBA = (srcRGBA * dstRGBA) + (dstRGBA)
	SRC = 0x200, // dstRGBA = (srcRGBA * srcRGBA) + (dstRGBA)
	BLEND_MASK = BLENDED | ADD | MOD | MUL | ONE_MINUS_DST | DST | SRC,
	// color/alpha mod applies to color param
	COLOR_MOD = 0x1000, // srcC = srcC * (color / 255)
	ALPHA_MOD = 0x2000, // srcA = srcA * (alpha / 255)
	MIRRORX = 0x10000,
	MIRRORY = 0x20000,
	GREY = 0x80000, // timestop palette
	SEPIA = 0x02000000, // dream scene palette
	// stencil flags operate on the stencil buffer
	STENCIL_ALPHA = 0x00800000, // blend with the stencil buffer using the stencil's alpha channel as the stencil
	STENCIL_RED = 0x01000000, // blend with the stencil buffer using the stencil's r channel as the stencil
	STENCIL_GREEN = 0x08000000, // blend with the stencil buffer using the stencil's g channel as the stencil
	STENCIL_BLUE = 0x20000000, // blend with the stencil buffer using the stencil's b channel as the stencil
	STENCIL_DITHER = 0x10000000, // use dithering instead of transpanency. only affects stencil values of 128.
	STENCIL_MASK = STENCIL_ALPHA | STENCIL_RED | STENCIL_GREEN | STENCIL_BLUE | STENCIL_DITHER
};

class GEM_EXPORT Sprite2D {
public:
	static const TypeID ID;
	
	using Iterator = PixelFormatIterator;
	Iterator GetIterator(IPixelIterator::Direction xdir = IPixelIterator::Forward,
						 IPixelIterator::Direction ydir = IPixelIterator::Forward);

	Iterator GetIterator(IPixelIterator::Direction xdir,
						 IPixelIterator::Direction ydir,
						 const Region& clip);
	
	Iterator GetIterator(IPixelIterator::Direction xdir = IPixelIterator::Forward,
						 IPixelIterator::Direction ydir = IPixelIterator::Forward) const;

	Iterator GetIterator(IPixelIterator::Direction xdir,
						 IPixelIterator::Direction ydir,
						 const Region& clip) const;

protected:
	void* pixels = nullptr;
	bool freePixels = true;
	
	PixelFormat format;
	uint16_t pitch;
	
	virtual void UpdatePalette() noexcept {};
	virtual void UpdateColorKey() noexcept {};

public:
	Region Frame;
	BlitFlags renderFlags = BlitFlags::NONE;

	Sprite2D(const Region&, void* pixels, const PixelFormat& fmt, uint16_t pitch) noexcept;
	Sprite2D(const Region&, void* pixels, const PixelFormat& fmt) noexcept;
	Sprite2D(const Sprite2D &obj) noexcept;
	Sprite2D(Sprite2D&&) noexcept;
	virtual ~Sprite2D() noexcept;

	virtual Holder<Sprite2D> copy() const { return MakeHolder<Sprite2D>(*this); };

	virtual bool HasTransparency() const noexcept;
	bool IsPixelTransparent(const Point& p) const noexcept;
	
	uint16_t GetPitch() const noexcept { return pitch; }

	virtual const void* LockSprite() const;
	virtual void* LockSprite();
	virtual void UnlockSprite() const {};

	const PixelFormat& Format() const noexcept { return format; }
	Color GetPixel(const Point&) const noexcept;
	Holder<Palette> GetPalette() const noexcept { return format.palette; }
	void SetPalette(const Holder<Palette>& pal);
	
	/* GetColorKey: either a px value or a palette index if sprite has a palette. */
	colorkey_t GetColorKey() const noexcept { return format.ColorKey; }
	/* SetColorKey: either a px value or a palette index if sprite has a palette. */
	void SetColorKey(colorkey_t);

	virtual bool ConvertFormatTo(const PixelFormat&) noexcept;
};

}

#endif  // ! SPRITE2D_H
