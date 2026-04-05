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
			#define GEM_EXPORT     __attribute__((visibility("default")))
			#define GEM_EXPORT_T   GEM_EXPORT
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

#endif
