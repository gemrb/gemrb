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

#include "config.h"

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

/// Semantic Warning Macros
#ifdef HAVE_ATTRIBUTE_WARN_UNUSED_RESULT
	#define WARN_UNUSED __attribute__((warn_unused_result))
#else
	#define WARN_UNUSED
#endif

/// Disable silly MSVC warnings
#if _MSC_VER >= 1000
	//  4138 disables the warning for */ found outside of comment, mostly in GameScript.h
	#pragma warning(disable : 4138)
	//  4267 disables the warnings related to conversion between size_t and other types
	#pragma warning(disable : 4267)
	//	4251 disables the annoying warning about missing dll interface in templates
	#pragma warning(disable : 4251 521)
	#pragma warning(disable : 4275)
	// _CRT_SECURE_NO_WARNINGS on use of various strcpy/printf variants
	#pragma warning(disable : 4996)
	//  coercion to bool
	#pragma warning(disable : 4800)
	//  conversion from 'GemRB::ieWord' to 'GemRB::ieByte', possible loss of data
	#pragma warning(disable : 4244)
	//  new behavior: elements of array will be default initialized
	#pragma warning(disable : 4351)
	//	disables warnings about posix functions
	#define _CRT_NONSTDC_NO_DEPRECATE 1
#endif

/// Make sure we don't link to static libraries
/// This causes hard to debug errors due to multiple heaps.
#if defined(_MSC_VER) && !defined(_DLL)
	#error GemRB must be dynamically linked with runtime libraries on win32.
#endif

/// Silence some persistent unused warnings (supported since gcc 2.4)
#ifdef HAVE_ATTRIBUTE_UNUSED
	#define IGNORE_UNUSED __attribute__((unused))
#else
	#define IGNORE_UNUSED
#endif

#endif
