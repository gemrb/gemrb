/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2018 The GemRB Project
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

#ifndef PIXELS_H
#define PIXELS_H

#include "Holder.h"
#include "Region.h"
#include "Palette.h"

#define ERROR_UNKNOWN_BPP error("Video", "Invalid bpp.")

namespace GemRB {

using colorkey_t = uint32_t;

struct GEM_EXPORT PixelFormat {
	uint8_t  Rloss = 0;
	uint8_t  Gloss = 0;
	uint8_t  Bloss = 0;
	uint8_t  Aloss = 0;
	uint8_t  Rshift = 0;
	uint8_t  Gshift = 0;
	uint8_t  Bshift = 0;
	uint8_t  Ashift = 0;
	uint32_t Rmask = 0;
	uint32_t Gmask = 0;
	uint32_t Bmask = 0;
	uint32_t Amask = 0;
	
	uint8_t Bpp = 0; // bytes per pixel
	uint8_t Depth = 0;
	colorkey_t ColorKey = 0;
	bool HasColorKey = false;
	bool RLE = false;
	Holder<Palette> palette;
	
	PixelFormat() noexcept = default;
	PixelFormat(uint8_t bpp, uint32_t rmask, uint32_t gmask, uint32_t bmask, uint32_t amask) noexcept
	: Rmask(rmask), Gmask(gmask), Bmask(bmask), Amask(amask), Bpp(bpp), Depth(bpp * 8)
	{}
	
	// we wouldnt need this monster in C++14...
	PixelFormat(uint8_t rloss, uint8_t gloss, uint8_t bloss, uint8_t aloss,
				uint8_t rshift, uint8_t gshift, uint8_t bshift, uint8_t ashift,
				uint32_t rmask, uint32_t gmask, uint32_t bmask, uint32_t amask,
				uint8_t bpp, uint8_t depth,
				colorkey_t ck, bool hasCk, bool rle, Holder<Palette> pal) noexcept
	: Rloss(rloss), Gloss(gloss), Bloss(bloss), Aloss(aloss),
	Rshift(rshift), Gshift(gshift), Bshift(bshift), Ashift(ashift),
	Rmask(rmask), Gmask(gmask), Bmask(bmask), Amask(amask),
	Bpp(bpp), Depth(depth), ColorKey(ck), HasColorKey(hasCk),
	RLE(rle), palette(std::move(pal))
	{}
	
	static PixelFormat Paletted8Bit(Holder<Palette> pal, bool hasColorKey = false, colorkey_t key = 0) {
		return PixelFormat {
			0, 0, 0, 0,
			0, 0, 0, 0,
			0, 0, 0, 0,
			1, 8,
			key, hasColorKey,
			false, std::move(pal)
		};
	}
	
	static PixelFormat RLE8Bit(Holder<Palette> pal, colorkey_t key) {
		return PixelFormat {
			0, 0, 0, 0,
			0, 0, 0, 0,
			0, 0, 0, 0,
			1, 8,
			key, true,
			true, std::move(pal)
		};
	}

	static PixelFormat ARGB32Bit() {
		return PixelFormat {
			0, 0, 0, 0,
			16, 8, 0, 24,
			0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000,
			4, 32,
			0, false,
			false, {}
		};
	}
};

#pragma pack(push,1)
struct GEM_EXPORT Pixel24Bit {
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
	uint8_t r;
	uint8_t g;
	uint8_t b;
#else
	uint8_t b;
	uint8_t g;
	uint8_t r;
#endif
	
	Pixel24Bit& operator=(uint32_t pixel) {
		r = (pixel & 0xff000000) >> 24;
		g = (pixel & 0x00ff0000) >> 16;
		b = (pixel & 0x0000ff00) >> 8;
		return *this;
	}
	
	operator uint32_t() const {
		return r + ((uint32_t)g << 8) + ((uint32_t)b << 16);
	}
};
#pragma pack(pop)
static_assert(sizeof(Pixel24Bit) == 3, "24bit pixel should be 3 bytes.");

struct GEM_EXPORT RGBBlender {
	virtual void operator()(const Color& src, Color& dst, uint8_t mask) const=0;
	virtual ~RGBBlender() noexcept = default;
};

inline void ShaderTint(const Color& tint, Color& c) {
	c.r = (tint.r * c.r) >> 8;
	c.g = (tint.g * c.g) >> 8;
	c.b = (tint.b * c.b) >> 8;
}

inline void ShaderGreyscale(Color& c) {
	c.r >>= 2;
	c.g >>= 2;
	c.b >>= 2;
	uint8_t avg = c.r + c.g + c.b;
	c.r = c.g = c.b = avg;
}

inline void ShaderSepia(Color& c) {
	c.r >>= 2;
	c.g >>= 2;
	c.b >>= 2;
	uint8_t avg = c.r + c.g + c.b;
	c.r = avg + 21; // can't overflow, since a is at most 189
	c.g = avg;
	c.b = avg < 32 ? 0 : avg - 32;
}

template <bool ALPHA>
inline void ShaderBlend(const Color& src, Color& dst) {
	// macro for a fast approximation for division by 255 (more accurate than >> 8)
	// TODO: this is probably only faster on old CPUs, should #ifdef on arch
#define DIV255(x) ((x + 1 + (x>>8)) >> 8) //((x)/255)
	dst.r = DIV255(src.a * src.r) + DIV255((255 - src.a) * dst.r);
	dst.g = DIV255(src.a * src.g) + DIV255((255 - src.a) * dst.g);
	dst.b = DIV255(src.a * src.b) + DIV255((255 - src.a) * dst.b);
	if (ALPHA)
		dst.a = src.a + DIV255((255 - src.a) * dst.a);
#undef DIV255
}

inline void ShaderAdditive(const Color& src, Color& dst) {
	// dstRGB = (srcRGB * srcA) + dstRGB
	// dstA = dstA
	// FIXME: I guess we will ignore alpha for now...
	dst.r = src.r + dst.r;
	dst.g = src.g + dst.g;
	dst.b = src.b + dst.b;
}

template <bool MASKED, bool SRCALPHA>
struct OneMinusSrcA : RGBBlender {
	void operator()(const Color& c, Color& dst, uint8_t mask) const override {
		ShaderBlend<SRCALPHA>(c, dst);
		if (MASKED) {
			dst.a = mask ? (255 - mask) + (c.a * mask) : c.a; // FIXME: not sure this is 100% correct, but it passes my known tests
		}
	}
};

template <bool MASKED>
struct TintDst : RGBBlender {
	void operator()(const Color& c, Color& dst, uint8_t mask) const override {
		ShaderTint(c, dst);
		if (MASKED) {
			dst.a = mask ? (255 - mask) + (c.a * mask) : c.a; // FIXME: not sure this is 100% correct, but it passes my known tests
		}
	}
};

template <bool MASKED>
struct SrcRGBA : RGBBlender {
	void operator()(const Color& c, Color& dst, uint8_t mask) const override {
		dst = c;
		if (MASKED) {
			dst.a = mask ? (255 - mask) + (c.a * mask) : c.a; // FIXME: not sure this is 100% correct, but it passes my known tests
		}
	}
};

enum class SHADER {
	NONE,
	BLEND,
	TINT,
	GREYSCALE,
	SEPIA
};

// using a template to avoid runtime branch evaluation
// by optimizing down to a single case
template <SHADER SHADE, bool SRCALPHA>
class RGBBlendingPipeline : private RGBBlender {
	Color tint;
	unsigned int shift;
	void (*blender)(const Color& src, Color& dst);

public:
	explicit RGBBlendingPipeline(void (*blender)(const Color& src, Color& dst) = ShaderBlend<SRCALPHA>)
	: tint(1,1,1,0xff), blender(blender) {
		shift = 0;
		if (SHADE == SHADER::GREYSCALE || SHADE == SHADER::SEPIA) {
			shift += 2;
		}
	}

	explicit RGBBlendingPipeline(const Color& tint, void (*blender)(const Color& src, Color& dst) = ShaderBlend<SRCALPHA>)
	: tint(tint), blender(blender) {
		shift = 8; // we shift by 8 as a fast aproximation of dividing by 255
		if (SHADE == SHADER::GREYSCALE || SHADE == SHADER::SEPIA) {
			shift += 2;
		}
	}

	void operator()(const Color& src, Color& dst, uint8_t mask) const override {
		if (SRCALPHA && src.a == 0) {
			return;
		}
		
		Color c = src;
		c.a = mask ? (255 - mask) + (c.a * mask) : c.a; // FIXME: not sure this is 100% correct, but it passes my known tests

		// first apply "shader"
		switch (SHADE) {
			case SHADER::TINT:
				assert(shift);
				c.r = (tint.r * c.r) >> shift;
				c.g = (tint.g * c.g) >> shift;
				c.b = (tint.b * c.b) >> shift;
				break;
			case SHADER::GREYSCALE:
				{
					c.r = (tint.r * c.r) >> shift;
					c.g = (tint.g * c.g) >> shift;
					c.b = (tint.b * c.b) >> shift;
					uint8_t avg = c.r + c.g + c.b;
					c.r = c.g = c.b = avg;
				}
				break;
			case SHADER::SEPIA:
				{
					c.r = (tint.r * c.r) >> shift;
					c.g = (tint.g * c.g) >> shift;
					c.b = (tint.b * c.b) >> shift;
					uint8_t avg = c.r + c.g + c.b;
					c.r = avg + 21; // can't overflow, since a is at most 189
					c.g = avg;
					c.b = avg < 32 ? 0 : avg - 32;
				}
				break;
			case SHADER::NONE:
				// fallthough
			default:
				break;
		}

		blender(c, dst);
	}
};

struct GEM_EXPORT IPixelIterator
{
	enum Direction {
		Reverse = -1,
		Forward = 1
	};

	void* pixel;
	//int offset; // current pixel distance from 'pixel'
	int pitch; // in bytes

	Direction xdir;
	Direction ydir;

	IPixelIterator(void* px, int pitch, Direction x, Direction y) noexcept
	: pixel(px), pitch(pitch), xdir(x), ydir(y) {}

	virtual ~IPixelIterator() noexcept = default;

	virtual IPixelIterator* Clone() const noexcept=0;
	virtual void Advance(int) noexcept=0;
	virtual uint8_t Channel(uint32_t mask, uint8_t shift) const noexcept=0;
	virtual const Point& Position() const noexcept=0;

	IPixelIterator& operator++() noexcept {
		Advance(1);
		return *this;
	}

	bool operator!=(const IPixelIterator& rhs) const noexcept {
		return pixel != rhs.pixel;
	}
};

template <typename PIXEL>
class PixelIterator : public IPixelIterator
{
protected:
	Size size;
	Point pos;

public:
	PixelIterator(PIXEL* p, Size s, int pitch)
	: IPixelIterator(p, pitch, Forward, Forward), size(s) {
		assert(size.w >= 0); // == 0 is the same thing as an end iterator so it is valid too
		assert(pitch >= size.w);
	}

	PixelIterator(PIXEL* p, Direction x, Direction y, Size s, int pitch)
	: IPixelIterator(p, pitch, x, y), size(s) {
		assert(size.w >= 0); // == 0 is the same thing as an end iterator so it is valid too
		assert(pitch >= size.w);
		pos.x = (x == Reverse) ? size.w - 1 : 0;
		pos.y = (y == Reverse) ? size.h - 1 : 0;
	}

	virtual PIXEL& operator*() const {
		return *static_cast<PIXEL*>(pixel);
	}

	virtual PIXEL* operator->() {
		return static_cast<PIXEL*>(pixel);
	}

	uint8_t Channel(uint32_t mask, uint8_t shift) const noexcept override {
		return ((*static_cast<PIXEL*>(pixel))&mask) >> shift;
	}

	IPixelIterator* Clone() const noexcept override {
		return new PixelIterator<PIXEL>(*this);
	}

	void Advance(int dx) noexcept override {
		if (dx == 0 || size.IsInvalid()) return;

		uint8_t* ptr = static_cast<uint8_t*>(pixel);
		
		int pixelsToAdvance = xdir * dx;
		int rowsToAdvance = std::abs(pixelsToAdvance / size.w);
		int xToAdvance = pixelsToAdvance % size.w;
		int tmpx = pos.x + xToAdvance;
		
		if (tmpx < 0) {
			++rowsToAdvance;
			tmpx = size.w + tmpx;
			xToAdvance = tmpx - pos.x;
		} else if (tmpx >= size.w) {
			++rowsToAdvance;
			tmpx = tmpx - size.w;
			xToAdvance = tmpx - pos.x;
		}
		
		if (dx < 0) {
			ptr -= pitch * rowsToAdvance * ydir;
			pos.y -= rowsToAdvance;
		} else {
			ptr += pitch * rowsToAdvance * ydir;
			pos.y += rowsToAdvance;
		}
		
		ptr += int(xToAdvance * sizeof(PIXEL));
		
		pos.x = tmpx;
		assert(pos.x >= 0 && pos.x < size.w);
		pixel = ptr;
	}
	
	const Point& Position() const noexcept override {
		return pos;
	}
};

class GEM_EXPORT PixelFormatIterator : public IPixelIterator
{
private:
	IPixelIterator* imp = nullptr;
	IPixelIterator* InitImp(void* pixel, int pitch) const noexcept;

public:
	const PixelFormat& format;
	Region clip;

	PixelFormatIterator(void* pixels, int pitch, const PixelFormat& fmt, const Region& clip) noexcept;
	PixelFormatIterator(void* pixels, int pitch, const PixelFormat& fmt, Direction x, Direction y, const Region& clip) noexcept;
	PixelFormatIterator(const PixelFormatIterator& orig) noexcept;

	~PixelFormatIterator() noexcept override;

	static PixelFormatIterator end(const PixelFormatIterator& beg) noexcept;

	PixelFormatIterator& operator++() noexcept;

	bool operator!=(const PixelFormatIterator& rhs) const noexcept;

	uint8_t& operator*() const noexcept;

	uint8_t* operator->() const noexcept;

	uint8_t Channel(uint32_t mask, uint8_t shift) const noexcept override;

	IPixelIterator* Clone() const noexcept override;

	void Advance(int amt) noexcept override;
	
	Color ReadRGBA() const noexcept;

	void ReadRGBA(uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a) const noexcept;

	void WriteRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a) noexcept;
	
	const Point& Position() const noexcept override;
};

struct GEM_EXPORT IAlphaIterator
{
	virtual ~IAlphaIterator() noexcept = default;
	
	virtual uint8_t operator*() const=0;
	virtual void Advance(int)=0;
	
	IAlphaIterator& operator++() {
		Advance(1);
		return *this;
	}
};

// an endless iterator that always returns 'alpha' when dereferenced
struct GEM_EXPORT StaticAlphaIterator : public IAlphaIterator
{
	uint8_t alpha;

	explicit StaticAlphaIterator(uint8_t a) : alpha(a) {}

	uint8_t operator*() const override {
		return alpha;
	}

	void Advance(int) override {}
};

struct GEM_EXPORT RGBAChannelIterator : public IAlphaIterator
{
	uint32_t mask;
	uint8_t shift;
	IPixelIterator& pixelIt;

	RGBAChannelIterator(IPixelIterator& it, uint32_t mask, uint8_t shift)
	: mask(mask), shift(shift), pixelIt(it)
	{}

	uint8_t operator*() const override {
		return pixelIt.Channel(mask, shift);
	}
	
	void Advance(int amt) override {
		pixelIt.Advance(amt);
	}
};

}

#endif // PIXELS_H
