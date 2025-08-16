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

#ifndef EXPORTS_H
#define EXPORTS_H

#ifdef HAVE_CONFIG_H
	#include "config.h"
#endif

/**
 * @file exports.h
 * This file contains global compiler configuration.
 *
 * It should not contain any declarations or includes,
 * only compiler dependent macros and pragmas.
 */

/// Symbol visibility macros
#ifndef STATIC_LINK
	#ifdef WIN32
		#ifdef GEM_BUILDING_CORE
			#define GEM_EXPORT   __declspec(dllexport)
			#define GEM_EXPORT_T GEM_EXPORT
		#else
			#define GEM_EXPORT __declspec(dllimport)
			#define GEM_EXPORT_T
		#endif
		#define GEM_EXPORT_DLL extern "C" __declspec(dllexport)
	#else
		#ifdef __GNUC__
			#ifdef GEM_BUILDING_CORE
				#define GEM_EXPORT   __attribute__((visibility("default")))
				#define GEM_EXPORT_T GEM_EXPORT
			#endif
			#define GEM_EXPORT_DLL extern "C" __attribute__((visibility("default")))
		#endif
	#endif
#endif

#ifndef GEM_EXPORT
	#define GEM_EXPORT
	#define GEM_EXPORT_T
#endif

#ifndef GEM_EXPORT_DLL
	#define GEM_EXPORT_DLL extern "C"
#endif


/// Make sure we don't link to static libraries
/// This causes hard to debug errors due to multiple heaps.
#if defined(_MSC_VER) && !defined(_DLL)
	#error GemRB must be dynamically linked with runtime libraries on win32.
#endif


#endif
