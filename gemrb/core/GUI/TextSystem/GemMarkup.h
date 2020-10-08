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

#include "TextContainer.h"
#include "System/String.h"

#include <map>
#include <stack>

namespace GemRB {

class GemMarkupParser {
public:
	enum ParseState {
		TEXT = 0,
		OPEN_TAG,
		CLOSE_TAG,
		COLOR
	};

	GemMarkupParser();
	GemMarkupParser(const Font* ftext, PaletteHolder textPal = nullptr,
					const Font* finit = nullptr, PaletteHolder initPal = nullptr);
	~GemMarkupParser() {};

	void ResetAttributes(const Font *ftext = nullptr, PaletteHolder textPal = nullptr,
						 const Font *finit = nullptr, PaletteHolder initPal = nullptr);

	void Reset();

	TextSpan* ParseMarkupTag(const String&) const;
	ParseState ParseMarkupStringIntoContainer(const String&, TextContainer&);

private:
	class TextAttributes {
		private:
		Holder<Palette> palette;
		Holder<Palette> swapPalette;

		public:
		const Font* TextFont;
		const Font* SwapFont;

		public:
		TextAttributes(const Font* text, PaletteHolder textPal = nullptr,
					   const Font *init = nullptr, PaletteHolder initPal = nullptr)
		: palette(textPal), swapPalette(initPal)
		{
			TextFont = text;
			SwapFont = (init) ? init : TextFont;
			assert(TextFont);
		}

		TextAttributes(const TextAttributes& ta)
		: palette(ta.palette), swapPalette(ta.swapPalette)
		{
			TextFont = ta.TextFont;
			SwapFont = ta.SwapFont;
		}

		TextAttributes& operator=(const TextAttributes& ta) {
			if (&ta != this) {
				TextFont = ta.TextFont;
				SwapFont = ta.SwapFont;
				palette = ta.palette;
				swapPalette = ta.swapPalette;
			}
			return *this;
		}

		void SwapFonts() {
			std::swap(TextFont, SwapFont);
			std::swap(palette, swapPalette);
		}

		void SetTextPalette(Holder<Palette> pal) {
			palette = pal;
		}

		Holder<Palette> TextPalette() const {
			if (palette) {
				return palette;
			}
			return TextFont->GetPalette();
		}
	};

	static Holder<Palette> GetSharedPalette(const String& colorString);

	typedef std::map<String, Holder<Palette> > PaletteCache;
	static PaletteCache PalCache;
	std::stack<TextAttributes> context;
	ParseState state;
};

}

#endif
