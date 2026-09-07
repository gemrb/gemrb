// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef EXPORTS_CORE_H
#define EXPORTS_CORE_H

#include "config.h"

/**
 * @file exports.h
 * This file contains global compiler configuration related to symbol visibility.
 */

#if defined(__has_include) && !defined(STATIC_LINK)
	#if __has_include("gem-core-export.h")
		#include "gem-core-export.h"
	#else
		// if the file wasn't found, fallback to manual implementation
		// left here for the included xcode project until #1865
		// afterwards we can simplify to the gem-core-export.h include and the last two fallbacks for static builds
		#if !defined(GEM_NO_EXPORT) && defined(__GNUC__)
			#define GEM_EXPORT   __attribute__((visibility("default")))
			#define GEM_EXPORT_T GEM_EXPORT
		#endif
	#endif
#endif

#ifndef GEM_EXPORT
	#define GEM_EXPORT
#endif
#ifndef GEM_EXPORT_T
	#define GEM_EXPORT_T
#endif
#define GEM_EXPORT_DLL extern "C" GEM_EXPORT_T

/**
 * Branch-probability hints. Only for cases where a profile (or the shape of the data) says a
 * branch really is near-always one way, and where the compiler's block layout was measured to be
 * wrong without them - do not use it without a reason.
 *
 * GCC and Clang (which also defines __GNUC__) get __builtin_expect. MSVC has no equivalent - it
 * dropped __assume-based hinting in favour of PGO - so it gets the no-op.
 * C++20's [[likely]]/[[unlikely]] would replace this.
 */
#if defined(__GNUC__) || defined(__clang__)
	#define GEM_LIKELY(x)   __builtin_expect(!!(x), 1)
	#define GEM_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
	#define GEM_LIKELY(x)   (!!(x))
	#define GEM_UNLIKELY(x) (!!(x))
#endif

/* Keeps a function out of line. Only used where the alternative - letting the compiler inline it -
 * is measurably worse: either it would grow a hot loop's other, more common paths through worse
 * code layout, or (elsewhere) it would let the compiler re-materialise a returned value at every
 * use site instead of keeping it in a register.
 */
#if defined(__GNUC__) || defined(__clang__)
	#define GEM_NOINLINE __attribute__((noinline))
#elif defined(_MSC_VER)
	#define GEM_NOINLINE __declspec(noinline)
#else
	#define GEM_NOINLINE
#endif

#endif
