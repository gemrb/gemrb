/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2011 The GemRB Project
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
 *
 *
 */

#ifndef GemRB_FontManager_h
#define GemRB_FontManager_h

#include "TypeID.h"
#include "Resource.h"
#include "exports.h"
#include "GUI/TextSystem/Font.h"

namespace GemRB {

class GEM_EXPORT FontManager : public Resource {
public:
	// Public data members
	static const TypeID ID;
public:
	// Public methods
	FontManager();
	virtual ~FontManager(void);

	virtual Font* GetFont(ieWord pxSize, FontStyle style, Palette* = NULL)=0;
};

}

#endif
