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

#include "Video.h"

namespace GemRB {

struct RGBBlender {
	virtual void operator()(const Color& src, Color& dst, uint8_t mask) const=0;
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
			dst.a = (mask) ? (255-mask) + (c.a * mask) : c.a; // FIXME: not sure this is 100% correct, but it passes my known tests
		}
	}
};

template <bool MASKED>
struct TintDst : RGBBlender {
	void operator()(const Color& c, Color& dst, uint8_t mask) const override {
		ShaderTint(c, dst);
		if (MASKED) {
			dst.a = (mask) ? (255-mask) + (c.a * mask) : c.a; // FIXME: not sure this is 100% correct, but it passes my known tests
		}
	}
};

template <bool MASKED>
struct SrcRGBA : RGBBlender {
	void operator()(const Color& c, Color& dst, uint8_t mask) const override {
		dst = c;
		if (MASKED) {
			dst.a = (mask) ? (255-mask) + (c.a * mask) : c.a; // FIXME: not sure this is 100% correct, but it passes my known tests
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
class RGBBlendingPipeline : RGBBlender {
	Color tint;
	unsigned int shift;
	void (*blender)(const Color& src, Color& dst);

public:
	RGBBlendingPipeline(void (*blender)(const Color& src, Color& dst) = ShaderBlend<SRCALPHA>)
	: tint(1,1,1,0xff), blender(blender) {
		shift = 0;
		if (SHADE == SHADER::GREYSCALE || SHADE == SHADER::SEPIA) {
			shift += 2;
		}
	}

	RGBBlendingPipeline(const Color& tint, void (*blender)(const Color& src, Color& dst) = ShaderBlend<SRCALPHA>)
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
		c.a = (mask) ? (255-mask) + (c.a * mask) : c.a; // FIXME: not sure this is 100% correct, but it passes my known tests

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

struct IPixelIterator
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

	IPixelIterator(void* px, int pitch, Direction x, Direction y)
	: pixel(px), pitch(pitch), xdir(x), ydir(y) {}

	virtual ~IPixelIterator() {};

	virtual IPixelIterator* Clone() const=0;
	virtual void Advance(int)=0;
	virtual uint8_t Channel(uint32_t mask, uint8_t shift) const=0;
	virtual const Point& Position() const=0;

	IPixelIterator& operator++() {
		Advance(1);
		return *this;
	}

	bool operator!=(const IPixelIterator& rhs) const {
		return pixel != rhs.pixel;
	}
};

template <typename PIXEL>
struct PixelIterator : IPixelIterator
{
	Size size;
	Point pos;

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

	uint8_t Channel(uint32_t mask, uint8_t shift) const override {
		return ((*static_cast<PIXEL*>(pixel))&mask) >> shift;
	}

	IPixelIterator* Clone() const override {
		return new PixelIterator<PIXEL>(*this);
	}

	void Advance(int dx) override {
		if (dx == 0 || size.IsEmpty()) return;

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
	
	const Point& Position() const override {
		return pos;
	}
};

struct IAlphaIterator
{
	virtual ~IAlphaIterator() = default;
	
	virtual uint8_t operator*() const=0;
	virtual void Advance(int)=0;
	
	IAlphaIterator& operator++() {
		Advance(1);
		return *this;
	}
};

// an endless iterator that always returns 'alpha' when dereferenced
struct StaticAlphaIterator : public IAlphaIterator
{
	uint8_t alpha;

	StaticAlphaIterator(uint8_t a) : alpha(a) {}

	uint8_t operator*() const override {
		return alpha;
	}

	void Advance(int) override {}
};

struct RGBAChannelIterator : public IAlphaIterator
{
	uint32_t mask;
	uint8_t shift;
	IPixelIterator* pixelIt;

	RGBAChannelIterator(IPixelIterator* it, uint32_t mask, uint8_t shift)
	: mask(mask), shift(shift), pixelIt(it)
	{}

	uint8_t operator*() const override {
		return pixelIt->Channel(mask, shift);
	}
	
	void Advance(int amt) override {
		pixelIt->Advance(amt);
	}
};

}

#endif // PIXELS_H
