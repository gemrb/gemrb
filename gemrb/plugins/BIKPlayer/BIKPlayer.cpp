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
 * $Id: BIKPlayer.cpp 6005 2009-05-19 10:21:38Z lynxlupodian $
 *
 */

// BIKPlayer.cpp : Defines the entry point for the DLL application.
//

#include "../../includes/win32def.h"
#include "BIKPlayerDesc.h"
#include "../../includes/globals.h"

#ifdef WIN32
#include <windows.h>

BOOL APIENTRY DllMain(HANDLE hModule, DWORD  ul_reason_for_call,
	LPVOID lpReserved)
{
	return TRUE;
}

#endif

GEM_EXPORT_DLL ClassDesc* LibClassDesc()
{
	return &BIKPlayCD;
}

GEM_EXPORT_DLL const char* LibDescription()
{
	return "BIK Video Player";
}

GEM_EXPORT_DLL const char* LibVersion()
{
	return VERSION_GEMRB;
}
