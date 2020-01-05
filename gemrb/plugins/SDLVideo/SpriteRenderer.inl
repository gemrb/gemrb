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

struct SRShadow_NOP {
	template<typename PTYPE>
	bool operator()(PTYPE&, Uint8, int&, unsigned int) const { return false; }
};

struct SRShadow_None {
	template<typename PTYPE>
	bool operator()(PTYPE&, Uint8 p, int&, unsigned int) const { return (p == 1); }
};

struct SRShadow_HalfTrans {
	SRShadow_HalfTrans(const SDL_PixelFormat* format, const Color& col)
	{
		shadowcol = (Uint32)SDL_MapRGBA(format, col.r/2, col.g/2, col.b/2, 0);
		mask =   (0x7F >> format->Rloss) << format->Rshift
				| (0x7F >> format->Gloss) << format->Gshift
				| (0x7F >> format->Bloss) << format->Bshift;
	}

	template<typename PTYPE>
	bool operator()(PTYPE& pix, Uint8 p, int&, unsigned int) const
	{
		if (p == 1) {
			pix = ((pix >> 1)&mask) + shadowcol;
			return true;
		}
		return false;
	}

	Uint32 mask;
	Uint32 shadowcol;
};

struct SRShadow_Regular {
	template<typename PTYPE>
	bool operator()(PTYPE&, Uint8, int&, unsigned int) const { return false; }
};

// Conditionally handle halftrans,noshadow,transshadow
struct SRShadow_Flags {
	template<typename PTYPE>
	bool operator()(PTYPE&, Uint8 p, int& a, unsigned int flags) const {
		if ((flags & BLIT_HALFTRANS) || ((p == 1) && (flags & BLIT_TRANSSHADOW)))
			a = 1;
		if (p == 1 && (flags & BLIT_NOSHADOW))
			return true;
		return false;
	}
};


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
template<typename PTYPE, bool COVER, bool XFLIP, typename Shadow, typename Tinter, typename Blender>
static void BlitSpriteRLE_internal(SDL_Surface* target,
            const Uint8* srcdata, const Color* col,
            int tx, int ty,
            int width, int height,
            bool yflip,
            Region clip,
            Uint8 transindex,
            SDL_Surface* cover,
            const Sprite2D* spr, unsigned int flags,
            const Shadow& shadow, const Tinter& tint, const Blender& blend, PTYPE /*dummy*/ = 0)
{
	Uint16 coverpitch = 0;
	Uint32 covermask = 0;
	Uint8 covershift = 0;

	if (COVER) {
		assert(cover);
		SDL_PixelFormat* fmt = cover->format;
		assert(fmt->BytesPerPixel == 4);
		SDL_LockSurface(cover);
		coverpitch = cover->pitch / 4;

		if (flags&BLIT_STENCIL_RED) {
			covermask = fmt->Rmask;
			covershift = fmt->Rshift;
		} else if (flags&BLIT_STENCIL_GREEN) {
			covermask = fmt->Gmask;
			covershift = fmt->Gshift;
		} else if (flags&BLIT_STENCIL_BLUE) {
			covermask = fmt->Bmask;
			covershift = fmt->Bshift;
		} else {
			covermask = fmt->Amask;
			covershift = fmt->Ashift;
		}
	}
	assert(spr);

	int pitch = target->pitch / target->format->BytesPerPixel;
	int coverx = tx;
	int covery = ty;

	// We assume the clipping rectangle is the exact rectangle in which we will
	// paint. This means clip rect <= sprite rect <= cover rect

	assert(clip.w > 0 && clip.h > 0);
	assert(clip.x >= tx);
	assert(clip.y >= ty);
	assert(clip.x + clip.w <= tx + spr->Frame.w);
	assert(clip.y + clip.h <= ty + spr->Frame.h);

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

	// FIXME: we have a lot of unnecissary branching with COVER
	// this is because for some reason we chose to mirror the destination rather than the source
	// if we rewrote this to mirror jsut the source the code would be more simple and probably faster
	PTYPE *line, *end, *pix;
	Uint32 *coverline, *coverpix = NULL;
	if (!yflip) {
		line = (PTYPE*)target->pixels + ty*pitch;
		end = (PTYPE*)target->pixels + (clip.y + clip.h)*pitch;
		if (COVER)
			coverline = (Uint32*)cover->pixels + covery * coverpitch;
	} else {
		line = (PTYPE*)target->pixels + (ty + height-1)*pitch;
		end = (PTYPE*)target->pixels + (clip.y-1)*pitch;
		if (COVER)
			coverline = (Uint32*)cover->pixels + (covery+height-1) * coverpitch;
	}
	if (!XFLIP) {
		pix = line + tx;
		clipstartpix = line + clip.x;
		clipendpix = clipstartpix + clip.w;
		if (COVER)
			coverpix = coverline + coverx;
	} else {
		pix = line + tx + width - 1;
		clipstartpix = line + clip.x + clip.w - 1;
		clipendpix = clipstartpix - clip.w;
		if (COVER)
			coverpix = coverline + coverx + width - 1;
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
				if (COVER)
					coverpix += count;
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
				if (COVER)
					coverpix -= count;
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
					if (!XFLIP) {
						pix += count;
						if (COVER)
							coverpix += count;
					} else {
						pix -= count;
						if (COVER)
							coverpix -= count;
					}
				} else {
					Uint8 maskval = (coverpix) ? (((*coverpix)&covermask) >> covershift) : 0;
					if (!COVER || (coverpix && maskval < 0xff)) {
						int extra_alpha = 0;
						if (!shadow(*pix, p, extra_alpha, flags)) {
							Uint8 r = col[p].r;
							Uint8 g = col[p].g;
							Uint8 b = col[p].b;
							Uint8 a = col[p].a;
							tint(r, g, b, a, flags);
							a = a - maskval;
							blend(*pix, r, g, b, a >> extra_alpha);
							*pix |= amask; // color keyed surface is 100% opaque
						}
					}
#ifdef HIGHLIGHTCOVER
					else if (COVER) {
						blend(*pix, 255, 255, 255, 255);
					}
#endif

					if (!XFLIP) {
						pix++;
						if (COVER) coverpix++;
					} else {
						pix--;
						if (COVER) coverpix--;
					}
				}
			}
		}

		// We pretend we ended the sprite line here, and move all
		// pointers to the next line

		line += yfactor * pitch;
		pix += yfactor * pitch - xfactor * width;
		if (COVER)
			coverpix += yfactor * coverpitch - xfactor * width;
		clipstartpix += yfactor * pitch;
		clipendpix += yfactor * pitch;
	}
	if (COVER)
		SDL_UnlockSurface(cover);
}

// call the BlitSprite{RLE,}_internal instantiation with the specified
// COVER, XFLIP
template<typename PTYPE, typename Shadow, typename Tinter, typename Blender>
static void BlitSpritePAL_dispatch2(bool COVER, bool XFLIP,
            SDL_Surface* target,
            const Uint8* srcdata, const Color* col,
            int tx, int ty,
            int width, int height,
            bool yflip,
            const Region& clip,
            int transindex,
            SDL_Surface* cover,
            const Sprite2D* spr, unsigned int flags,
            const Shadow& shadow, const Tinter& tint, const Blender& blend, PTYPE /*dummy*/ = 0)
{
	if (!COVER && !XFLIP)
		BlitSpriteRLE_internal<PTYPE, false, false, Shadow, Tinter, Blender>(target,
			srcdata, col, tx, ty, width, height, yflip, clip, transindex, cover, spr, flags,
			shadow, tint, blend);
	else if (!COVER && XFLIP)
		BlitSpriteRLE_internal<PTYPE, false, true, Shadow, Tinter, Blender>(target,
			srcdata, col, tx, ty, width, height, yflip, clip, transindex, cover, spr, flags,
			shadow, tint, blend);
	else if (COVER && !XFLIP)
		BlitSpriteRLE_internal<PTYPE, true, false, Shadow, Tinter, Blender>(target,
			srcdata, col, tx, ty, width, height, yflip, clip, transindex, cover, spr, flags,
			shadow, tint, blend);
	else // if (COVER && XFLIP)
		BlitSpriteRLE_internal<PTYPE, true, true, Shadow, Tinter, Blender>(target,
			srcdata, col, tx, ty, width, height, yflip, clip, transindex, cover, spr, flags,
			shadow, tint, blend);
}

// call the BlitSpritePAL_dispatch2 instantiation with the right pixelformat
// TODO: Hardcoded/non-hardcoded pixelformat
template<typename Shadow, typename Tinter, typename Blender>
static void BlitSpritePAL_dispatch(bool COVER, bool XFLIP,
            SDL_Surface* target,
            const Uint8* srcdata, const Color* col,
            int tx, int ty,
            int width, int height,
            bool yflip,
            const Region& clip,
            int transindex,
            SDL_Surface* cover,
            const Sprite2D* spr, unsigned int flags,
            const Shadow& shadow, const Tinter& tint, const Blender& /*dummy*/)
{
	if (target->format->BytesPerPixel == 4) {
		SRBlender<Uint32, Blender, SRFormat_Hard> blend;
		BlitSpritePAL_dispatch2<Uint32>(COVER, XFLIP, target, srcdata, col, tx, ty,
		                                width, height, yflip, clip, transindex,
		                                cover, spr, flags, shadow, tint, blend);
	} else {
		SRBlender<Uint16, Blender, SRFormat_Hard> blend;
		BlitSpritePAL_dispatch2<Uint16>(COVER, XFLIP, target, srcdata, col, tx, ty,
		                                width, height, yflip, clip, transindex,
		                                cover, spr, flags, shadow, tint, blend);
	}
}
