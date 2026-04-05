// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef PALETTEDIMAGEMGR_H
#define PALETTEDIMAGEMGR_H

#include "Resource.h"
#include "Sprite2D.h"

namespace GemRB {

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
	/**
	 * Returns a @ref{Sprite2D} that has been colored with the given palette.
	 *
	 * @param[in] type Type of palette to use.
	 * @param[in] paletteIndex Array of palettes to use.
	 */
	virtual Holder<Sprite2D> GetSprite2D(unsigned int type, ieDword paletteIndex[8]) = 0;
};

}

#endif
