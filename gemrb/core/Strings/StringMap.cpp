// SPDX-FileCopyrightText: 2023 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
