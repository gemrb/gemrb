/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2011 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#ifndef GemRB_BAMFontManager_h
#define GemRB_BAMFontManager_h

#include "FontManager.h"
#include "BAMImporter.h"

namespace GemRB {

class BAMFontManager : public FontManager
{
private:
	/** private data members */
	BAMImporter* bamImp;
	bool isStateFont = false;
	ResRef resRef;
public:
	/** public methods */
	BAMFontManager(const BAMFontManager&) = delete;
	~BAMFontManager() override;
	BAMFontManager& operator=(const BAMFontManager&) = delete;
	BAMFontManager();

	bool Import(DataStream* stream) override;

	Holder<Font> GetFont(ieWord pxSize, FontStyle style, bool background) override;
};

}

#endif
