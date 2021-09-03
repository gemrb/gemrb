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
	HALFTRANS = 2, // IE_VVC_TRANSPARENT
	BLENDED = 8, // IE_VVC_BLENDED, not implemented in SDLVideo yet
	MIRRORX = 0x10, // IE_VVC_MIRRORX
	MIRRORY = 0x20, // IE_VVC_MIRRORY
	// IE_VVC_TINT = 0x00030000. which is (BlitFlags::COLOR_MOD | BlitFlags::ALPHA_MOD)
	COLOR_MOD = 0x00010000, // srcC = srcC * (color / 255)
	ALPHA_MOD = 0x00020000, // srcA = srcA * (alpha / 255)
	GREY = 0x80000, // IE_VVC_GREYSCALE, timestop palette
	SEPIA = 0x02000000, // IE_VVC_SEPIA, dream scene palette
	MULTIPLY = 0x00100000, // IE_VVC_DARKEN, not implemented in SDLVideo yet
	GLOW = 0x00200000, // IE_VVC_GLOWING, not implemented in SDLVideo yet
	ADD = 0x00400000,
	STENCIL_ALPHA = 0x00800000, // blend with the stencil buffer using the stencil's alpha channel as the stencil
	STENCIL_RED = 0x01000000, // blend with the stencil buffer using the stencil's r channel as the stencil
	STENCIL_GREEN = 0x08000000, // blend with the stencil buffer using the stencil's g channel as the stencil
	STENCIL_BLUE = 0x20000000, // blend with the stencil buffer using the stencil's b channel as the stencil
	STENCIL_DITHER = 0x10000000 // use dithering instead of transpanency. only affects stencil values of 128.
};

#define BLIT_STENCIL_MASK (BlitFlags::STENCIL_ALPHA|BlitFlags::STENCIL_RED|BlitFlags::STENCIL_GREEN|BlitFlags::STENCIL_BLUE|BlitFlags::STENCIL_DITHER)

class GEM_EXPORT Sprite2D : public Held<Sprite2D> {
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
	
	virtual void UpdatePalette(PaletteHolder) noexcept {};
	virtual void UpdateColorKey(colorkey_t) noexcept {};

public:
	Region Frame;
	BlitFlags renderFlags = BlitFlags::NONE;

	Sprite2D(const Region&, void* pixels, const PixelFormat& fmt, uint16_t pitch) noexcept;
	Sprite2D(const Region&, void* pixels, const PixelFormat& fmt) noexcept;
	Sprite2D(const Sprite2D &obj) noexcept;
	Sprite2D(Sprite2D&&) noexcept;
	~Sprite2D() noexcept override;

	virtual Holder<Sprite2D> copy() const { return MakeHolder<Sprite2D>(*this); };

	virtual bool HasTransparency() const noexcept;
	bool IsPixelTransparent(const Point& p) const noexcept;
	
	uint16_t GetPitch() const noexcept { return pitch; }

	virtual const void* LockSprite() const;
	virtual void* LockSprite();
	virtual void UnlockSprite() const {};

	const PixelFormat& Format() const noexcept { return format; }
	Color GetPixel(const Point&) const noexcept;
	PaletteHolder GetPalette() const noexcept { return format.palette; }
	void SetPalette(const PaletteHolder& pal);
	
	/* GetColorKey: either a px value or a palete index if sprite has a palette. */
	colorkey_t GetColorKey() const noexcept { return format.ColorKey; }
	/* SetColorKey: either a px value or a palete index if sprite has a palette. */
	void SetColorKey(colorkey_t);

	virtual bool ConvertFormatTo(const PixelFormat&) noexcept;
};

}

#endif  // ! SPRITE2D_H
