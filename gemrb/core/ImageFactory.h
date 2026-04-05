// SPDX-FileCopyrightText: 2007 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef IMAGEFACTORY_H
#define IMAGEFACTORY_H

#include "exports.h"

#include "FactoryObject.h"
#include "Sprite2D.h"

namespace GemRB {

class GEM_EXPORT ImageFactory : public FactoryObject {
private:
	Holder<Sprite2D> bitmap;

public:
	ImageFactory(const ResRef& resref, Holder<Sprite2D> bitmap);

	Holder<Sprite2D> GetSprite2D() const { return bitmap; }
};

}

#endif
