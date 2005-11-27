/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2005 The GemRB Project
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/SDLVideo/SDLVideoDriver.inl,v 1.2 2005/11/27 19:32:29 wjpalenstijn Exp $
 *
 */


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
#else
#define PITCHMULT 4
#define PTYPE Uint32
#endif

#ifdef PALETTE_ALPHA
#define ALPHA
#ifdef TINT_ALPHA
#define ALPHAVALUE (((PAL)[p].a * tint.a)>>8)
#else
#define ALPHAVALUE (PAL)[p].a
#endif

#else

#ifdef TINT_ALPHA
#define ALPHA
#define ALPHAVALUE tint.a
#else
#undef ALPHA
#define ALPHAVALUE 0
#endif

#endif

// TODO: preconvert palette to surface-specific color values, where possible

#ifdef ALPHA

#ifdef TINT
#define BLENDPIXEL(target,cr,cg,cb,ca,curval) \
do { \
	if ((ca) != 0) { \
		dR = (((curval)>>rshift)<<rloss)&0xFF; \
		dG = (((curval)>>gshift)<<gloss)&0xFF; \
		dB = (((curval)>>bshift)<<bloss)&0xFF; \
		dR = 1 + (ca)*((tint.r*(cr)) >> 8) + (dR * (255-(ca))); \
 		dR = (dR + (dR >> 8)) >> 8; \
		dG = 1 + (ca)*((tint.g*(cg)) >> 8) + (dG * (255-(ca))); \
 		dG = (dG + (dG >> 8)) >> 8; \
		dB = 1 + (ca)*((tint.b*(cb)) >> 8) + (dB * (255-(ca))); \
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
		dR = (((curval)>>rshift)<<rloss)&0xFF; \
		dG = (((curval)>>gshift)<<gloss)&0xFF; \
		dB = (((curval)>>bshift)<<bloss)&0xFF; \
		dR = 1 + (ca)*(cr) + (dR * (255-(ca))); \
 		dR = (dR + (dR >> 8)) >> 8; \
		dG = 1 + (ca)*(cg) + (dG * (255-(ca))); \
 		dG = (dG + (dG >> 8)) >> 8; \
		dB = 1 + (ca)*(cb) + (dB * (255-(ca))); \
 		dB = (dB + (dB >> 8)) >> 8; \
		target = (PTYPE) ( ((dR) >> rloss) << rshift \
						   | ((dG) >> gloss) << gshift \
						   | ((dB) >> bloss) << bshift); \
	} \
} while (0)
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

{
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

#ifdef ALPHA
	unsigned int dR;
	unsigned int dG;
	unsigned int dB;
#endif

	PTYPE* line = (PTYPE*)(TARGET)->pixels +
		(ty - yneg*((HEIGHT)-1))*(TARGET)->pitch/(PITCHMULT);
	PTYPE* end = line + YNEG(HEIGHT)*(TARGET)->pitch/(PITCHMULT);
	PTYPE* clipstartline = (PTYPE*)(TARGET)->pixels
		+ clipy*((TARGET)->pitch)/PITCHMULT;
	PTYPE* clipendline = clipstartline + cliph*((TARGET)->pitch)/PITCHMULT;
#ifdef COVER
	Uint8* coverline = (Uint8*)cover->pixels + (COVERY)*cover->Width -
		yneg*((HEIGHT)-1);
#endif

#ifdef FLIP
	if (VFLIP_CONDITIONAL) {
		if (end < clipstartline)
			end = clipstartline - ((TARGET)->pitch)/PITCHMULT;
	} else
#endif
	{
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
		Uint8* coverpix = coverline +(COVERX) + translength - xneg*((WIDTH)-1);
#endif
		if (yneg) {
			if (line >= clipendline) clipstartpix = clipendpix;
		} else {
			if (line < clipstartline) clipstartpix = clipendpix;
		}
		while (XNEG((int)(endpix - pix)) > 0)
		{
			Uint8 p = *rle++;
			if (p == (Uint8)data->transindex) {
				int count = XNEG((*rle++) + 1);
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
							BLENDPIXEL(*pix, (PAL)[p].r, (PAL)[p].g, (PAL)[p].b, (ALPHAVALUE), *pix);
						}
					}
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
}



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
