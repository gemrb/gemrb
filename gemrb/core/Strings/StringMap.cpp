/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2023 The GemRB Project
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

#include "StringMap.h"

namespace GemRB {

HeterogeneousStringKey::HeterogeneousStringKey(std::string str) noexcept
	: keyBuf(std::make_unique<std::string>(std::move(str))),
	  key(*keyBuf)
{}

HeterogeneousStringKey::HeterogeneousStringKey(StringView sv) noexcept
	: key(sv)
{}

HeterogeneousStringKey::HeterogeneousStringKey(const HeterogeneousStringKey& other) noexcept
	: HeterogeneousStringKey(other.key.MakeString())
{}

HeterogeneousStringKey& HeterogeneousStringKey::operator=(const HeterogeneousStringKey& other) noexcept
{
	if (&other != this) {
		keyBuf = std::make_unique<std::string>(other.key.MakeString());
		key = StringView(*keyBuf);
	}
	return *this;
}

HeterogeneousStringKey::operator StringView() const noexcept
{
	return key;
}

fmt::string_view format_as(const HeterogeneousStringKey& key)
{
	StringView sv(key);
	return fmt::string_view(sv.c_str(), sv.length());
}

}
