// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef IMAGEMGR_H
#define IMAGEMGR_H

#include "exports.h"

#include "Resource.h"
#include "Sprite2D.h"

namespace GemRB {

class ImageFactory;

/**
 * Base class for Image plugins.
 */
class GEM_EXPORT ImageMgr : public Resource {
public:
	static const TypeID ID;

public:
	/** Returns a \ref Sprite2D containing the image. */
	virtual Holder<Sprite2D> GetSprite2D() = 0;

	virtual Holder<Sprite2D> GetSprite2D(Region&&) = 0;
	/**
	 * Returns image palette.
	 *
	 * @param[in] colors Number of colors to return.
	 * @param[out] pal Palette to fill with colors.
	 *
	 * This does nothing if there is no palette.
	 */
	virtual int GetPalette(int colors, Palette& pal);

	Size GetSize() const { return size; }

	/**
	 * Returns a \ref ImageFactory for the current image.
	 *
	 * @param[in] ResRef name of image represented by factory.
	 */
	std::shared_ptr<ImageFactory> GetImageFactory(const ResRef& ref);

protected:
	Size size;
};

}

#endif
