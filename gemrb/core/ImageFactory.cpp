// SPDX-FileCopyrightText: 2007 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ImageFactory.h"

#include <utility>

namespace GemRB {

ImageFactory::ImageFactory(const ResRef& resref, Holder<Sprite2D> bitmap_)
	: FactoryObject(resref, IE_BMP_CLASS_ID), bitmap(std::move(bitmap_))
{
}

}
