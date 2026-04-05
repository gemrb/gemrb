// SPDX-FileCopyrightText: 2014 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

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

template<typename T>
using SharedPredicate = std::shared_ptr<Predicate<T>>;

template<typename PT>
struct CompoundPredicate : Predicate<PT> {
protected:
	SharedPredicate<PT> pred1;
	SharedPredicate<PT> pred2;

public:
	CompoundPredicate(SharedPredicate<PT> p1, SharedPredicate<PT> p2)
	{
		pred1 = std::move(p1);
		pred2 = std::move(p2);
	}
};

template<typename PT>
struct AndPredicate : CompoundPredicate<PT> {
	AndPredicate(SharedPredicate<PT> p1, SharedPredicate<PT> p2)
		: CompoundPredicate<PT>(std::move(p1), std::move(p2)) {}

	bool operator()(const PT& param) const override
	{
		return (*this->pred1)(param) && (*this->pred2)(param);
	}
};

template<typename PT>
struct OrPredicate : CompoundPredicate<PT> {
	OrPredicate(SharedPredicate<PT> p1, SharedPredicate<PT> p2)
		: CompoundPredicate<PT>(std::move(p1), std::move(p2)) {}

	bool operator()(const PT& param) const override
	{
		return (*this->pred1)(param) || (*this->pred2)(param);
	}
};

}

#endif
