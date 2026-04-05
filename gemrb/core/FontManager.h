// SPDX-FileCopyrightText: 2011 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef GemRB_FontManager_h
#define GemRB_FontManager_h

#include "exports.h"

#include "Resource.h"
#include "TypeID.h"

#include "GUI/TextSystem/Font.h"

namespace GemRB {

class GEM_EXPORT FontManager : public Resource {
public:
	// Public data members
	static const TypeID ID;

public:
	virtual Holder<Font> GetFont(ieWord pxSize, FontStyle style, bool background) = 0;
};

}

#endif
