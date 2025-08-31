/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2025 The GemRB Project
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

#ifndef ATTRIBUTES_H
#define ATTRIBUTES_H

#ifdef HAVE_CONFIG_H
	#include "config.h"
#endif

// Semantic warning macros
#ifdef HAVE_ATTRIBUTE_WARN_UNUSED_RESULT
	#define WARN_UNUSED __attribute__((warn_unused_result))
#else
	#define WARN_UNUSED
#endif

// Silence some persistent unused warnings (supported since gcc 2.4)
#ifdef HAVE_ATTRIBUTE_UNUSED
	#define IGNORE_UNUSED __attribute__((unused))
#else
	#define IGNORE_UNUSED
#endif

#endif
