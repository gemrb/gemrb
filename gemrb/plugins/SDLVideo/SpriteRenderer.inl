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

// these always change together
#define ADVANCE_ITERATORS(count) dest.Advance(count); cover.Advance(count);

template<typename PTYPE, typename Tinter, typename Blender>
void MaskedTintedBlend(PTYPE& dest, Uint8 maskval, Uint8 amask,
					   Color col, uint32_t flags,
					   const Tinter& tint, const Blender& blend)
{
	if (maskval < 0xff) {
		tint(col.r, col.g, col.b, col.a, flags);
		col.a = col.a - maskval;
		blend(dest, col.r, col.g, col.b, col.a);
		// FIXME: we should probably address this in the blenders instead
		dest |= amask; // color keyed surface is 100% opaque
	}
}

// use this when you need to copy the entire source sprite
template<typename PTYPE, typename Tinter, typename Blender>
static void BlitSpriteRLE_Total(const Uint8* rledata,
								const Color* pal, Uint8 transindex,
								SDLPixelIterator& dest, IAlphaIterator& cover,
								uint32_t flags, const Tinter& tint, const Blender& blend)
{
	assert(pal);

	SDLPixelIterator end = SDLPixelIterator::end(dest);
	while (dest != end) {
		Uint8 p = *rledata++;
		if (p == transindex) {
			int count = (*rledata++) + 1;
			ADVANCE_ITERATORS(count);
			continue;
		}
		
		MaskedTintedBlend<PTYPE>((PTYPE&)*dest, *cover, dest.format->Amask, pal[p], flags, tint, blend);
		ADVANCE_ITERATORS(1);
	}
}

// use this when you need a partial copy of the source sprite
template<typename PTYPE, typename Tinter, typename Blender>
static void BlitSpriteRLE_Partial(const Uint8* rledata, const int pitch, const Region& srect,
								  const Color* pal, Uint8 transindex,
								  SDLPixelIterator& dest, IAlphaIterator& cover,
								  uint32_t flags, const Tinter& tint, const Blender& blend)
{
	const int endy = srect.y + srect.h;

	int count = srect.y * pitch;
	while (count > 0) {
		Uint8 p = *rledata++;
		if (p == transindex) {
			count -= (*rledata++) + 1;
		} else {
			--count;
		}
	}

	int transQueue = -count;
	int endx = srect.x + srect.w;
	for (int y = srect.y; y < endy; ++y) {
		// We assume 'dest' and 'cover' are setup appropriately to accept 'srect.size'
		
		if (transQueue >= pitch) {
			transQueue -= pitch;
			ADVANCE_ITERATORS(srect.w);
			continue;
		}
		
		for (int x = 0; x < pitch;) {
			assert(transQueue >= 0);

			if (transQueue > 0) {
				int segment = 0;
				bool advance = false;
				if (x < srect.x) {
					segment = srect.x - x;
				} else if (x >= endx) {
					segment = pitch - x;
				} else {
					segment = endx - x;
					advance = true;
				}
				assert(segment > 0);
				
				if (transQueue < segment) {
					if (advance) {
						ADVANCE_ITERATORS(transQueue);
					}
					
					x += transQueue;
					transQueue = 0;
				} else {
					if (advance) {
						ADVANCE_ITERATORS(segment);
					}

					transQueue -= segment;
					x += segment;
				}
			} else {
				Uint8 p = *rledata++;
				if (p == transindex) {
					transQueue = (*rledata++) + 1;
				} else {
					if (x >= srect.x && x < endx) {
						MaskedTintedBlend<PTYPE>((PTYPE&)*dest, *cover, dest.format->Amask, pal[p], flags, tint, blend);
						ADVANCE_ITERATORS(1);
					}
					++x;
				}
			}
			
			assert(x <= pitch);
		}
	}
}

template<typename Blender, typename Tinter>
static void BlitSpriteRLE(const Sprite2D* spr, const Region& srect,
						  SDL_Surface* dst, const Region& drect,
						  IAlphaIterator* cover,
						  uint32_t flags, const Tinter& tint)
{
	assert(spr->BAM);

	if (srect.Dimensions().IsEmpty())
		return;
	
	if (drect.Dimensions().IsEmpty())
		return;
	
	Palette* palette = spr->GetPalette();
	const Uint8* rledata = (const Uint8*)spr->LockSprite();
	uint8_t ck = spr->GetColorKey();
	
	bool partial = spr->Frame.Dimensions() != srect.Dimensions();
		
	IPixelIterator::Direction xdir = (flags&BLIT_MIRRORX) ? IPixelIterator::Reverse : IPixelIterator::Forward;
	IPixelIterator::Direction ydir = (flags&BLIT_MIRRORY) ? IPixelIterator::Reverse : IPixelIterator::Forward;
	
	SDLPixelIterator dstit = SDLPixelIterator(xdir, ydir, RectFromRegion(drect), dst);
	
	static StaticAlphaIterator nomask(0);
	if (cover == nullptr) {
		cover = &nomask;
	}
	
	switch (dstit.format->BytesPerPixel) {
		case 4:
		{
			SRBlender<Uint32, Blender, SRFormat_Hard> blend;
			if (partial) {
				BlitSpriteRLE_Partial<Uint32>(rledata, spr->Frame.w, srect, palette->col, ck, dstit, *cover, flags, tint, blend);
			} else {
				BlitSpriteRLE_Total<Uint32>(rledata, palette->col, ck, dstit, *cover, flags, tint, blend);
			}
			break;
		}
		case 2:
		{
			SRBlender<Uint16, Blender, SRFormat_Hard> blend;
			if (partial) {
				BlitSpriteRLE_Partial<Uint16>(rledata, spr->Frame.w, srect, palette->col, ck, dstit, *cover, flags, tint, blend);
			} else {
				BlitSpriteRLE_Total<Uint16>(rledata, palette->col, ck, dstit, *cover, flags, tint, blend);
			}
			break;
		}
		default:
			Log(ERROR, "SpriteRenderer", "Invalid Bpp");
			break;
	}
	
	palette->release();
}

#undef ADVANCE_ITERATORS
