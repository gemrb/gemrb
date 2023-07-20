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
 *
 *
 */

#ifndef GemRB_Predicates_h
#define GemRB_Predicates_h

#if defined(__MORPHOS__) && !defined(WARPUP)
#undef bind
#undef Debug
#undef Wait
#undef Remove
#endif

#include <functional>
#include <memory>

namespace GemRB {

template<typename PT>
struct Predicate {
	virtual ~Predicate() noexcept = default;
	virtual bool operator()(const PT& param) const = 0;
};

template <typename T>
using SharedPredicate = std::shared_ptr<Predicate<T>>;

template<typename PT>
struct CompoundPredicate : Predicate<PT> {
protected:
	SharedPredicate<PT> pred1;
	SharedPredicate<PT> pred2;
public:
	CompoundPredicate(SharedPredicate<PT> p1, SharedPredicate<PT> p2) {
		pred1 = p1;
		pred2 = p2;
	}
};

template<typename PT>
struct AndPredicate : CompoundPredicate<PT> {
	AndPredicate(SharedPredicate<PT> p1, SharedPredicate<PT> p2)
	: CompoundPredicate<PT>(p1, p2) {}

	bool operator()(const PT& param) const override {
		return (*this->pred1)(param) && (*this->pred2)(param);
	}
};

template<typename PT>
struct OrPredicate : CompoundPredicate<PT> {
	OrPredicate(SharedPredicate<PT> p1, SharedPredicate<PT> p2)
	: CompoundPredicate<PT>(p1, p2) {}

	bool operator()(const PT& param) const override {
		return (*this->pred1)(param) || (*this->pred2)(param);
	}
};

}

#endif
