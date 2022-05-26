/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef STRING_H
#define STRING_H

#include "exports.h"

#include "StringView.h"

#include <algorithm>
#include <cwctype>
#include <string>
#include <vector>

#include "Format.h"
#include <fmt/xchar.h>

#define WHITESPACE_STRING_W L"\n\t\r "
#define WHITESPACE_STRING "\n\t\r "

#define WHITESPACE_STRING_VIEW(StrT) \
StringViewT<StrT>((sizeof(typename StrT::value_type) == 1) ? (const typename StrT::value_type*)WHITESPACE_STRING : (const typename StrT::value_type*)WHITESPACE_STRING_W, sizeof(WHITESPACE_STRING) - 1)

namespace GemRB {

using String = std::basic_string<wchar_t>;

template <typename STR>
using StringViewT = StringViewImp<typename std::add_const<typename STR::value_type>::type>;

template <typename STR>
STR StringFromView(StringViewT<STR> sv)
{
	return STR(sv.c_str(), sv.length());
}

template <typename STR, typename IT>
IT FindNotOf(IT first, IT last, StringViewT<STR> s) noexcept
{
	for (; first != last ; ++first) {
		if (std::find(s.begin(), s.end(), *first) == s.end()) {
			return first;
		}
	}
	return last;
}

template <typename STR>
typename STR::size_type FindFirstNotOf(const STR& s, StringViewT<STR> sv, typename STR::size_type pos = 0) noexcept
{
	if (pos >= s.length()) {
		return STR::npos;
	}
	if (s.length() == 0) {
		return pos;
	}
	auto iter = FindNotOf<STR>(s.begin() + pos, s.end(), sv);
	return iter == s.end() ? STR::npos : std::distance(s.begin(), iter);
}

template <typename STR>
typename STR::size_type FindLastNotOf(const STR& s, StringViewT<STR> sv, typename STR::size_type pos = STR::npos) noexcept
{
	if (pos >= s.length()) {
		pos = s.length() - 1;
	}
	if (sv.length() == 0) {
		return pos;
	}
	pos = s.length() - (pos + 1);
	auto iter = FindNotOf<STR>(s.rbegin() + pos, s.rend(), sv);
	return iter == s.rend() ? STR::npos : s.length() - 1 - std::distance(s.rbegin(), iter);
}

template <typename STR>
void RTrim(STR& string, StringViewT<STR> chars = WHITESPACE_STRING_VIEW(STR))
{
	auto pos = FindLastNotOf(string, chars);
	if (pos != STR::npos) {
		string.erase(pos + 1);
	}
}

template <typename STR>
void LTrim(STR& string, StringViewT<STR> chars = WHITESPACE_STRING_VIEW(STR))
{
	string.erase(0, FindFirstNotOf(string, chars));
}

template <typename STR>
void TrimString(STR& string, StringViewT<STR> chars = WHITESPACE_STRING_VIEW(STR))
{
	LTrim(string, chars);
	RTrim(string, chars);
}

template<typename STR, typename RET = StringViewT<STR>>
std::vector<RET> Explode(const STR& str, typename STR::value_type delim = ',')
{
	std::vector<RET> elements;
	size_t beg = FindFirstNotOf(str, WHITESPACE_STRING_VIEW(STR));
	size_t cur = beg;
	for (; cur < str.length(); ++cur)
	{
		if (str[cur] == delim) {
			elements.emplace_back(&str[beg], cur - beg);
			beg = FindFirstNotOf(str, WHITESPACE_STRING_VIEW(STR), cur + 1);
		}
	}

	if (beg != STR::npos && beg != cur) {
		elements.emplace_back(&str[beg]);
	}

	return elements;
}

template<typename ...ARGS>
std::string& AppendFormat(std::string& str, const std::string& fmt, ARGS&& ...args) {
	std::string formatted = fmt::format(fmt, std::forward<ARGS>(args)...);
	return str += formatted;
}

}

#endif
