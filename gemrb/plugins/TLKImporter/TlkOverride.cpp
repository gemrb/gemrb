/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003-2007 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 * TlkOverride.cpp: implementation of the saved game specific
 * (dynamic) part of the talk table (tlk)
 */

#include "TlkOverride.h"

#include "Interface.h"

#include "Logging/Logging.h"
#include "Streams/FileStream.h"

#include <algorithm>
#include <cassert>

using namespace GemRB;

CTlkOverride::~CTlkOverride()
{
	CloseResources();
}

bool CTlkOverride::Init()
{
	CloseResources();
	//Creation of the headers should be game specific, some games don't have these
	toh_str = GetAuxHdr(true);
	if (!toh_str) {
		return false;
	}
	tot_str = GetAuxTlk(true);
	if (!tot_str) {
		return false;
	}

	char Signature[8];

	memset(Signature, 0, 8);
	toh_str->Read(Signature, 4);
	if (strncmp(Signature, "TLK ", 4) != 0) {
		Log(ERROR, "TLKImporter", "Not a valid TOH file.");
		return false;
	}
	toh_str->Seek(8, GEM_CURRENT_POS);
	toh_str->ReadDword(AuxCount);
	if (tot_str->ReadScalar<strpos_t, int32_t>(FreeOffset) != 4) {
		FreeOffset = DataStream::InvalidPos;
	}
	NextStrRef = DataStream::InvalidPos;

	return true;
}

void CTlkOverride::CloseResources()
{
	if (toh_str) {
		delete toh_str;
		toh_str = nullptr;
	}
	if (tot_str) {
		delete tot_str;
		tot_str = nullptr;
	}
}

//gets the length of a stored string which might span more than one segment
strret_t CTlkOverride::GetLength(strpos_t offset)
{
	strpos_t tmp = offset;
	char buffer[SEGMENT_SIZE];

	if (tot_str->Seek(offset + 8, GEM_STREAM_START) != GEM_OK) {
		return 0;
	}

	strret_t length = 0;
	do {
		if (tot_str->Seek(tmp + 8, GEM_STREAM_START) != GEM_OK) {
			return 0;
		}
		memset(buffer, 0, sizeof(buffer));
		tot_str->Read(buffer, SEGMENT_SIZE);
		tot_str->ReadScalar<strpos_t, int32_t>(tmp);
		if (tmp != DataStream::InvalidPos) {
			length += SEGMENT_SIZE;
		}
	} while (tmp != DataStream::InvalidPos);
	buffer[SEGMENT_SIZE - 1] = 0;
	length += strlen(buffer);
	return length;
}

//returns a string stored at a given offset of the .tot file
char* CTlkOverride::GetString(strpos_t offset)
{
	if (!tot_str) {
		return nullptr;
	}

	strret_t length = GetLength(offset);
	if (length == 0) {
		return nullptr;
	}

	//assuming char is one byte
	char* ret = (char*) malloc(length + 1);
	char* pos = ret;
	ret[length] = 0;
	while (length) {
		tot_str->Seek(offset + 8, GEM_STREAM_START);
		strret_t tmp = std::min<strret_t>(length, SEGMENT_SIZE);
		tot_str->Read(pos, tmp);
		tot_str->Seek(SEGMENT_SIZE - tmp, GEM_CURRENT_POS);
		tot_str->ReadScalar<strpos_t, int32_t>(offset);
		length -= tmp;
		pos += tmp;
	}
	return ret;
}

ieStrRef CTlkOverride::UpdateString(ieStrRef strref, const String& string)
{
	ieDword memoffset = 0;
	strpos_t offset = LocateString(strref);

	if (offset == DataStream::InvalidPos) {
		strref = GetNewStrRef(strref);
		offset = LocateString(strref);
		assert(strref != ieStrRef::INVALID);
	}

	std::string newvalue = TLKStringFromString(string);
	size_t length = std::min<size_t>(newvalue.length(), std::numeric_limits<uint16_t>::max());

	//set the backpointer of the first string segment
	strpos_t backp = DataStream::InvalidPos;

	do {
		//fill the backpointer
		tot_str->Seek(offset + 4, GEM_STREAM_START);
		tot_str->WriteScalar<strpos_t, int32_t>(backp);
		size_t seglen = std::min<size_t>(SEGMENT_SIZE, length);
		tot_str->Write(newvalue.data() + memoffset, seglen);
		length -= seglen;
		memoffset += seglen;
		backp = offset;
		tot_str->Seek(SEGMENT_SIZE - seglen, GEM_CURRENT_POS);
		tot_str->ReadScalar<strpos_t, int32_t>(offset);

		//end of string
		if (!length) {
			if (offset != DataStream::InvalidPos) {
				strpos_t freep = offset;
				offset = DataStream::InvalidPos;
				tot_str->Seek(-4, GEM_CURRENT_POS);
				tot_str->WriteScalar<strpos_t, int32_t>(offset);
				ReleaseSegment(freep);
			}
			break;
		}

		if (offset == DataStream::InvalidPos) {
			//no more space, but we need some
			offset = ClaimFreeSegment();
			tot_str->Seek(-4, GEM_CURRENT_POS);
			tot_str->WriteScalar<strpos_t, int32_t>(offset);
		}
	} while (length);

	return strref;
}

strpos_t CTlkOverride::ClaimFreeSegment()
{
	strpos_t offset = FreeOffset;
	strpos_t pos = tot_str->GetPos();

	if (offset == DataStream::InvalidPos) {
		offset = tot_str->Size();
	} else {
		tot_str->Seek(offset, GEM_STREAM_START);
		if (tot_str->ReadScalar<strpos_t, int32_t>(FreeOffset) != 4) {
			FreeOffset = DataStream::InvalidPos;
		}
	}
	ieDword tmp = 0;
	tot_str->Seek(offset, GEM_STREAM_START);
	tot_str->WriteDword(tmp);
	tmp = 0xffffffff;
	tot_str->WriteDword(tmp);
	tot_str->WriteFilling(SEGMENT_SIZE);
	tot_str->WriteDword(tmp);

	//update free segment pointer
	tot_str->Seek(0, GEM_STREAM_START);
	tot_str->WriteScalar<strpos_t, int32_t>(FreeOffset);
	tot_str->Seek(pos, GEM_STREAM_START);
	return offset;
}

void CTlkOverride::ReleaseSegment(strpos_t offset)
{
	// also release linked segments, if any
	do {
		tot_str->Seek(offset, GEM_STREAM_START);
		tot_str->WriteScalar<strpos_t, int32_t>(FreeOffset);
		FreeOffset = offset;
		tot_str->Seek(SEGMENT_SIZE + 4, GEM_CURRENT_POS);
		tot_str->ReadScalar<strpos_t, int32_t>(offset);
	} while (offset != DataStream::InvalidPos);
	tot_str->Seek(0, GEM_STREAM_START);
	tot_str->WriteScalar<strpos_t, int32_t>(FreeOffset);
}

ieStrRef CTlkOverride::GetNextStrRef()
{
	if (NextStrRef == DataStream::InvalidPos) {
		// find the largest entry; should be the last - unless we
		// overwrote internal strings, or biographies
		ieDword last = 0;
		ieDword end = ieDword(ieStrRef::OVERRIDE_START);
		int cnt = AuxCount;

		while (--cnt >= 0 && last < end) {
			if (toh_str->Seek(TOH_HEADER_SIZE + EntryType::FileSize * cnt, GEM_STREAM_START) != GEM_OK) {
				// looks like the file is damaged
				AuxCount--;
				continue;
			}
			toh_str->ReadDword(last);
		}
		NextStrRef = std::max<ieDword>(end, ++last);
	}
	return ieStrRef(NextStrRef++);
}

ieStrRef CTlkOverride::GetNewStrRef(ieStrRef strref)
{
	EntryType entry;

	if (strref >= ieStrRef::BIO_START && strref <= ieStrRef::BIO_END) {
		entry.strref = strref;
	} else {
		entry.strref = GetNextStrRef();
	}
	entry.offset = ClaimFreeSegment();

	toh_str->Seek(TOH_HEADER_SIZE + AuxCount * EntryType::FileSize, GEM_STREAM_START);
	toh_str->WriteStrRef(entry.strref);
	toh_str->WriteDword(entry.flags);
	toh_str->WriteResRef(entry.soundRef);
	toh_str->WriteDword(entry.volumeVariance);
	toh_str->WriteDword(entry.pitchVariance);
	toh_str->WriteScalar<strpos_t, int32_t>(entry.offset);
	AuxCount++;
	toh_str->Seek(12, GEM_STREAM_START);
	toh_str->WriteDword(AuxCount);
	return entry.strref;
}

strpos_t CTlkOverride::LocateString(ieStrRef strref)
{
	ieStrRef strref2;
	ieDword offset;

	if (!toh_str) return DataStream::InvalidPos;
	toh_str->Seek(TOH_HEADER_SIZE, GEM_STREAM_START);
	for (ieDword i = 0; i < AuxCount; i++) {
		toh_str->ReadStrRef(strref2);
		toh_str->Seek(20, GEM_CURRENT_POS);
		toh_str->ReadDword(offset);
		if (strref2 == strref) {
			return offset;
		}
	}
	return DataStream::InvalidPos;
}

char* CTlkOverride::ResolveAuxString(ieStrRef strref, size_t& Length)
{
	char* string = nullptr;
	strpos_t offset = LocateString(strref);
	if (offset != DataStream::InvalidPos) {
		string = GetString(offset);
	}
	if (string != NULL) {
		Length = strlen(string);
	} else {
		Length = 0;
		string = (char*) malloc(1);
		string[0] = 0;
	}

	return string;
}

DataStream* CTlkOverride::GetAuxHdr(bool create)
{
	char Signature[] = "TLK ";

	path_t nPath = PathJoin(core->config.CachePath, "default.toh");
	FileStream* fs = new FileStream();

	while (true) {
		if (fs->Modify(nPath)) {
			return fs;
		}
		if (create) {
			fs->Create("default", IE_TOH_CLASS_ID);
			fs->Write(Signature, 4);
			fs->WriteFilling(TOH_HEADER_SIZE - 4);
			create = false;
			continue;
		}
		delete fs;
		return nullptr;
	}
}

DataStream* CTlkOverride::GetAuxTlk(bool create)
{
	path_t nPath = PathJoin(core->config.CachePath, "default.tot");
	FileStream* fs = new FileStream();

	while (true) {
		if (fs->Modify(nPath)) {
			if (fs->Size() % (SEGMENT_SIZE + 12) == 0) {
				return fs;
			}

			Log(ERROR, "TLKImporter", "Defective default.tot detected. Discarding.");
			// if this happens we also need to account for the TOH file
			AuxCount = 0;
			if (toh_str->Seek(12, GEM_STREAM_START) == GEM_OK) {
				toh_str->WriteDword(AuxCount);
			}
			toh_str->Rewind();
		}

		if (create) {
			fs->Create("default", IE_TOT_CLASS_ID);
			create = false;
			continue;
		}
		delete fs;
		return nullptr;
	}
}
