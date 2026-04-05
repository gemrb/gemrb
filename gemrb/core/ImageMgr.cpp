// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ImageMgr.h"

#include "ImageFactory.h"

namespace GemRB {

const TypeID ImageMgr::ID = { "ImageMgr" };

int ImageMgr::GetPalette(int /*colors*/, Palette& /*pal*/)
{
	return -1;
}

std::shared_ptr<ImageFactory> ImageMgr::GetImageFactory(const ResRef& ref)
{
	return std::make_shared<ImageFactory>(ref, GetSprite2D());
}

}
