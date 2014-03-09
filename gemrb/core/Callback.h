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

#include "exports.h"

#include "Holder.h"

namespace GemRB {

class GEM_EXPORT VoidCallback : public Held<VoidCallback> {
public:
	virtual ~VoidCallback() {};
	virtual bool operator()()=0;
};

template<class T>
class GEM_EXPORT Callback : public VoidCallback {
public:
	virtual bool operator()(T target)=0;
};

template<class L, typename T>
class GEM_EXPORT MethodCallback : public Callback<T> {
	typedef bool (L::*CallbackMethod)(T);
private:
	L* listener;
	CallbackMethod callback;
public:
	MethodCallback(L* l, CallbackMethod cb)
	: listener(l), callback(cb) {}

	bool operator()(T target) {
		return (listener->*callback)(target);
	}

	bool operator()() {
		return true;
	}
};

class GEM_EXPORT EventHandler : public Holder<VoidCallback> {
public:
	EventHandler(VoidCallback* ptr = NULL)
	: Holder<VoidCallback>(ptr) {}

	bool operator()() {
		return (*ptr)();
	}
};

}

#endif
