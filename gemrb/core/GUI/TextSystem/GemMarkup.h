// SPDX-FileCopyrightText: 2014 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef GEMMARKUP_H
#define GEMMARKUP_H

#include "Font.h"

#include "Strings/String.h"

#include <stack>

namespace GemRB {

class TextContainer;
class TextSpan;

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

		void SwapFonts()
		{
			std::swap(TextFont, SwapFont);
			std::swap(textColor, swapColor);
		}

		void SetTextColor(const Font::PrintColors& c)
		{
			textColor = c;
		}

		const Font::PrintColors& TextColor() const
		{
			return textColor;
		}
	};

	std::stack<TextAttributes> context;
	ParseState state;
	Color textBg;
};

}

#endif
