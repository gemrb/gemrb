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

#ifdef WIN32
#	ifdef GEM_BUILD_DLL
#		define GEM_EXPORT __declspec(dllexport)
#	else
#		define GEM_EXPORT __declspec(dllimport)
#	endif
#	define GEM_EXPORT_DLL extern "C" __declspec(dllexport)
#else
#	if (__GNUC__ >= 3) && (__GNUC_MINOR__ >=4 || __GNUC__ > 3)
#		ifdef GEM_BUILD_DLL
#			define GEM_EXPORT __attribute__ ((visibility("default")))
#		endif
#		define GEM_EXPORT_DLL extern "C" __attribute__ ((visibility("default")))
#	endif
#endif

#ifndef GEM_EXPORT
#define GEM_EXPORT
#endif

#ifndef GEM_EXPORT_DLL
#define GEM_EXPORT_DLL extern "C"
#endif

#ifdef __GNUC__
#define WARN_UNUSED __attribute__ ((warn_unused_result))
#define SENTINEL __attribute__ ((sentinel))
#else
#define WARN_UNUSED
#define SENTINEL
#endif

#endif
