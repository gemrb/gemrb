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
// TlkOverride.h: interface for the CTlkOverride class.
//
//////////////////////////////////////////////////////////////////////

#ifndef TLKOVERRIDE_H
#define TLKOVERRIDE_H

#include "globals.h"

#include "System/FileStream.h"
#include "System/String.h"

#ifdef CACHE_TLK_OVERRIDE
#include <map>

using StringMapType = std::map<ieStrRef, char *>;
#endif

namespace GemRB {

constexpr strret_t SEGMENT_SIZE = 512;
#define TOH_HEADER_SIZE 20

struct EntryType
{
	ieStrRef strref;
	ieByte dummy[20];
	strpos_t offset;
	
	static constexpr strpos_t FileSize = 28; // size in bytes for this structure in the TLK file
};

class CTlkOverride  
{
private:
#ifdef CACHE_TLK_OVERRIDE
	StringMapType stringmap;
#endif
	DataStream* tot_str = nullptr;
	DataStream* toh_str = nullptr;
	ieDword AuxCount = 0;
	strpos_t FreeOffset = DataStream::InvalidPos;
	strpos_t NextStrRef = DataStream::InvalidPos;

	void CloseResources();
	DataStream *GetAuxHdr(bool create);
	DataStream *GetAuxTlk(bool create);
	ieStrRef GetNewStrRef(ieStrRef strref);
	strpos_t LocateString(ieStrRef strref);
	ieStrRef GetNextStrRef();
	strpos_t ClaimFreeSegment();
	void ReleaseSegment(strpos_t offset);
	char *GetString(strpos_t offset);
	strret_t GetLength(strpos_t offset);
public:
	CTlkOverride() = default;
	virtual ~CTlkOverride();

	bool Init();
	char *ResolveAuxString(ieStrRef strref, size_t &Length);
	ieStrRef UpdateString(ieStrRef strref, const String& newvalue);
};

#endif //TLKOVERRIDE_H
}


