// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef HOLDER_H
#define HOLDER_H

#include <memory>

namespace GemRB {

template<typename T>
using Holder = std::shared_ptr<T>;

template<class T, typename... ARGS>
inline Holder<T> MakeHolder(ARGS&&... args) noexcept
{
	return std::make_shared<T>(std::forward<ARGS>(args)...);
}

}

#endif
