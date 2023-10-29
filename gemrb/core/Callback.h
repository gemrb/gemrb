/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
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

#ifndef CALLBACK_H
#define CALLBACK_H

#include <functional>

#define METHOD_CALLBACK(func_ptr, obj_ptr) std::bind(func_ptr, obj_ptr, std::placeholders::_1)

namespace GemRB {

template <typename R, typename... ARGS>
using Callback = std::function<R(ARGS...)>;

using EventHandler = std::function<void()>;

// std::function has an explicitly deleted operator== so we need to write our own comparator
// that makes sense for our purposes
template <typename R, typename... ARGS>
bool FunctionTargetsEqual(const std::function<R(ARGS...)>& lhs, const std::function<R(ARGS...)>& rhs)
{
	using fnType = R(*)(ARGS...);
	return lhs.template target<fnType*>() == rhs.template target<fnType*>();
}

}

#endif
