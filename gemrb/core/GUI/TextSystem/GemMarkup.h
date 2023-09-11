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
#include "Strings/String.h"

#include <map>
#include <stack>

namespace GemRB {

class GemMarkupParser {
public:
	enum ParseState {
		TEXT = 0,
		OPEN_TAG,
		CLOSE_TAG,
		COLOR,
		INT
	};

	GemMarkupParser();
	GemMarkupParser(const Holder<Font> ftext, Font::PrintColors textCols,
					const Holder<Font> finit, Font::PrintColors initCols);
	~GemMarkupParser() noexcept = default;

	void ResetAttributes(const Holder<Font> ftext, Font::PrintColors textCols,
						 const Holder<Font> finit, Font::PrintColors initCols);

	void Reset();

	TextSpan* ParseMarkupTag(const String&) const;
	ParseState ParseMarkupStringIntoContainer(const String&, TextContainer&);

private:
	class TextAttributes {
		private:
		Font::PrintColors textColor;
		Font::PrintColors swapColor;

		public:
		Holder<Font> TextFont;
		Holder<Font> SwapFont;

		public:
		TextAttributes(const Holder<Font> text, Font::PrintColors textColor,
					   const Holder<Font> init, Font::PrintColors initColor)
		: textColor(textColor), swapColor(initColor), TextFont(text), SwapFont(init)
		{
			assert(TextFont && SwapFont);
		}

		TextAttributes(const TextAttributes& ta) = default;
		TextAttributes& operator=(const TextAttributes& ta) = default;

		void SwapFonts() {
			std::swap(TextFont, SwapFont);
			std::swap(textColor, swapColor);
		}

		void SetTextColor(const Font::PrintColors& c) {
			textColor = c;
		}

		const Font::PrintColors& TextColor() const {
			return textColor;
		}
	};

	std::stack<TextAttributes> context;
	ParseState state;
	Color textBg;
};

}

#endif
