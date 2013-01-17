/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2011 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#define NOCOLOR 1
#define HAVE_ICONV 1

#ifndef HAVE_SNPRINTF
	#define HAVE_SNPRINTF 1
#endif

#if TARGET_OS_IPHONE
	#define PACKAGE "GemRB"
	#define TOUCHSCREEN
	#define STATIC_LINK//this is in the target build settings now.
	#define SIZEOF_INT 4
	#define SIZEOF_LONG_INT 8

	#define DATADIR UserDir
#else
	#define PACKAGE "GemRB"
	#define SIZEOF_INT 4
	#define SIZEOF_LONG_INT 8

	#define MAC_GEMRB_APPSUPPORT "~/Library/Application Support/GemRB"
	#define PLUGINDIR "~/Library/Application Support/GemRB/plugins"
	#define DATADIR UserDir
#endif

#define HAVE_FORBIDDEN_OBJECT_TO_FUNCTION_CAST 1
