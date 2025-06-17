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

#include "fmt/xchar.h"

#include <algorithm>
#include <cassert>
#include <cwctype>
#include <string>
#include <vector>

#define WHITESPACE_STRING_W u"\n\t\r "
#define WHITESPACE_STRING   "\n\t\r "

#define WHITESPACE_STRING_VIEW(StrT) \
	StringViewT<StrT>((sizeof(typename StrT::value_type) == 1) ? (const typename StrT::value_type*) WHITESPACE_STRING : (const typename StrT::value_type*) WHITESPACE_STRING_W, sizeof(WHITESPACE_STRING) - 1)

namespace GemRB {

using String = std::u16string;

template<typename STR>
using StringViewT = StringViewImp<std::add_const_t<typename STR::value_type>>;

template<typename STR>
STR StringFromView(StringViewT<STR> sv)
{
	return STR(sv.c_str(), sv.length());
}

template<typename STR>
typename STR::size_type FindFirstOf(const STR& s, StringViewT<STR> sv, typename STR::size_type pos = 0) noexcept
{
	if (pos >= s.length() || s.empty()) {
		return STR::npos;
	}
	auto iter = std::find_first_of(s.begin() + pos, s.end(), sv.begin(), sv.end());
	return iter == s.end() ? STR::npos : std::distance(s.begin(), iter);
}

template<typename STR, typename IT>
IT FindNotOf(IT first, IT last, StringViewT<STR> s) noexcept
{
	for (; first != last; ++first) {
		if (std::find(s.begin(), s.end(), *first) == s.end()) {
			return first;
		}
	}
	return last;
}

template<typename STR>
typename STR::size_type FindFirstNotOf(const STR& s, StringViewT<STR> sv, typename STR::size_type pos = 0) noexcept
{
	if (pos >= s.length() || s.length() == 0) {
		return STR::npos;
	}

	auto iter = FindNotOf<STR>(s.begin() + pos, s.end(), sv);
	return iter == s.end() ? STR::npos : static_cast<typename STR::size_type>(std::distance(s.begin(), iter));
}

template<typename STR>
typename STR::size_type FindLastNotOf(const STR& s, StringViewT<STR> sv, typename STR::size_type pos = STR::npos, bool reverse = false) noexcept
{
	if (pos >= s.length()) {
		pos = s.length() - 1;
	}
	if (sv.length() == 0) {
		return pos;
	}
	if (!reverse) pos = s.length() - (pos + 1);

	auto iterEnd = s.rend() - (reverse ? pos : 0);
	auto iter = FindNotOf<STR>(s.rbegin() + (reverse ? 0 : pos), iterEnd, sv);
	return iter == iterEnd ? STR::npos : s.length() - 1 - static_cast<typename STR::size_type>(std::distance(s.rbegin(), iter));
}

template<typename STR>
void RTrim(STR& string, StringViewT<STR> chars = WHITESPACE_STRING_VIEW(STR))
{
	auto pos = FindLastNotOf(string, chars);
	if (pos == STR::npos) {
		// deal with delimiter-only strings
		string.clear();
	} else {
		string.erase(pos + 1);
	}
}

template<typename STR>
STR RTrimCopy(STR string, StringViewT<STR> chars = WHITESPACE_STRING_VIEW(STR))
{
	RTrim(string, chars);
	return string;
}

template<typename STR>
void LTrim(STR& string, StringViewT<STR> chars = WHITESPACE_STRING_VIEW(STR))
{
	string.erase(0, FindFirstNotOf(string, chars));
}

template<typename STR>
void TrimString(STR& string, StringViewT<STR> chars = WHITESPACE_STRING_VIEW(STR))
{
	LTrim(string, chars);
	RTrim(string, chars);
}

template<typename STR, typename RET = StringViewT<STR>>
std::vector<RET> Explode(const STR& str, typename STR::value_type delim = ',', size_t lim = 0)
{
	std::vector<RET> elements;
	elements.reserve(lim + 1);
	size_t beg = FindFirstNotOf(str, WHITESPACE_STRING_VIEW(STR));
	size_t cur = beg;
	for (; cur < str.length(); ++cur) {
		if (str[cur] == delim) {
			if (str[beg] == delim) {
				elements.emplace_back();
			} else {
				elements.emplace_back(&str[beg], cur - beg);
			}
			beg = FindFirstNotOf(str, WHITESPACE_STRING_VIEW(STR), static_cast<typename STR::size_type>(cur + 1));
			if (lim && elements.size() == lim) {
				break;
			} else if (beg == STR::npos) {
				elements.emplace_back();
				break;
			} else if (str[beg] == delim) {
				cur = beg - 1;
				continue;
			}
			cur = beg;
		}
	}

	if (beg != STR::npos && beg != cur) {
		// trim any trailing spaces
		size_t last = FindLastNotOf(str, WHITESPACE_STRING_VIEW(STR), static_cast<typename STR::size_type>(beg), true);
		if (last != STR::npos) {
			elements.emplace_back(&str[beg], last - beg + 1);
		}
	}

	return elements;
}

template<typename STR>
StringViewT<STR> SubStr(const STR& str, typename STR::size_type pos, typename STR::size_type len = STR::npos)
{
	if (len == STR::npos) len = str.length() - pos;
	assert(pos + len <= str.length());
	return StringViewT<STR>(&str[0] + pos, len);
}

template<typename CIT, typename IT>
GEM_EXPORT_T IT StringToLower(CIT it, CIT end, IT dest)
{
	for (; it != end; ++it, ++dest) {
		*dest = std::towlower(*it);
	}
	return dest;
}

template<typename T>
GEM_EXPORT_T void StringToLower(T& str)
{
	StringToLower(std::begin(str), std::end(str), std::begin(str));
}

template<typename CIT, typename IT>
GEM_EXPORT_T IT StringToUpper(CIT it, CIT end, IT dest)
{
	for (; it != end; ++it, ++dest) {
		*dest = std::towupper(*it);
	}
	return dest;
}

template<typename T>
GEM_EXPORT_T void StringToUpper(T& str)
{
	StringToUpper(std::begin(str), std::end(str), std::begin(str));
}

template<typename... ARGS>
std::string& AppendFormat(std::string& str, const std::string& fmt, ARGS&&... args)
{
	std::string formatted = fmt::format(fmt, std::forward<ARGS>(args)...);
	return str += formatted;
}

template<typename STR>
GEM_EXPORT_T size_t Count(const STR& str, typename STR::value_type delim = ',')
{
	size_t count = 0;
	for (auto cur = str.begin(); cur != str.end(); ++cur) {
		if (*cur == delim) {
			count++;
		}
	}
	return count;
}

}

#endif
