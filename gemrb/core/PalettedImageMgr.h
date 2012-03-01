/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef PALETTEDIMAGEMGR_H
#define PALETTEDIMAGEMGR_H

#include "Resource.h"
#include "Sprite2D.h"
#include "System/DataStream.h"

namespace GemRB {

class ImageFactory;

/**
 * Base class for Paletted Imqge plugins
 *
 * This represents a false color image that is combined with
 * color data to generate a real image.
 */
class GEM_EXPORT PalettedImageMgr : public Resource {
public:
	static const TypeID ID;
public:
	PalettedImageMgr(void);
	virtual ~PalettedImageMgr(void);
	/**
	 * Returns a @ref{Sprite2D} that has been colored with the given palette.
	 *
	 * @param[in] type Type of palette to use.
	 * @param[in] paletteIndex Array of palettes to use.
	 */
	virtual Sprite2D* GetSprite2D(unsigned int type, ieDword paletteIndex[8]) = 0;
};

}

#endif
