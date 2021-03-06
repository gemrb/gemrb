/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2021 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#ifndef PLATFORM_H
#define PLATFORM_H

#ifdef HAVE_CONFIG_H
	#include <config.h>
#else
	// we need this fallback for Android and anyone else skipping
	// cmake, where the proper sizes are checked for
	#ifndef SIZEOF_INT
	#define SIZEOF_INT 4
	#endif
	#ifndef SIZEOF_LONG_INT
	#define SIZEOF_LONG_INT 4
	#endif
#endif

#include "exports.h"

#ifndef _MAX_PATH
	#ifdef WIN32
		#define _MAX_PATH 260
	#else
		#define _MAX_PATH FILENAME_MAX
	#endif
#endif

#ifdef WIN32
	#include "win32def.h"
#elif defined(HAVE_UNISTD_H)
	#include <unistd.h>
#endif

#include <cstdio>
#include <cstdlib>

#include "System/Logging.h"
#include "System/String.h"

#endif  //! PLATFORM_H
