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

class GEM_EXPORT CallbackBase : public Held<CallbackBase> {
	public:
	virtual ~CallbackBase() {};
};

template<typename P=void, typename R=void>
class GEM_EXPORT Callback : public CallbackBase {
public:
	virtual R operator()(P) const {};
};

// specialization for no argument
template<typename R>
class GEM_EXPORT Callback<void, R> : public Held< Callback<void, R> > {
public:
	virtual ~Callback() {}
	virtual R operator()() const=0;
};

template<class C, typename P=void, typename R=void>
class GEM_EXPORT MethodCallback : public Callback<P,R> {
	typedef R (C::*CallbackMethod)(P);
private:
	C* listener;
	CallbackMethod callback;
public:
	MethodCallback(C* l, CallbackMethod cb)
	: listener(l), callback(cb) {}

	R operator()(P target) const {
		return (listener->*callback)(target);
	}
};

typedef Callback<void, void> VoidCallback;

class GEM_EXPORT EventHandler : public Holder<VoidCallback> {
public:
	EventHandler(VoidCallback* ptr = NULL)
	: Holder<VoidCallback>(ptr) {}

	void operator()() const {
		return (*ptr)();
	}
};

}

#endif
