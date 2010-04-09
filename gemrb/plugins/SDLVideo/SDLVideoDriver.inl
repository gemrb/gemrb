/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2005-2006 The GemRB Project
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

// Input macros:
//
// SRCDATA
//   pointer to the pixel data
// RLE
//   is the sprite RLE-encoded?

// USE_PALETTE
//   does the sprite use a palette? (If defined, define PAL too)
// PAL
//   the palette
// PALETTE_ALPHA
//   use the alpha values from the palette

// BPP16
//   output surface is 16bpp instead of the default 32bpp

// ALREADYCLIPPED
//   the variables clipx, clipy, clipw, cliph have already been set
// FLIP
//   enable code for flipping sprite H/V, through the following two macros:
// HFLIP_CONDITIONAL
//   boolean expression for when to flip the sprite horizontally
// VFLIP_CONDITIONAL
//   boolean expression for when to flip the sprite vertically

// COVER
//   apply wallcovers (The wallcover data is taken from the 'cover' variable)

// TINT
//   enable tinting (The colour to tint with is taken from the 'tint' variable)
// TINT_ALPHA
//   use the alpha component of the tint

// SPECIALPIXEL
//   special code to be inserted right before rendering a pixel

// HIGHLIGHTCOVER
//   (debugging) make covered pixels white instead of black


//#define HIGHLIGHTCOVER

#define TARGET backBuf
#define WIDTH spr->Width
#define HEIGHT spr->Height

#ifdef FLIP

#define XNEG(x) (((x)+xneg)^xneg)
#define YNEG(y) (((y)+yneg)^yneg)

#else

#define XNEG(x) (x)
#define YNEG(y) (y)

#endif


#ifdef BPP16
#define PTYPE Uint16
#define PITCHMULT 2
#define MASK mask16
#else
#define PITCHMULT 4
#define PTYPE Uint32
#define MASK mask32
#endif

#ifdef USE_PALETTE
#define RVALUE(p) (col[(p)].r)
#define GVALUE(p) (col[(p)].g)
#define BVALUE(p) (col[(p)].b)
#define AVALUE(p) (col[(p)].a)
#define ISTRANSPARENT(p) (p == (Uint8)data->transindex)
#define SRCTYPE Uint8
#else
// TODO: These shifts might need fixing if we ever use this functionality
// for sprites not created by SDLVideoDriver::CreateLight
#define RVALUE(p) (p & 0xff)
#define GVALUE(p) ((p >> 8) & 0xff)
#define BVALUE(p) ((p >> 16) & 0xff)
#define AVALUE(p) (p >> 24)
#define ISTRANSPARENT(p) (AVALUE(p) == 0)
#define SRCTYPE Uint32
#endif

#ifndef CUSTOMBLENDING
#define ALPHAADJUST(a) a
#define CUSTOMBLEND(r,g,b)
#endif


#ifdef PALETTE_ALPHA
#define ALPHA
#ifdef TINT_ALPHA
#define ALPHAVALUE(p) (ALPHAADJUST(((AVALUE(p)) * tint.a)>>8))
#else
#define ALPHAVALUE(p) (ALPHAADJUST(AVALUE(p)))
#endif

#else

#ifdef TINT_ALPHA
#define ALPHA
#define ALPHAVALUE(p) (ALPHAADJUST(tint.a))
#else
#undef ALPHA
#define ALPHAVALUE(p) (ALPHAADJUST(255))
#endif

#endif

#ifdef COVER
assert(cover);
#endif

// TODO: preconvert palette to surface-specific color values, where possible

#ifdef ALPHA

#ifdef TINT
#define BLENDPIXEL(target,cr,cg,cb,ca,curval) \
do { \
	if ((ca) != 0) { \
		dR = ((tint.r*(cr)) >> 8); \
		dG = ((tint.g*(cg)) >> 8); \
		dB = ((tint.b*(cb)) >> 8); \
		CUSTOMBLEND(dR,dG,dB); \
		dR = 1 + (ca)*dR + ((((curval)>>rshift)<<rloss)&0xFF)*(255-(ca)); \
 		dR = (dR + (dR >> 8)) >> 8; \
		dG = 1 + (ca)*dG + ((((curval)>>gshift)<<gloss)&0xFF)*(255-(ca)); \
 		dG = (dG + (dG >> 8)) >> 8; \
		dB = 1 + (ca)*dB + ((((curval)>>bshift)<<bloss)&0xFF)*(255-(ca)); \
		dB = (dB + (dB >> 8)) >> 8; \
		target = (PTYPE) ( ((dR) >> rloss) << rshift \
						   | ((dG) >> gloss) << gshift \
						   | ((dB) >> bloss) << bshift); \
	} \
} while (0)
#else
#define BLENDPIXEL(target,cr,cg,cb,ca,curval) \
do { \
	if ((ca) != 0) { \
		dR = (cr); \
		dG = (cg); \
		dB = (cb); \
		CUSTOMBLEND(dR,dG,dB); \
		dR = 1 + (ca)*dR + ((((curval)>>rshift)<<rloss)&0xFF)*(255-(ca)); \
 		dR = (dR + (dR >> 8)) >> 8; \
		dG = 1 + (ca)*dG + ((((curval)>>gshift)<<gloss)&0xFF)*(255-(ca)); \
 		dG = (dG + (dG >> 8)) >> 8; \
		dB = 1 + (ca)*dB + ((((curval)>>bshift)<<bloss)&0xFF)*(255-(ca)); \
		dB = (dB + (dB >> 8)) >> 8; \
		target = (PTYPE) ( ((dR) >> rloss) << rshift \
						   | ((dG) >> gloss) << gshift \
						   | ((dB) >> bloss) << bshift); \
	} \
} while (0)
#endif

#else

#ifdef HALFALPHA

#ifdef TINT

#define BLENDPIXEL(target,cr,cg,cb,ca,curval) target = ((curval >> 1)&MASK) + \
					(((PTYPE)( ((tint.r*(cr)) >> (rloss+8)) << rshift  \
						| ((tint.g*(cg)) >> (gloss+8)) << gshift \
						| ((tint.b*(cb)) >> (bloss+8)) << bshift)) >> 1)

#else

#define BLENDPIXEL(target,cr,cg,cb,ca,curval) target = ((curval >> 1)&MASK) + \
					((((PTYPE)( ((cr) >> rloss) << rshift  \
						| ((cg) >> gloss) << gshift \
						| ((cb) >> bloss) << bshift)) >> 1)&MASK)

#endif

#else

#ifdef CUSTOMBLENDING

#ifdef TINT
#define BLENDPIXEL(target,cr,cg,cb,ca,curval) \
do { \
	dR = ((tint.r*(cr)) >> 8); \
	dG = ((tint.g*(cg)) >> 8); \
	dB = ((tint.b*(cb)) >> 8); \
	CUSTOMBLEND(dR,dG,dB); \
	target = (PTYPE) ( ((dR) >> rloss) << rshift \
					   | ((dG) >> gloss) << gshift \
					   | ((dB) >> bloss) << bshift); \
} while(0)
#else
#define BLENDPIXEL(target,cr,cg,cb,ca,curval) \
do { \
	dR = (cr); \
	dG = (cg); \
	dB = (cb); \
	CUSTOMBLEND(dR,dG,dB); \
	target = (PTYPE) ( ((dR) >> rloss) << rshift \
					   | ((dG) >> gloss) << gshift \
					   | ((dB) >> bloss) << bshift); \
} while(0)
#endif

#else

#ifdef TINT
#define BLENDPIXEL(target,cr,cg,cb,ca,curval) target = (PTYPE)( ((tint.r*(cr)) >> (rloss+8)) << rshift  \
								   | ((tint.g*(cg)) >> (gloss+8)) << gshift \
								   | ((tint.b*(cb)) >> (bloss+8)) << bshift) 
#else
#define BLENDPIXEL(target,cr,cg,cb,ca,curval) target = (PTYPE)( ((cr) >> rloss) << rshift  \
								   | ((cg) >> gloss) << gshift \
								   | ((cb) >> bloss) << bshift)
#endif
#endif
#endif
#endif

do {
#ifdef FLIP
	const int xneg = (HFLIP_CONDITIONAL)?-1:0;
	const int yneg = (VFLIP_CONDITIONAL)?-1:0;
#else
	const int xneg = 0;
	const int yneg = 0;
#endif

	const int rloss = (TARGET)->format->Rloss;
	const int gloss = (TARGET)->format->Gloss;
	const int bloss = (TARGET)->format->Bloss;
	const int rshift = (TARGET)->format->Rshift;
	const int gshift = (TARGET)->format->Gshift;
	const int bshift = (TARGET)->format->Bshift;
#ifdef USE_PALETTE
	const Color* const col = (PAL)->col;
#endif
	const SRCTYPE* srcdata = (SRCDATA);

#if (defined(ALPHA) || defined(CUSTOMBLENDING))
	unsigned int dR;
	unsigned int dG;
	unsigned int dB;
#endif

#ifndef ALREADYCLIPPED
	int clipx, clipy, clipw, cliph;
	if (clip) {
		clipx = clip->x;
		clipy = clip->y;
		clipw = clip->w;
		cliph = clip->h;
	} else {
		clipx = 0;
		clipy = 0;
		clipw = (TARGET)->w;
		cliph = (TARGET)->h;
	}
	SDL_Rect cliprect;
	SDL_GetClipRect((TARGET), &cliprect);
	if (cliprect.x > clipx) {
		clipw -= (cliprect.x - clipx);
		clipx = cliprect.x;
	}
	if (cliprect.y > clipy) {
		cliph -= (cliprect.y - clipy);
		clipy = cliprect.y;
	}
	if (clipx+clipw > cliprect.x+cliprect.w) {
		clipw = cliprect.x+cliprect.w-clipx;
	}
	if (clipy+cliph > cliprect.y+cliprect.h) {
		cliph = cliprect.y+cliprect.h-clipy;
	}
#endif

#ifdef USE_PALETTE
	if (RLE) {
#else
	if (false) {
#endif

		PTYPE* line = (PTYPE*)(TARGET)->pixels +
			(ty - yneg*((HEIGHT)-1))*(TARGET)->pitch/(PITCHMULT);
		PTYPE* end = line + YNEG(HEIGHT)*(TARGET)->pitch/(PITCHMULT);
		PTYPE* clipstartline = (PTYPE*)(TARGET)->pixels
			+ clipy*((TARGET)->pitch)/PITCHMULT;
		PTYPE* clipendline = clipstartline + cliph*((TARGET)->pitch)/PITCHMULT;
#ifdef COVER
		Uint8* coverline = (Uint8*)cover->pixels + ((COVERY) - yneg*((HEIGHT)-1))*cover->Width;
#endif
		
		if (yneg) {
			if (end < clipstartline)
				end = clipstartline - ((TARGET)->pitch)/PITCHMULT;
		} else {
			if (end > clipendline)
				end = clipendline;
		}
		
		int translength = 0;
		for (; YNEG((int)(end - line)) > 0; line += YNEG((TARGET)->pitch/(PITCHMULT)))
		{
			PTYPE* pix = line + tx + translength - xneg*((WIDTH)-1);
			PTYPE* endpix = line + tx - xneg*((WIDTH)-1) + XNEG(WIDTH);
			PTYPE* clipstartpix = line + clipx;
			PTYPE* clipendpix = clipstartpix + clipw;
#ifdef COVER
			Uint8* coverpix = coverline + (COVERX) + translength - xneg*((WIDTH)-1);
#endif
			if (yneg) {
				if (line >= clipendline) clipstartpix = clipendpix;
			} else {
				if (line < clipstartline) clipstartpix = clipendpix;
			}
			while (XNEG((int)(endpix - pix)) > 0)
			{
				Uint8 p = *srcdata++;
				if (ISTRANSPARENT(p)) {
					int count = XNEG((*srcdata++) + 1);
					pix += count;
#ifdef COVER
					coverpix += count;
#endif
				} else {
					if (pix >= clipstartpix && pix < clipendpix) {
#ifdef COVER
						if (!*coverpix)
#endif
						{
							SPECIALPIXEL {
								BLENDPIXEL(*pix, (RVALUE(p)), (GVALUE(p)), (BVALUE(p)), (ALPHAVALUE(p)), *pix);
							}
						}
#if defined(COVER) && defined(HIGHLIGHTCOVER)
						else {
							BLENDPIXEL(*pix, 255, 255, 255, 255, *pix);
						}
#endif
					}
					pix += XNEG(1);
#ifdef COVER
					coverpix += XNEG(1);
#endif
				}
			}
			translength = pix - endpix;
#ifdef COVER
			coverline += YNEG(cover->Width);
#endif
		}
	} else {
#ifdef COVER
		Uint8* coverline = (Uint8*)cover->pixels + ((COVERY) - yneg*((HEIGHT)-1))*cover->Width;
#endif
		int starty, endy;

		if (!yneg) {
			starty = ty;
			if (clipy > starty) starty = clipy;
			endy = ty + (HEIGHT);
			if (clipy+cliph < endy) endy = clipy+cliph;

			if (starty >= endy) break;

			// skip clipped lines at start
			srcdata += (starty - ty) * (WIDTH);
#ifdef COVER
			coverline += (starty - ty) * cover->Width;
#endif
		} else {
			starty = ty + (HEIGHT) - 1;
			if (clipy+cliph <= starty) starty = clipy+cliph-1;
			endy = ty - 1;
			if (clipy-1 > endy) endy = clipy-1;

			if (starty <= endy) break;

			// skip clipped lines at start
			srcdata += (ty + (HEIGHT) - 1 - starty) * (WIDTH);
#ifdef COVER
			coverline -= (ty + (HEIGHT) - 1 - starty) * cover->Width;
#endif
		}

		int startx, endx;
		int prelineskip = 0;
		int postlineskip = 0;

		if (!xneg) {
			startx = tx;
			if (clipx > startx) startx = clipx;
			endx = tx + (WIDTH);
			if (clipx+clipw < endx) endx = clipx+cliph;

			if (startx >= endx) break;

			prelineskip = startx - tx;
			postlineskip = tx + (WIDTH) - endx;
		} else {
			startx = tx + (WIDTH) - 1;
			if (clipx+clipw <= startx) startx = clipx+clipw-1;
			endx = tx - 1;
			if (clipx-1 > endx) endx = clipx-1;

			if (startx <= endx) break;

			prelineskip = (tx + (WIDTH) - 1) - startx;
			postlineskip = endx - (tx-1);
		}


		PTYPE* line = (PTYPE*)(TARGET)->pixels +
			starty*(TARGET)->pitch/(PITCHMULT);
		PTYPE* endline = (PTYPE*)(TARGET)->pixels +
			endy*(TARGET)->pitch/(PITCHMULT);

		while (line != endline) {
			PTYPE* pix = line + startx;
			PTYPE* endpix = line + endx;
#ifdef COVER
			Uint8* coverpix = coverline + (COVERX) + XNEG(prelineskip) - xneg*((WIDTH)-1);
#endif
			srcdata += prelineskip;

			while (pix != endpix)
			{
				SRCTYPE p = *srcdata++;
				if (!(ISTRANSPARENT(p))) {
#ifdef COVER
					if (!*coverpix)
#endif
					{
						SPECIALPIXEL {
							BLENDPIXEL(*pix, (RVALUE(p)), (GVALUE(p)), (BVALUE(p)), ( (ALPHAVALUE(p))), *pix);
						}
					}
#if defined(COVER) && defined(HIGHLIGHTCOVER)
					else {
						BLENDPIXEL(*pix, 255, 255, 255, 255, *pix);
					}
#endif
				}
				pix += XNEG(1);
#ifdef COVER
				coverpix += XNEG(1);
#endif
			}

			srcdata += postlineskip;

			line += YNEG((TARGET)->pitch/(PITCHMULT));
#ifdef COVER
			coverline += YNEG(cover->Width);
#endif			
		}
	}

} while(0);



#undef XNEG
#undef YNEG
#undef PITCHMULT
#undef PTYPE
#undef BLENDPIXEL
#undef TARGET
#undef WIDTH
#undef HEIGHT
#undef ALPHA
#undef ALPHAVALUE
#undef HIGHLIGHTCOVER
#undef MASK
#undef RVALUE
#undef GVALUE
#undef BVALUE
#undef AVALUE
#undef ISTRANSPARENT
#undef SRCTYPE

#ifndef CUSTOMBLENDING
#undef ALPHAADJUST
#undef CUSTOMBLEND
#endif
