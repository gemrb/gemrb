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
 *
 *
 */

#ifndef EXPORTS_CORE_H
#define EXPORTS_CORE_H

#include "config.h"

/**
 * @file exports-core.h
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
