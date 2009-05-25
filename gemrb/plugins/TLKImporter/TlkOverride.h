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
 * $Id$
 *
 */
// TlkOverride.h: interface for the CTlkOverride class.
//
//////////////////////////////////////////////////////////////////////

#ifndef TLKOVERRIDE_H
#define TLKOVERRIDE_H

#include "../../includes/globals.h"
#include "../Core/Interface.h"
#include "../Core/FileStream.h"
#ifdef CACHE_TLK_OVERRIDE
#include <map>

typedef std::map<ieStrRef, char *> StringMapType;
#endif

class CTlkOverride  
{
private:
#ifdef CACHE_TLK_OVERRIDE
	StringMapType stringmap;
#endif
	DataStream *tot_str;
	DataStream *toh_str;
	ieDword AuxCount;

	void CloseResources();
	DataStream *GetAuxHdr();
	DataStream *GetAuxTlk();
	char* LocateString2(ieDword offset);
	char* LocateString(ieStrRef strref);
	ieDword GetLength();
public:
	CTlkOverride();
	virtual ~CTlkOverride();

	char *CS(const char *src);
	bool Init();
	char *ResolveAuxString(ieStrRef strref, int &Length);
};

#endif //TLKOVERRIDE_H

