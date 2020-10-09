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

template<typename PTYPE, typename BLENDER>
struct SRBlender {
	SRBlender(SDL_PixelFormat* format);

	void operator()(PTYPE& /*pix*/, Uint8 /*r*/, Uint8 /*g*/, Uint8 /*b*/, Uint8 /*a*/) const;
};

template<typename BLENDER>
struct SRBlender<Uint16, BLENDER> {
	uint8_t RLOSS;
	uint8_t GLOSS;
	uint8_t BLOSS;

	uint8_t RSHIFT;
	uint8_t GSHIFT;
	uint8_t BSHIFT;
	
	Uint16 AMASK;
	Uint16 halfmask;

	SRBlender(SDL_PixelFormat* fmt) {
		RLOSS = fmt->Rloss;
		GLOSS = fmt->Gloss;
		BLOSS = fmt->Bloss;
		
		RSHIFT = fmt->Rshift;
		GSHIFT = fmt->Gshift;
		BSHIFT = fmt->Bshift;
		
		AMASK = fmt->Amask;
		
		halfmask = ((0xFFU >> (RLOSS + 1)) << RSHIFT) | ((0xFFU >> (GLOSS + 1)) << GSHIFT) | ((0xFFU >> (BLOSS + 1)) << BSHIFT);
	}
	
	void operator()(Uint16& /*pix*/, Uint8 /*r*/, Uint8 /*g*/, Uint8 /*b*/, Uint8 /*a*/) const;
};

template<typename BLENDER>
struct SRBlender<Uint32, BLENDER> {
	uint8_t RSHIFT;
	uint8_t GSHIFT;
	uint8_t BSHIFT;
	
	Uint32 AMASK;
	Uint32 halfmask;

	SRBlender(SDL_PixelFormat* fmt) {
		RSHIFT = fmt->Rshift;
		GSHIFT = fmt->Gshift;
		BSHIFT = fmt->Bshift;
		
		AMASK = fmt->Amask;
		
		halfmask = ((0xFFU >> 1) << RSHIFT) | ((0xFFU >> 1) << GSHIFT) | ((0xFFU >> 1) << BSHIFT);
	}
	
	void operator()(Uint32& /*pix*/, Uint8 /*r*/, Uint8 /*g*/, Uint8 /*b*/, Uint8 /*a*/) const;
};

template<> // 16 bpp, 565
void SRBlender<Uint16, SRBlender_NoAlpha>::operator()(Uint16& pix, Uint8 r, Uint8 g, Uint8 b, Uint8) const {
	pix = ((r >> RLOSS) << RSHIFT) |
		  ((g >> GLOSS) << GSHIFT) |
		  ((b >> BLOSS) << BSHIFT);
}

template<> // 16 bpp, 565
void SRBlender<Uint16, SRBlender_HalfAlpha>::operator()(Uint16& pix, Uint8 r, Uint8 g, Uint8 b, Uint8) const {
	pix = ((pix >> 1) & halfmask) +
	((((r >> (RLOSS + 1)) << RSHIFT) | ((g >> (GLOSS + 1)) << GSHIFT) | ((b >> (BLOSS + 1)) << BSHIFT)) & halfmask);
}

template<> // 16 bpp, 565
void SRBlender<Uint16, SRBlender_Alpha>::operator()(Uint16& pix, Uint8 r, Uint8 g, Uint8 b, Uint8 a) const {
	unsigned int dr = 1 + a*(r >> RLOSS) + (255-a)*((pix >> RSHIFT) & ((1 << (8-RLOSS))-1));
	unsigned int dg = 1 + a*(g >> GLOSS) + (255-a)*((pix >> GSHIFT) & ((1 << (8-GLOSS))-1));
	unsigned int db = 1 + a*(b >> BLOSS) + (255-a)*((pix >> BSHIFT) & ((1 << (8-BLOSS))-1));

	r = (dr + (dr>>8)) >> 8;
	g = (dg + (dg>>8)) >> 8;
	b = (db + (db>>8)) >> 8;
	pix = (r << RSHIFT) |
		  (g << GSHIFT) |
		  (b << BSHIFT);
}

template<> // 32 bpp, 888
void SRBlender<Uint32, SRBlender_NoAlpha>::operator()(Uint32& pix, Uint8 r, Uint8 g, Uint8 b, Uint8) const {
	pix = (r << RSHIFT) |
	(g << GSHIFT) |
	(b << BSHIFT);
}

template<> // 32 bpp, 888
void SRBlender<Uint32, SRBlender_HalfAlpha>::operator()(Uint32& pix, Uint8 r, Uint8 g, Uint8 b, Uint8) const {
	pix = ((pix >> 1) & halfmask) +
	((((r << RSHIFT) | (g << GSHIFT) | (b << BSHIFT)) >> 1) & halfmask);
}

template<> // 32 bpp, 888
void SRBlender<Uint32, SRBlender_Alpha>::operator()(Uint32& pix, Uint8 r, Uint8 g, Uint8 b, Uint8 a) const {
	unsigned int dr = 1 + a*r + (255-a)*((pix >> RSHIFT) & 0xFF);
	unsigned int dg = 1 + a*g + (255-a)*((pix >> GSHIFT) & 0xFF);
	unsigned int db = 1 + a*b + (255-a)*((pix >> BSHIFT) & 0xFF);
	r = (dr + (dr>>8)) >> 8;
	g = (dg + (dg>>8)) >> 8;
	b = (db + (db>>8)) >> 8;
	pix = (r << RSHIFT) |
		  (g << GSHIFT) |
		  (b << BSHIFT);
}

// these always change together
#define ADVANCE_ITERATORS(count) dest.Advance(count); cover.Advance(count);

template<typename PTYPE, typename Tinter, typename Blender>
void TintedBlend(SDLPixelIterator& dest, Uint8 alpha,
				 Color col, uint32_t flags,
				 const Tinter& tint, const Blender& blend)
{
	PTYPE& pix = (PTYPE&)*dest;

	tint(col.r, col.g, col.b, col.a, flags);
	col.a = col.a - alpha; // FIXME: seems like this should be handled by something else, we shouldn't need the 'alpha' param
	blend(pix, col.r, col.g, col.b, col.a);
	
	// FIXME: we should probably address this in the blenders instead
	pix |= blend.AMASK; // color keyed surface is 100% opaque
}

template<typename PTYPE, typename Tinter, typename Blender>
void MaskedTintedBlend(SDLPixelIterator& dest, Uint8 maskval,
					   const Color& col, uint32_t flags,
					   const Tinter& tint, const Blender& blend)
{
	if (maskval < 0xff) {
		if ((flags & BLIT_STENCIL_DITHER) && maskval == 128) {
			const Point& pos = dest.Position();
			if (pos.y % 2 == 0) {
				maskval = (pos.x % 2) ? 0xC0 : 0x80;
			} else {
				maskval = (pos.x % 2) ? 0x80 : 0xC0;
			}
		}
		
		TintedBlend<PTYPE>(dest, maskval, col, flags, tint, blend);
	}
}

// use this when you need to copy the entire source sprite
template<typename PTYPE, typename Tinter, typename Blender>
static void BlitSpriteRLE_Total(const Uint8* rledata,
								const Color* pal, Uint8 transindex,
								SDLPixelIterator& dest, IAlphaIterator& cover,
								uint32_t flags, const Tinter& tint, const Blender& blend)
{
	SDLPixelIterator end = SDLPixelIterator::end(dest);
	while (dest != end) {
		Uint8 p = *rledata++;
		if (p == transindex) {
			int count = (*rledata++) + 1;
			ADVANCE_ITERATORS(count);
			continue;
		}
		
		MaskedTintedBlend<PTYPE>(dest, *cover, pal[p], flags, tint, blend);
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
	const int endx = srect.x + srect.w;
	const int endy = srect.y + srect.h;
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
						MaskedTintedBlend<PTYPE>(dest, *cover, pal[p], flags, tint, blend);
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
static void BlitSpriteRLE(Holder<Sprite2D> spr, const Region& srect,
						  SDL_Surface* dst, const Region& drect,
						  IAlphaIterator* cover,
						  uint32_t flags, const Tinter& tint)
{
	assert(spr->BAM);

	if (srect.Dimensions().IsEmpty())
		return;

	if (drect.Dimensions().IsEmpty())
		return;

	PaletteHolder palette = spr->GetPalette();
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
			SRBlender<Uint32, Blender> blend(dstit.format);
			if (partial) {
				BlitSpriteRLE_Partial<Uint32>(rledata, spr->Frame.w, srect, palette->col, ck, dstit, *cover, flags, tint, blend);
			} else {
				BlitSpriteRLE_Total<Uint32>(rledata, palette->col, ck, dstit, *cover, flags, tint, blend);
			}
			break;
		}
		case 2:
		{
			SRBlender<Uint16, Blender> blend(dstit.format);
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
}

#undef ADVANCE_ITERATORS
