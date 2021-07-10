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

#include "GemMarkup.h"

#include <cwchar>

namespace GemRB {

static Color ParseColor(const String& colorString)
{
	Color color = ColorWhite;
	swscanf(colorString.c_str(), L"%02hhx%02hhx%02hhx%02hhx", &color.r, &color.g, &color.b, &color.a);
	return color;
}

GemMarkupParser::GemMarkupParser()
{
	state = TEXT;
}

GemMarkupParser::GemMarkupParser(const Font* ftext, Font::PrintColors textCols,
								 const Font* finit, Font::PrintColors initCols)
{
	state = TEXT;
	ResetAttributes(ftext, textCols, finit, initCols);
}

void GemMarkupParser::ResetAttributes(const Font* ftext, Font::PrintColors textCols, const Font* finit, Font::PrintColors initCols)
{
	while(!context.empty()) context.pop();
	textBg = textCols.bg;
	context.emplace(ftext, textCols, finit, initCols);
}

void GemMarkupParser::Reset()
{
	// keep only the starting text attributes (assuming we have some)
	while (context.size() > 1) {
		context.pop();
	}
}

GemMarkupParser::ParseState
GemMarkupParser::ParseMarkupStringIntoContainer(const String& text, TextContainer& container)
{
	size_t tagPos = text.find_first_of('[');
	if (tagPos != 0) {
		// handle any text before the markup
		container.AppendText(text.substr(0, tagPos));
	}
	// parse the text looking for accepted tags ([cap], [color], [p])
	// [cap] encloses a span of text to be rendered with the finit font
	// [color=%02X%02X%02X] encloses a span of text to be rendered with the given RGB values
	// [p] encloses a span of text to be rendered as an inline block:
	//     it will grow vertically as needed, but be confined to the remaining width of the line

	// TODO: implement escaping [] ('\')
	Size frame;
	String token;

	assert(tagPos < text.length());
	String::const_iterator it = text.begin() + tagPos;
	for (; it != text.end(); ++it) {
		assert(!context.empty());
		TextAttributes& attributes = context.top();

		switch (state) {
			case OPEN_TAG:
				switch (*it) {
					case '=':
						if (token == L"color") {
							state = COLOR;
							token.clear();
						}
						// else is a parse error...
						continue;
					case ']':
						if (token == L"cap") {
							attributes.SwapFonts();
							//align = IE_FONT_SINGLE_LINE;
						} else if (token == L"p") {
							frame.w = -1;
						}
						state = TEXT;
						token.clear();
						continue;
					case '[': // wasn't actually a tag after all
						state = TEXT;
						token.insert((String::size_type) 0, 1, L'[');
						--it; // rewind so the TEXT node is created
						continue;
				}
				break;
			case CLOSE_TAG:
				switch (*it) {
					case ']':
						if (token == L"color") {
							context.pop();
						} else if (token == L"cap") {
							attributes.SwapFonts();
							//align = 0;
						} else if (token == L"p") {
							frame.w = 0;
						}
						state = TEXT;
						token.clear();
						continue;
				}
				break;
			case TEXT:
				switch (*it) {
					case '[':
						if (token.length() && token != L"\n") {
							// FIXME: lazy hack.
							// we ought to ignore all white space between markup unless it contains other text
							container.AppendContent(new TextSpan(token, attributes.TextFont, attributes.TextColor(), &frame));
						}
						token.clear();
						if (*++it == '/')
							state = CLOSE_TAG;
						else {
							--it;
							state = OPEN_TAG;
						}
						continue;
				}
				break;
			case COLOR:
				switch (*it) {
					case L']':
						context.emplace(attributes);
						context.top().SetTextColor({ParseColor(token), textBg});
						state = TEXT;
						token.clear();
						continue;
				}
				break;
			default: // parse error, not clearing token
				state = TEXT;
				break;
		}
		assert(it >= text.begin() && it < text.end());
		token += *it;
	}

	if (token.length()) {
		// there was some text at the end without markup
		container.AppendText(token);
	}
	return state;
}

}
