/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2014 The GemRB Project
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

#ifndef GEMMARKUP_H
#define GEMMARKUP_H

#include "GameData.h"
#include "TextContainer.h"

namespace GemRB {

class GemMarkupParser {
private:
	const Font* ftext;
	const Font* finit;
	Palette* palette;

public:
	GemMarkupParser() : ftext(NULL), finit(NULL), palette(NULL) {};
	~GemMarkupParser() {
		gamedata->FreePalette(palette);
	};

	void SetTextDefaults(const Font* ftext, const Font* finit, Palette* textCol);
	TextSpan* ParseMarkupTag(const String&) const;
	void ParseMarkupStringIntoContainer(const String&, TextContainer&) const;
};

}

#endif
