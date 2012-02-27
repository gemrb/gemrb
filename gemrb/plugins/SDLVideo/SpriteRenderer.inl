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


static Region computeClipRect(SDL_Surface* target, const Region* clip,
                              int tx, int ty, int width, int height)
{
	Region r;
	if (clip) {
		r = *clip;
	} else {
		r.x = 0;
		r.y = 0;
		r.w = target->w;
		r.h = target->h;
	}

	// Intersect with SDL clipping rect
	SDL_Rect cliprect;
	SDL_GetClipRect(target, &cliprect);
	if (cliprect.x > r.x) {
		r.w -= (cliprect.x - r.x);
		r.x = cliprect.x;
	}
	if (cliprect.y > r.y) {
		r.h -= (cliprect.y - r.y);
		r.y = cliprect.y;
	}
	if (r.x+r.w > cliprect.x+cliprect.w) {
		r.w = cliprect.x+cliprect.w-r.x;
	}
	if (r.y+r.h > cliprect.y+cliprect.h) {
		r.h = cliprect.y+cliprect.h-r.y;
	}

	// Intersect with actual sprite target rectangle
	if (r.x < tx) {
		r.w -= (tx - r.x);
		r.x = tx;
	}
	if (r.y < ty) {
		r.h -= (ty - r.y);
		r.y = ty;
	}
	if (r.x+r.w > tx+width)
		r.w = tx+width-r.x;
	if (r.y+r.h > ty+height)
		r.h = ty+height-r.y;

	return r;
}


struct SR_TRUE {
	bool operator()() const { return true; }
};
struct SR_FALSE {
	bool operator()() const { return false; }
};


template<typename PTYPE>
struct SRShadow_None {
	bool operator()(PTYPE&, Uint8 p, int&, unsigned int) const { return (p == 1); }
};

template<typename PTYPE>
struct SRShadow_HalfTrans {
	SRShadow_HalfTrans(const SDL_PixelFormat* format, const Color& col)
	{
		shadowcol = (PTYPE)SDL_MapRGBA(format, col.r/2, col.g/2, col.b/2, 0);
		mask =   (0x7F >> format->Rloss) << format->Rshift
				| (0x7F >> format->Gloss) << format->Gshift
				| (0x7F >> format->Bloss) << format->Bshift;
	}

	bool operator()(PTYPE& pix, Uint8 p, int&, unsigned int) const
	{
		if (p == 1) {
			pix = ((pix >> 1)&mask) + shadowcol;
			return true;
		}
		return false;
	}

	PTYPE mask;
	PTYPE shadowcol;
};

template<typename PTYPE>
struct SRShadow_Regular {
	bool operator()(PTYPE&, Uint8, int&, unsigned int) const { return false; }
};

// Conditionally handle halftrans,noshadow,transshadow
template<typename PTYPE>
struct SRShadow_Flags {
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


template<typename PTYPE>
struct SRBlender_NoAlpha {
	void operator()(PTYPE& pix, Uint8 r, Uint8 g, Uint8 b, Uint8) const { assert(false); }
};

template<typename PTYPE>
struct SRBlender_HalfAlpha {
	void operator()(PTYPE& pix, Uint8 r, Uint8 g, Uint8 b, Uint8) const { assert(false); }
};

template<typename PTYPE>
struct SRBlender_Alpha {
	void operator()(PTYPE& pix, Uint8 r, Uint8 g, Uint8 b, Uint8 a) const { assert(false); }
};

// 16 bpp, 565
template<>
struct SRBlender_NoAlpha<Uint16> {
	void operator()(Uint16& pix, Uint8 r, Uint8 g, Uint8 b, Uint8) const {
		pix = ((r >> 3) << 11) |
		      ((g >> 2) <<  5) |
		      ((b >> 3) <<  0);
	}
};

template<>
struct SRBlender_HalfAlpha<Uint16> {
	void operator()(Uint16& pix, Uint8 r, Uint8 g, Uint8 b, Uint8) const {
		// 0x7BEF: the most significant bit of each component cleared
		pix = ((pix >> 1) & 0x7BEF) +
		      (((r << 15) | (g <<  7) | (b >>  1)) & 0x7BEF);
	}
};

template<>
struct SRBlender_Alpha<Uint16> {
	void operator()(Uint16& pix, Uint8 r, Uint8 g, Uint8 b, Uint8 a) const {
		unsigned int dr = 1 + a*(r >> 3) + (255-a)*((pix >> 11) & 0x1F);
		unsigned int dg = 1 + a*(g >> 2) + (255-a)*((pix >>  5) & 0x3F);
		unsigned int db = 1 + a*(b >> 3) + (255-a)*((pix >>  0) & 0x1F);
		r = (dr + (dr>>8)) >> 8;
		g = (dg + (dg>>8)) >> 8;
		b = (db + (db>>8)) >> 8;
		pix = (r << 11) |
		      (g <<  5) |
		      (b <<  0);
	}
};

// 32 bpp, 888
template<>
struct SRBlender_NoAlpha<Uint32> {
	void operator()(Uint32& pix, Uint8 r, Uint8 g, Uint8 b, Uint8) const {
		pix = (r << 16) |
		      (g <<  8) |
		      (b <<  0);
	}
};

template<>
struct SRBlender_HalfAlpha<Uint32> {
	void operator()(Uint32& pix, Uint8 r, Uint8 g, Uint8 b, Uint8) const {
		// 0x7F7F7F: the most significant bit of each component cleared
		pix = ((pix >> 1) & 0x007F7F7F) +
		      (((r << 15) | (g <<  7) | (b >>  1)) & 0x007F7F7F);
	}
};


template<>
struct SRBlender_Alpha<Uint32> {
	void operator()(Uint32& pix, Uint8 r, Uint8 g, Uint8 b, Uint8 a) const {
		unsigned int dr = 1 + a*r + (255-a)*((pix >> 16) & 0xFF);
		unsigned int dg = 1 + a*g + (255-a)*((pix >>  8) & 0xFF);
		unsigned int db = 1 + a*b + (255-a)*((pix >>  0) & 0xFF);
		r = (dr + (dr>>8)) >> 8;
		g = (dg + (dg>>8)) >> 8;
		b = (db + (db>>8)) >> 8;
		pix = (r << 16) |
		      (g <<  8) |
		      (b <<  0);
	}
};


// RLE, palette
template<typename PTYPE, typename Cover, typename XFlip, typename Shadow, typename Tinter, typename Blender>
static void BlitSpriteRLE_internal(SDL_Surface* target,
            const Uint8* srcdata, const Color* col,
            int tx, int ty,
            int width, int height,
            bool yflip,
            Region clip,
            Uint8 transindex,
            const SpriteCover* cover,
            const Sprite2D* spr, unsigned int flags,
            const Shadow& shadow, const Tinter& tint, const Blender& blend,
            Cover COVER = Cover(), const XFlip XFLIP = XFlip(), PTYPE /*dummy*/ = 0)
{
	if (COVER())
		assert(cover);
	assert(spr);

	int pitch = target->pitch / target->format->BytesPerPixel;
	int coverx, covery;
	if (COVER()) {
		coverx = cover->XPos - spr->XPos;
		covery = cover->YPos - spr->YPos;
	}

	// We assume the clipping rectangle is the exact rectangle in which we will
	// paint. This means clip rect <= sprite rect <= cover rect

	assert(clip.w > 0 && clip.h > 0);
	assert(clip.x >= tx);
	assert(clip.y >= ty);
	assert(clip.x + clip.w <= tx + spr->Width);
	assert(clip.y + clip.h <= ty + spr->Height);

	if (COVER()) {
		assert(tx >= tx - coverx);
		assert(ty >= ty - coverx);
		assert(tx + spr->Width <= tx - coverx + cover->Width);
		assert(ty + spr->Height <= ty - covery + cover->Height);
	}


	PTYPE *clipstartpix, *clipendpix;
	PTYPE *clipstartline;

	if (!yflip) {
		clipstartline = (PTYPE*)target->pixels + clip.y*pitch;
	} else {
		clipstartline = (PTYPE*)target->pixels + (clip.y + clip.h - 1)*pitch;
	}


	PTYPE *line, *end, *lineend, *linestart, *pix;
	Uint8 *coverline, *coverpix;
	if (!yflip) {
		line = (PTYPE*)target->pixels + ty*pitch;
		end = (PTYPE*)target->pixels + (clip.y + clip.h)*pitch;
		if (COVER())
			coverline = (Uint8*)cover->pixels + covery*cover->Width;
	} else {
		line = (PTYPE*)target->pixels + (ty + height-1)*pitch;
		end = (PTYPE*)target->pixels + (clip.y-1)*pitch;
		if (COVER())
			coverline = (Uint8*)cover->pixels + (covery+height-1)*cover->Width;
	}
	if (!XFLIP()) {
		linestart = line + tx;
		lineend = line + tx + width;
		clipstartpix = line + clip.x;
		clipendpix = clipstartpix + clip.w;
		if (COVER())
			coverpix = coverline + coverx;
	} else {
		linestart = line + tx + width - 1;
		lineend = line + tx - 1;
		clipstartpix = line + clip.x + clip.w - 1;
		clipendpix = clipstartpix - clip.w;
		if (COVER())
			coverpix = coverline + coverx + width - 1;
	}

	// clipstartpix is the first pixel to draw
	// clipendpix is one past the last pixel to draw (in either x direction)

	pix = linestart;

	const int yfactor = yflip ? -1 : 1;
	const int xfactor = XFLIP() ? -1 : 1;

	while (line != end) {

		// Fast-forward through the RLE data until we reach clipstartpix

		if (!XFLIP()) {
			while (pix < clipstartpix) {
				Uint8 p = *srcdata++;
				int count;
				if (p == transindex)
					count = (*srcdata++) + 1;
				else
					count = 1;
				pix += count;
				if (COVER())
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
				if (COVER())
					coverpix -= count;
			}
		}

		// Blit a line, if it's not vertically clipped

		if ((!yflip && pix >= clipstartline) || (yflip && pix <= clipstartline+pitch))
		{
			while ( (!XFLIP() && pix < clipendpix) || (XFLIP() && pix > clipendpix) )
			{
				//fprintf(stderr, "PIX: %p\n", pix);
				Uint8 p = *srcdata++;
				if (p == transindex) {
					int count = (int)(*srcdata++) + 1;
					if (!XFLIP()) {
						pix += count;
						if (COVER())
							coverpix += count;
					} else {
						pix -= count;
						if (COVER())
							coverpix -= count;
					}
				} else {
					if (!COVER() || !*coverpix) {
						int extra_alpha = 0;
						if (!shadow(*pix, p, extra_alpha, flags)) {
							Uint8 r = col[p].r;
							Uint8 g = col[p].g;
							Uint8 b = col[p].b;
							Uint8 a = col[p].a;
							tint(r, g, b, a, flags);
							blend(*pix, r, g, b, a >> extra_alpha);
						}
					}
#ifdef HIGHLIGHTCOVER
					else if (COVER()) {
						blend(*pix, 255, 255, 255, 255);
					}
#endif

					if (!XFLIP()) {
						pix++;
						if (COVER()) coverpix++;
					} else {
						pix--;
						if (COVER()) coverpix--;
					}
				}
			}
		}

		// We pretend we ended the sprite line here, and move all
		// pointers to the next line

		line += yfactor * pitch;
		pix += yfactor * (pitch - xfactor * width);
		linestart += yfactor * pitch;
		lineend += yfactor * pitch;
		if (COVER())
			coverpix += yfactor * (cover->Width - xfactor * width);
		clipstartpix += yfactor * pitch;
		clipendpix += yfactor * pitch;
	}

}

// non-RLE, palette
template<typename PTYPE, typename Cover, typename XFlip, typename Shadow, typename Tinter, typename Blender>
static void BlitSprite_internal(SDL_Surface* target,
            const Uint8* srcdata, const Color* col,
            int tx, int ty,
            int width, int /*height*/,
            bool yflip,
            Region clip,
            Uint8 transindex,
            const SpriteCover* cover,
            const Sprite2D* spr, unsigned int flags,
            const Shadow& shadow, const Tinter& tint, const Blender& blend,
            Cover COVER = Cover(), const XFlip XFLIP = XFlip(), PTYPE /*dummy*/ = 0)
{
	if (COVER())
		assert(cover);
	assert(spr);

	int pitch = target->pitch / target->format->BytesPerPixel;
	int coverx, covery;
	if (COVER()) {
		coverx = cover->XPos - spr->XPos;
		covery = cover->YPos - spr->YPos;
	}

	// We assume the clipping rectangle is the exact rectangle in which we will
	// paint. This means clip rect <= sprite rect <= cover rect

	assert(clip.w > 0 && clip.h > 0);
	assert(clip.x >= tx);
	assert(clip.y >= ty);
	assert(clip.x + clip.w <= tx + spr->Width);
	assert(clip.y + clip.h <= ty + spr->Height);

	if (COVER()) {
		assert(tx >= tx - coverx);
		assert(ty >= ty - coverx);
		assert(tx + spr->Width <= tx - coverx + cover->Width);
		assert(ty + spr->Height <= ty - covery + cover->Height);
	}


	PTYPE *line, *end;
	Uint8 *coverpix;

	if (!yflip) {
		line = (PTYPE*)target->pixels + clip.y*pitch;
		end = line + clip.h*pitch;
		srcdata += (clip.y - ty)*spr->Width;
		if (COVER())
			coverpix = (Uint8*)cover->pixels + (clip.y - ty + covery)*cover->Width;
	} else {
		line = (PTYPE*)target->pixels + (clip.y + clip.h - 1)*pitch;
		end = line - clip.h*pitch;
		srcdata += (ty + spr->Height - (clip.y + clip.h))*spr->Width;
		if (COVER())
			coverpix = (Uint8*)cover->pixels + (clip.y - ty + clip.h + covery - 1)*cover->Width;
	}

	PTYPE *pix, *endpix;
	if (!XFLIP()) {
		pix = line + clip.x;
		endpix = pix + clip.w;
		srcdata += clip.x - tx;
		if (COVER())
			coverpix += clip.x - tx + coverx;
	} else {
		pix = line + clip.x + clip.w - 1;
		endpix = pix - clip.w;
		srcdata += tx + spr->Width - (clip.x + clip.w);
		if (COVER())
			coverpix += clip.x - tx + clip.w + coverx - 1;
	}

	const int yfactor = yflip ? -1 : 1;
	const int xfactor = XFLIP() ? -1 : 1;

	while (line != end) {
		do {
			Uint8 p = *srcdata++;
			if (p != transindex) {
				if (!COVER() || !*coverpix) {
					int extra_alpha = 0;
					if (!shadow(*pix, p, extra_alpha, flags)) {
						Uint8 r = col[p].r;
						Uint8 g = col[p].g;
						Uint8 b = col[p].b;
						Uint8 a = col[p].a;
						tint(r, g, b, a, flags);
						blend(*pix, r, g, b, a >> extra_alpha);
					}
				}
#ifdef HIGHLIGHTCOVER
				else if (COVER()) {
					blend(*pix, 255, 255, 255, 255);
				}
#endif
			}
			if (!XFLIP()) {
				pix++;
				if (COVER()) coverpix++;
			} else {
				pix--;
				if (COVER()) coverpix--;
			}
		} while (pix != endpix);

		// advance all pointers to the next line
		pix += yfactor * (pitch - xfactor * clip.w);
		endpix += yfactor * pitch;
		line += yfactor * pitch;
		srcdata += (width - clip.w);
		if (COVER())
			coverpix += yfactor * (cover->Width - xfactor * clip.w);
	}

}

// non-RLE, 32 bit RGB
template<typename PTYPE, typename Cover, typename XFlip, typename Tinter, typename Blender>
static void BlitSpriteRGB_internal(SDL_Surface* target,
            const Uint32* srcdata,
            int tx, int ty,
            int width, int /*height*/,
            bool yflip,
            Region clip,
            const SpriteCover* cover,
            const Sprite2D* spr, unsigned int flags,
            const Tinter& tint, const Blender& blend,
            Cover COVER = Cover(), const XFlip XFLIP = XFlip(), PTYPE /*dummy*/ = 0)
{
	if (COVER())
		assert(cover);
	assert(spr);

	int pitch = target->pitch / target->format->BytesPerPixel;
	int coverx, covery;
	if (COVER()) {
		coverx = cover->XPos - spr->XPos;
		covery = cover->YPos - spr->YPos;
	}

	// We assume the clipping rectangle is the exact rectangle in which we will
	// paint. This means clip rect <= sprite rect <= cover rect

	assert(clip.w > 0 && clip.h > 0);
	assert(clip.x >= tx);
	assert(clip.y >= ty);
	assert(clip.x + clip.w <= tx + spr->Width);
	assert(clip.y + clip.h <= ty + spr->Height);

	if (COVER()) {
		assert(tx >= tx - coverx);
		assert(ty >= ty - coverx);
		assert(tx + spr->Width <= tx - coverx + cover->Width);
		assert(ty + spr->Height <= ty - covery + cover->Height);
	}


	PTYPE *line, *end;
	Uint8 *coverpix;

	if (!yflip) {
		line = (PTYPE*)target->pixels + clip.y*pitch;
		end = line + clip.h*pitch;
		srcdata += (clip.y - ty)*spr->Width;
		if (COVER())
			coverpix = (Uint8*)cover->pixels + (clip.y - ty + covery)*cover->Width;
	} else {
		line = (PTYPE*)target->pixels + (clip.y + clip.h - 1)*pitch;
		end = line - clip.h*pitch;
		srcdata += (ty + spr->Height - (clip.y + clip.h))*spr->Width;
		if (COVER())
			coverpix = (Uint8*)cover->pixels + (clip.y - ty + clip.h + covery - 1)*cover->Width;
	}

	PTYPE *pix, *endpix;
	if (!XFLIP()) {
		pix = line + clip.x;
		endpix = pix + clip.w;
		srcdata += clip.x - tx;
		if (COVER())
			coverpix += clip.x - tx + coverx;
	} else {
		pix = line + clip.x + clip.w - 1;
		endpix = pix - clip.w;
		srcdata += tx + spr->Width - (clip.x + clip.w);
		if (COVER())
			coverpix += clip.x - tx + clip.w + coverx - 1;
	}

	const int yfactor = yflip ? -1 : 1;
	const int xfactor = XFLIP() ? -1 : 1;

	while (line != end) {
		do {
			Uint32 p = *srcdata++;
			Uint8 a = (Uint8)(p >> 24);
			if (a != 0) {
				if (!COVER() || !*coverpix) {
					Uint8 r = (Uint8)(p);
					Uint8 g = (Uint8)(p >> 8);
					Uint8 b = (Uint8)(p >> 16);
					tint(r, g, b, a, flags);
					blend(*pix, r, g, b, a);
				}
#ifdef HIGHLIGHTCOVER
				else if (COVER()) {
					blend(*pix, 255, 255, 255, 255);
				}
#endif
			}
			if (!XFLIP()) {
				pix++;
				if (COVER()) coverpix++;
			} else {
				pix--;
				if (COVER()) coverpix--;
			}
		} while (pix != endpix);

		// advance all pointers to the next line
		pix += yfactor * (pitch - xfactor * clip.w);
		endpix += yfactor * pitch;
		line += yfactor * pitch;
		srcdata += (width - clip.w);
		if (COVER())
			coverpix += yfactor * (cover->Width - xfactor * clip.w);
	}

}



// call the BlitSprite{RLE,}_internal instantiation with the specified
// COVER, XFLIP, RLE bools
// TODO: Add PTYPE?
template<typename PTYPE, typename Shadow, typename Tinter, typename Blender>
static void BlitSpriteBAM_dispatch(bool COVER, bool XFLIP,
            SDL_Surface* target,
            const Uint8* srcdata, const Color* col,
            int tx, int ty,
            int width, int height,
            bool yflip,
            const Region& clip,
            Uint8 transindex,
            const SpriteCover* cover,
            const Sprite2D* spr, unsigned int flags,
            const Shadow& shadow, const Tinter& tint, const Blender& blend, PTYPE /*dummy*/ = 0)
{
	assert(spr->BAM);
	Sprite2D_BAM_Internal *data = (Sprite2D_BAM_Internal*)spr->vptr;
	bool RLE = data->RLE;

	if (!COVER && !XFLIP)
		if (RLE)
			BlitSpriteRLE_internal<PTYPE, SR_FALSE, SR_FALSE, Shadow, Tinter, Blender>(target,
			    srcdata, col, tx, ty, width, height, yflip, clip, transindex, cover, spr, flags,
			    shadow, tint, blend);
		else
			BlitSprite_internal<PTYPE, SR_FALSE, SR_FALSE, Shadow, Tinter, Blender>(target,
			    srcdata, col, tx, ty, width, height, yflip, clip, transindex, cover, spr, flags,
			    shadow, tint, blend);
	else if (!COVER && XFLIP)
		if (RLE)
			BlitSpriteRLE_internal<PTYPE, SR_FALSE, SR_TRUE, Shadow, Tinter, Blender>(target,
			    srcdata, col, tx, ty, width, height, yflip, clip, transindex, cover, spr, flags,
			    shadow, tint, blend);
		else
			BlitSprite_internal<PTYPE, SR_FALSE, SR_TRUE, Shadow, Tinter, Blender>(target,
			    srcdata, col, tx, ty, width, height, yflip, clip, transindex, cover, spr, flags,
			    shadow, tint, blend);
	else if (COVER && !XFLIP)
		if (RLE)
			BlitSpriteRLE_internal<PTYPE, SR_TRUE, SR_FALSE, Shadow, Tinter, Blender>(target,
			    srcdata, col, tx, ty, width, height, yflip, clip, transindex, cover, spr, flags,
			    shadow, tint, blend);
		else
			BlitSprite_internal<PTYPE, SR_TRUE, SR_FALSE, Shadow, Tinter, Blender>(target,
			    srcdata, col, tx, ty, width, height, yflip, clip, transindex, cover, spr, flags,
			    shadow, tint, blend);
	else // if (COVER && XFLIP)
		if (RLE)
			BlitSpriteRLE_internal<PTYPE, SR_TRUE, SR_TRUE, Shadow, Tinter, Blender>(target,
			    srcdata, col, tx, ty, width, height, yflip, clip, transindex, cover, spr, flags,
			    shadow, tint, blend);
		else
			BlitSprite_internal<PTYPE, SR_TRUE, SR_TRUE, Shadow, Tinter, Blender>(target,
			    srcdata, col, tx, ty, width, height, yflip, clip, transindex, cover, spr, flags,
			    shadow, tint, blend);
}

// call the BlitSpriteRGB_internal instantiation with the specified
// COVER, XFLIP bools
// TODO: Add PTYPE?
template<typename PTYPE, typename Tinter, typename Blender>
static void BlitSpriteRGB_dispatch(bool COVER, bool XFLIP,
            SDL_Surface* target,
            const Uint32* srcdata,
            int tx, int ty,
            int width, int height,
            bool yflip,
            const Region& clip,
            const SpriteCover* cover,
            const Sprite2D* spr, unsigned int flags,
            const Tinter& tint, const Blender& blend, PTYPE /*dummy*/ = 0)
{
	if (!COVER && !XFLIP)
		BlitSpriteRGB_internal<PTYPE, SR_FALSE, SR_FALSE, Tinter, Blender>(target,
		    srcdata, tx, ty, width, height, yflip, clip, cover, spr, flags,
		    tint, blend);
	else if (!COVER && XFLIP)
		BlitSpriteRGB_internal<PTYPE, SR_FALSE, SR_TRUE, Tinter, Blender>(target,
		    srcdata, tx, ty, width, height, yflip, clip, cover, spr, flags,
		    tint, blend);
	else if (COVER && !XFLIP)
		BlitSpriteRGB_internal<PTYPE, SR_TRUE, SR_FALSE, Tinter, Blender>(target,
		    srcdata, tx, ty, width, height, yflip, clip, cover, spr, flags,
		    tint, blend);
	else // if (COVER && XFLIP)
		BlitSpriteRGB_internal<PTYPE, SR_TRUE, SR_TRUE, Tinter, Blender>(target,
		    srcdata, tx, ty, width, height, yflip, clip, cover, spr, flags,
		    tint, blend);
}


