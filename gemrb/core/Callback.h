// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef CALLBACK_H
#define CALLBACK_H

#include <functional>

#define METHOD_CALLBACK(func_ptr, obj_ptr) std::bind(func_ptr, obj_ptr, std::placeholders::_1)

namespace GemRB {

template<typename R, typename... ARGS>
using Callback = std::function<R(ARGS...)>;

using EventHandler = std::function<void()>;

// std::function has an explicitly deleted operator== so we need to write our own comparator
// that makes sense for our purposes
template<typename R, typename... ARGS>
bool FunctionTargetsEqual(const std::function<R(ARGS...)>& lhs, const std::function<R(ARGS...)>& rhs)
{
	using fnType = R (*)(ARGS...);
	return lhs.template target<fnType*>() == rhs.template target<fnType*>();
}

}

#endif
