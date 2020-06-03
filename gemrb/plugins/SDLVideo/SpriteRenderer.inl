/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2012 The GemRB Project
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

// For debugging:
//#define HIGHLIGHTCOVER

using namespace GemRB;

// For pixel formats:
// We hardcode a single pixel format per bit depth.

#if TARGET_OS_MAC

const unsigned int RLOSS16 = 3;
const unsigned int GLOSS16 = 2;
const unsigned int BLOSS16 = 3;
const unsigned int RSHIFT16 = 11;
const unsigned int GSHIFT16 = 5;
const unsigned int BSHIFT16 = 0;

const unsigned int RSHIFT32 = 8;
const unsigned int GSHIFT32 = 16;
const unsigned int BSHIFT32 = 24;

#else

const unsigned int RLOSS16 = 3;
const unsigned int GLOSS16 = 2;
const unsigned int BLOSS16 = 3;
const unsigned int RSHIFT16 = 11;
const unsigned int GSHIFT16 = 5;
const unsigned int BSHIFT16 = 0;

const unsigned int RSHIFT32 = 16;
const unsigned int GSHIFT32 = 8;
const unsigned int BSHIFT32 = 0;

#endif

const Uint16 halfmask16 = ((0xFFU >> (RLOSS16+1)) << RSHIFT16) | ((0xFFU >> (GLOSS16+1)) << GSHIFT16) | ((0xFFU >> (BLOSS16+1)) << BSHIFT16);
const Uint32 halfmask32 = ((0xFFU >> 1) << RSHIFT32) | ((0xFFU >> 1) << GSHIFT32) | ((0xFFU >> 1) << BSHIFT32);

template<bool PALALPHA>
struct SRTinter_NoTint {
	void operator()(Uint8&, Uint8&, Uint8&, Uint8& a, unsigned int) const {
		if (!PALALPHA) a = 255;
	}
};

template<bool PALALPHA, bool TINTALPHA>
struct SRTinter_Tint {
	SRTinter_Tint(const Color& t) : tint(t) { }

	void operator()(Uint8& r, Uint8& g, Uint8& b, Uint8& a, unsigned int) const {
		r = (tint.r * r) >> 8;
		g = (tint.g * g) >> 8;
		b = (tint.b * b) >> 8;
		if (TINTALPHA && PALALPHA) a = (tint.a * a) >> 8;
		//if (!TINTALPHA && PALALPHA) a = a;
		if (TINTALPHA && !PALALPHA) a = tint.a;
		if (!TINTALPHA && !PALALPHA) a = 255;
	}
	Color tint;
};

// Always tint, and conditionally handle grey, red
template<bool PALALPHA>
struct SRTinter_Flags {
	SRTinter_Flags(const Color& t) : tint(t) { }

	void operator()(Uint8& r, Uint8& g, Uint8& b, Uint8& a, unsigned int flags) const {
		if (flags & BLIT_GREY) {
			r = (tint.r * r) >> 10;
			g = (tint.g * g) >> 10;
			b = (tint.b * b) >> 10;
			Uint8 avg = r + g + b;
			r = g = b = avg;
		} else if (flags & BLIT_SEPIA) {
			r = (tint.r * r) >> 10;
			g = (tint.g * g) >> 10;
			b = (tint.b * b) >> 10;
			Uint8 avg = r + g + b;
			r = avg + 21; // can't overflow, since a is at most 189
			g = avg;
			b = avg < 32 ? 0 : avg - 32;
		} else {
			r = (tint.r * r) >> 8;
			g = (tint.g * g) >> 8;
			b = (tint.b * b) >> 8;
		}

		if (!PALALPHA)
			a = tint.a;
		else
			a = (tint.a * a) >> 8;
	}

	Color tint;
};

// Don't tint, but conditionally handle grey, sepia
template<bool PALALPHA>
struct SRTinter_FlagsNoTint {
	SRTinter_FlagsNoTint() { }

	void operator()(Uint8& r, Uint8& g, Uint8& b, Uint8& a, unsigned int flags) const {
		if (flags & BLIT_GREY) {
			r >>= 2;
			g >>= 2;
			b >>= 2;
			Uint8 avg = r + g + b;
			r = g = b = avg;
		} else if (flags & BLIT_SEPIA) {
			r >>= 2;
			g >>= 2;
			b >>= 2;
			Uint8 avg = r + g + b;
			r = avg + 21; // can't overflow, since a is at most 189
			g = avg;
			b = avg < 32 ? 0 : avg - 32;
		}

		if (!PALALPHA) a = 255;
	}
};


struct SRBlender_NoAlpha { };
struct SRBlender_HalfAlpha { };
struct SRBlender_Alpha { };

struct SRFormat_Hard { };
struct SRFormat_Soft { };

template<typename PTYPE, typename BLENDER, typename PIXELFORMAT>
struct SRBlender {
	void operator()(PTYPE& /*pix*/, Uint8 /*r*/, Uint8 /*g*/, Uint8 /*b*/, Uint8 /*a*/) const { assert(false); }
};



// 16 bpp, 565
template<>
struct SRBlender<Uint16, SRBlender_NoAlpha, SRFormat_Hard> {
	void operator()(Uint16& pix, Uint8 r, Uint8 g, Uint8 b, Uint8) const {
		pix = ((r >> RLOSS16) << RSHIFT16) |
		      ((g >> GLOSS16) << GSHIFT16) |
		      ((b >> BLOSS16) << BSHIFT16);
	}
};

template<>
struct SRBlender<Uint16, SRBlender_HalfAlpha, SRFormat_Hard> {
	void operator()(Uint16& pix, Uint8 r, Uint8 g, Uint8 b, Uint8) const {
		pix = ((pix >> 1) & halfmask16) +
		      ((((r >> (RLOSS16+1)) << RSHIFT16) | ((g >> (GLOSS16+1)) << GSHIFT16) | ((b >> (BLOSS16+1)) << BSHIFT16)) & halfmask16);
	}
};

template<>
struct SRBlender<Uint16, SRBlender_Alpha, SRFormat_Hard> {
	void operator()(Uint16& pix, Uint8 r, Uint8 g, Uint8 b, Uint8 a) const {
		unsigned int dr = 1 + a*(r >> RLOSS16) + (255-a)*((pix >> RSHIFT16) & ((1 << (8-RLOSS16))-1));
		unsigned int dg = 1 + a*(g >> GLOSS16) + (255-a)*((pix >> GSHIFT16) & ((1 << (8-GLOSS16))-1));
		unsigned int db = 1 + a*(b >> BLOSS16) + (255-a)*((pix >> BSHIFT16) & ((1 << (8-BLOSS16))-1));

		r = (dr + (dr>>8)) >> 8;
		g = (dg + (dg>>8)) >> 8;
		b = (db + (db>>8)) >> 8;
		pix = (r << RSHIFT16) |
		      (g << GSHIFT16) |
		      (b << BSHIFT16);
	}
};

// 32 bpp, 888
template<>
struct SRBlender<Uint32, SRBlender_NoAlpha, SRFormat_Hard> {
	void operator()(Uint32& pix, Uint8 r, Uint8 g, Uint8 b, Uint8) const {
		pix = (r << RSHIFT32) |
		      (g << GSHIFT32) |
		      (b << BSHIFT32);
	}
};

template<>
struct SRBlender<Uint32, SRBlender_HalfAlpha, SRFormat_Hard> {
	void operator()(Uint32& pix, Uint8 r, Uint8 g, Uint8 b, Uint8) const {
		pix = ((pix >> 1) & halfmask32) +
		      ((((r << RSHIFT32) | (g << GSHIFT32) | (b << BSHIFT32)) >> 1) & halfmask32);
	}
};


template<>
struct SRBlender<Uint32, SRBlender_Alpha, SRFormat_Hard> {
	void operator()(Uint32& pix, Uint8 r, Uint8 g, Uint8 b, Uint8 a) const {
		unsigned int dr = 1 + a*r + (255-a)*((pix >> RSHIFT32) & 0xFF);
		unsigned int dg = 1 + a*g + (255-a)*((pix >> GSHIFT32) & 0xFF);
		unsigned int db = 1 + a*b + (255-a)*((pix >> BSHIFT32) & 0xFF);
		r = (dr + (dr>>8)) >> 8;
		g = (dg + (dg>>8)) >> 8;
		b = (db + (db>>8)) >> 8;
		pix = (r << RSHIFT32) |
		      (g << GSHIFT32) |
		      (b << BSHIFT32);
	}
};

// RLE, palette
template<typename PTYPE, bool COVER, bool XFLIP, typename Tinter, typename Blender>
static void BlitSpriteRLE_internal(SDL_Surface* target,
            const Uint8* srcdata, const Color* col,
            const int tx, const int ty,
            int width, int height,
            bool yflip,
            Region clip,
            Uint8 transindex,
            IAlphaIterator& cover,
            unsigned int flags,
            const Tinter& tint, const Blender& blend, PTYPE /*dummy*/ = 0)
{
	int pitch = target->pitch / target->format->BytesPerPixel;

	// We assume the clipping rectangle is the exact rectangle in which we will
	// paint. This means clip rect <= sprite rect <= cover rect

	assert(clip.w > 0 && clip.h > 0);
	assert(clip.x >= tx);
	assert(clip.y >= ty);

	// Clipping strategy:
	// Because we can't jump to the right spot in the RLE data,
	// we have to process the full sprite.
	// We fast-forward through the bits outside of the clipping rectangle.

	// This is done line-by-line.
	// To avoid having to fast-forward, blit, fast-forward for each line,
	// we consider the end of each line to be directly behind the clipping
	// rectangle, so that we only do a single fast-forward loop followed by a
	// blit loop.


	PTYPE *clipstartpix, *clipendpix;
	PTYPE *clipstartline;

	if (!yflip) {
		clipstartline = (PTYPE*)target->pixels + clip.y*pitch;
	} else {
		clipstartline = (PTYPE*)target->pixels + (clip.y + clip.h - 1)*pitch;
	}

	PTYPE *line, *end, *pix;
	
	if (!yflip) {
		line = (PTYPE*)target->pixels + ty*pitch;
		end = (PTYPE*)target->pixels + (clip.y + clip.h)*pitch;
	} else {
		line = (PTYPE*)target->pixels + (ty + height-1)*pitch;
		end = (PTYPE*)target->pixels + (clip.y-1)*pitch;
	}
	if (!XFLIP) {
		pix = line + tx;
		clipstartpix = line + clip.x;
		clipendpix = clipstartpix + clip.w;
	} else {
		pix = line + tx + width - 1;
		clipstartpix = line + clip.x + clip.w - 1;
		clipendpix = clipstartpix - clip.w;
	}

	// clipstartpix is the first pixel to draw
	// clipendpix is one past the last pixel to draw (in either x direction)

	const int yfactor = yflip ? -1 : 1;
	const int xfactor = XFLIP ? -1 : 1;

	Uint32 amask = target->format->Amask;

	while (line != end) {
		// Fast-forward through the RLE data until we reach clipstartpix

		if (!XFLIP) {
			while (pix < clipstartpix) {
				Uint8 p = *srcdata++;
				int count;
				if (p == transindex)
					count = (*srcdata++) + 1;
				else
					count = 1;
				pix += count;
			}
		} else {
			while (pix > clipstartpix) {
				Uint8 p = *srcdata++;
				int count;
				if (p == transindex)
					count = (*srcdata++) + 1;
				else
					count = 1;
				pix -= count;
			}
		}

		// Blit a line, if it's not vertically clipped

		if ((!yflip && pix >= clipstartline) || (yflip && pix < clipstartline+pitch))
		{
			while ( (!XFLIP && pix < clipendpix) || (XFLIP && pix > clipendpix) )
			{
				Uint8 p = *srcdata++;
				if (p == transindex) {
					int count = (int)(*srcdata++) + 1;
					if (COVER)
						cover.Advance(count);
					
					if (!XFLIP) {
						pix += count;
					} else {
						pix -= count;
					}
				} else {
					Uint8 maskval = *cover;
					if (!COVER || maskval < 0xff) {
						Uint8 r = col[p].r;
						Uint8 g = col[p].g;
						Uint8 b = col[p].b;
						Uint8 a = col[p].a;
						tint(r, g, b, a, flags);
						a = a - maskval;
						blend(*pix, r, g, b, a);
						*pix |= amask; // color keyed surface is 100% opaque
					}
#ifdef HIGHLIGHTCOVER
					else if (COVER) {
						blend(*pix, 255, 255, 255, 255);
					}
#endif
					if (COVER) ++cover;

					if (!XFLIP) {
						pix++;
					} else {
						pix--;
					}
				}
			}
		}

		// We pretend we ended the sprite line here, and move all
		// pointers to the next line

		line += yfactor * pitch;
		pix += yfactor * pitch - xfactor * width;
		clipstartpix += yfactor * pitch;
		clipendpix += yfactor * pitch;
	}
}

// call the BlitSprite{RLE,}_internal instantiation with the specified
// COVER, XFLIP
template<typename PTYPE, typename Tinter, typename Blender>
static void BlitSpritePAL_dispatch2(bool XFLIP,
            SDL_Surface* target,
            const Uint8* srcdata, const Color* col,
            int tx, int ty,
            int width, int height,
            bool yflip,
            const Region& clip,
            int transindex,
            IAlphaIterator* cover,
            unsigned int flags,
            const Tinter& tint, const Blender& blend, PTYPE /*dummy*/ = 0)
{
	static StaticAlphaIterator nomask(0);
	if (!cover && !XFLIP)
		BlitSpriteRLE_internal<PTYPE, false, false, Tinter, Blender>(target,
			srcdata, col, tx, ty, width, height, yflip, clip, transindex, nomask, flags,
			tint, blend);
	else if (!cover && XFLIP)
		BlitSpriteRLE_internal<PTYPE, false, true, Tinter, Blender>(target,
			srcdata, col, tx, ty, width, height, yflip, clip, transindex, nomask, flags,
			tint, blend);
	else if (cover && !XFLIP)
		BlitSpriteRLE_internal<PTYPE, true, false, Tinter, Blender>(target,
			srcdata, col, tx, ty, width, height, yflip, clip, transindex, *cover, flags,
			tint, blend);
	else // if (cover && XFLIP)
		BlitSpriteRLE_internal<PTYPE, true, true, Tinter, Blender>(target,
			srcdata, col, tx, ty, width, height, yflip, clip, transindex, *cover, flags,
			tint, blend);
}

// call the BlitSpritePAL_dispatch2 instantiation with the right pixelformat
// TODO: Hardcoded/non-hardcoded pixelformat
template<typename Tinter, typename Blender>
static void BlitSpritePAL_dispatch(bool XFLIP,
            SDL_Surface* target,
            const Uint8* srcdata, const Color* col,
            int tx, int ty,
            int width, int height,
            bool yflip,
            const Region& clip,
            int transindex,
            IAlphaIterator* cover,
            unsigned int flags,
            const Tinter& tint, const Blender& /*dummy*/)
{
	if (target->format->BytesPerPixel == 4) {
		SRBlender<Uint32, Blender, SRFormat_Hard> blend;
		BlitSpritePAL_dispatch2<Uint32>(XFLIP, target, srcdata, col, tx, ty,
		                                width, height, yflip, clip, transindex,
		                                cover, flags, tint, blend);
	} else {
		SRBlender<Uint16, Blender, SRFormat_Hard> blend;
		BlitSpritePAL_dispatch2<Uint16>(XFLIP, target, srcdata, col, tx, ty,
		                                width, height, yflip, clip, transindex,
		                                cover, flags, tint, blend);
	}
}
