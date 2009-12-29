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
 * $Id$
 *
 */

/**
 * @file Sprite2D.h
 * Declares Sprite2D, class representing bitmap data
 * @author The GemRB Project
 */

#ifndef SPRITE2D_H
#define SPRITE2D_H

#include "../../includes/exports.h"
#include "../../includes/RGBAColor.h"
#include "Palette.h"

class AnimationFactory;

/**
 * @class Sprite2D
 * Class representing bitmap data.
 * Objects of this class are usually created by Video driver.
 */

class Sprite2D_BAM_Internal {
public:
        Sprite2D_BAM_Internal() { pal = 0; }
        ~Sprite2D_BAM_Internal() { if (pal) { pal->Release(); pal = 0; } }

        Palette* pal;
        bool RLE;
        int transindex;
        bool flip_hor;
        bool flip_ver;

        // The AnimationFactory in which the data for this sprite is stored.
        // (Used for refcounting of the data.)
        AnimationFactory* source;
};

class GEM_EXPORT Sprite2D {
public:
	/** Pointer to the Driver Video Structure */
	void* vptr;
	bool BAM;
	int RefCount;
	const void* pixels;
	int XPos, YPos, Width, Height, Bpp;
	Sprite2D(void);
	~Sprite2D(void);
	bool IsPixelTransparent(unsigned short x, unsigned short y);
	Palette *GetPalette();
	void SetPalette(Palette *pal);
	Color GetPixel(unsigned short x, unsigned short y);
};

#endif  // ! SPRITE2D_H
