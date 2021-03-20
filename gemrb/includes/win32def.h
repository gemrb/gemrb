/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
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

// TODO: move this to platforms/windows

#ifndef WIN32DEF_H
#define WIN32DEF_H

#define WIN32_LEAN_AND_MEAN

#ifndef NOMINMAX
#define NOMINMAX
#endif

#define UNICODE
#define _UNICODE
#define NOCOLOR
#define NOGDI
#define _USE_MATH_DEFINES

#ifdef _DEBUG
#include <crtdbg.h>
#endif

#include <windows.h>
#ifndef M_PI_4
#define 	M_PI_4   0.78539816339744830962
#endif

 /* WinAPI collision. GetObject from wingdi.h conflicts with a type used in the GUIScript source (core\ScriptEngine.h) */
#ifdef GetObject
#undef GetObject
#endif

#endif  //! WIN32DEF_H
