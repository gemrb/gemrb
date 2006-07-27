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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/WEDImporter/WEDImporter.cpp,v 1.6 2006/07/27 17:15:01 avenger_teambg Exp $
 *
 */

// WEDImporter.cpp : Defines the entry point for the DLL application.
//

#include "../../includes/globals.h"
#include "WEDImpCD.h"

/*
#ifdef WIN32
#define GEM_EXPORT_DLL __declspec(dllexport)
#else
#define GEM_EXPORT_DLL
#endif
*/
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
	return &WedImpCD;
}

GEM_EXPORT_DLL const char* LibDescription()
{
	return "WED File Importer";
}

GEM_EXPORT_DLL const char* LibVersion()
{
	return VERSION_GEMRB;
}
