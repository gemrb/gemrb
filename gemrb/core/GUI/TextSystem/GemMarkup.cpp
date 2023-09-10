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

#include <codecvt>

namespace GemRB {

static Color ParseColor(const String& colorString)
{
	Color color = ColorWhite;
	uint8_t values[4] = {0, 0, 0, 0};

	// That's a short way to interpret 4 groups of %02hhx,
	// since `swscanf` does not work on our string type unless converting first
	for (uint8_t i = 0; i < 4; ++i) {
		uint8_t value = 0;

		for (uint8_t j = 0; j < 2; ++j) {
			uint8_t nibble = 0;
			switch (colorString[i * 2 + j]) {
				case u'0': nibble = 0x0; break;
				case u'1': nibble = 0x1; break;
				case u'2': nibble = 0x2; break;
				case u'3': nibble = 0x3; break;
				case u'4': nibble = 0x4; break;
				case u'5': nibble = 0x5; break;
				case u'6': nibble = 0x6; break;
				case u'7': nibble = 0x7; break;
				case u'8': nibble = 0x8; break;
				case u'9': nibble = 0x9; break;
				case u'a': case u'A': nibble = 0xA; break;
				case u'b': case u'B': nibble = 0xB; break;
				case u'c': case u'C': nibble = 0xC; break;
				case u'd': case u'D': nibble = 0xD; break;
				case u'e': case u'E': nibble = 0xE; break;
				case u'f': case u'F': nibble = 0xF; break;
			}

			value |= (nibble << (4 * (1 - j)));
		}

		values[i] = value;
	}

	color.r = values[0];
	color.g = values[1];
	color.b = values[2];
	color.a = values[3];

	return color;
}

GemMarkupParser::GemMarkupParser()
{
	state = TEXT;
}

GemMarkupParser::GemMarkupParser(const Holder<Font> ftext, Font::PrintColors textCols,
								 const Holder<Font> finit, Font::PrintColors initCols)
{
	state = TEXT;
	ResetAttributes(ftext, textCols, finit, initCols);
}

void GemMarkupParser::ResetAttributes(const Holder<Font> ftext, Font::PrintColors textCols, const Holder<Font> finit, Font::PrintColors initCols)
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
	String saved;

	assert(tagPos < text.length());
	String::const_iterator it = text.begin() + tagPos;
	for (; it != text.end(); ++it) {
		assert(!context.empty());
		TextAttributes& attributes = context.top();

		switch (state) {
			case OPEN_TAG:
				switch (*it) {
					case '=':
						if (token == u"color") {
							state = COLOR;
							token.clear();
						} else if (token == u"int") {
							state = INT;
							token.clear();
						}
						// else is a parse error...
						continue;
					case ']':
						state = TEXT;
						if (token == u"cap") {
							attributes.SwapFonts();
							//align = IE_FONT_SINGLE_LINE;
						} else if (token == u"p") {
							frame.w = -1;
						}
						token.clear();
						continue;
					case '[': // wasn't actually a tag after all
						state = TEXT;
						token.insert((String::size_type) 0, 1, u'[');
						--it; // rewind so the TEXT node is created
						continue;
				}
				break;
			case CLOSE_TAG:
				switch (*it) {
					case ']':
						if (token == u"color") {
							context.pop();
						} else if (token == u"cap") {
							attributes.SwapFonts();
							//align = 0;
						} else if (token == u"p") {
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
						if (*++it == '/') {
							state = CLOSE_TAG;
						} else if (*it == '+') {
							state = OPEN_TAG;
							saved = token;
							token.clear();
							continue; // + means concatonate with the existing text
						} else {
							--it;
							state = OPEN_TAG;
						}
						
						token = saved + token;
						saved.clear();
						if (token.length() && token != u"\n") {
							// FIXME: lazy hack.
							// we ought to ignore all white space between markup unless it contains other text
							container.AppendContent(new TextSpan(token, attributes.TextFont, attributes.TextColor(), &frame));
						}
						token.clear();
						continue;
				}
				break;
			case COLOR:
				switch (*it) {
					case u']':
						context.emplace(attributes);
						context.top().SetTextColor({ParseColor(token), textBg});
						state = TEXT;
						token.clear();
						continue;
				}
				break;
			case INT:
				if (*it == u']') {
					std::wstring_convert<std::codecvt_utf16<wchar_t, 0x10ffff, std::little_endian>, wchar_t> conv;
					std::wstring wToken = conv.from_bytes(
						reinterpret_cast<const char*>(&token[0]),
						reinterpret_cast<const char*>(&token[0] + token.size())
					);
					// state icons, invalid as unicode, so we cant translate in Python
					wchar_t chr = (wchar_t)wcstoul(wToken.c_str(), nullptr, 0);
					token.clear();
					token.push_back(chr);
					state = TEXT;
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
		container.AppendText(std::move(token));
	}
	return state;
}

}
