// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

// TlkOverride.h: interface for the CTlkOverride class.
//
//////////////////////////////////////////////////////////////////////

#ifndef TLKOVERRIDE_H
#define TLKOVERRIDE_H

#include "strrefs.h"

#include "Streams/DataStream.h"
#include "Strings/String.h"

namespace GemRB {

constexpr strret_t SEGMENT_SIZE = 512;
#define TOH_HEADER_SIZE 20

// this mimics normal TLK entries with a bunch of unused / not useful fields
// the originals passed it around whole, but since custom entries were never
// used outside the UI, the flags and sound fields don't matter
struct EntryType {
	ieStrRef strref = ieStrRef::INVALID;
	ieDword flags = 0;
	ResRef soundRef;
	ieDword volumeVariance = 0;
	ieDword pitchVariance = 0;
	strpos_t offset = 0;

	static constexpr strpos_t FileSize = 28; // size in bytes for this structure in the TLK file
};

class CTlkOverride {
private:
	std::unique_ptr<DataStream> tot_str;
	std::unique_ptr<DataStream> toh_str;
	ieDword AuxCount = 0;
	strpos_t FreeOffset = DataStream::InvalidPos;
	strpos_t NextStrRef = DataStream::InvalidPos;

	void CloseResources();
	std::unique_ptr<DataStream> GetAuxHdr(bool create);
	std::unique_ptr<DataStream> GetAuxTlk(bool create);
	ieStrRef GetNewStrRef(ieStrRef strref);
	strpos_t LocateString(ieStrRef strref);
	strpos_t ClaimFreeSegment();
	void ReleaseSegment(strpos_t offset);
	char* GetString(strpos_t offset);
	strret_t GetLength(strpos_t offset);

public:
	CTlkOverride() noexcept = default;
	CTlkOverride(const CTlkOverride&) = delete;
	CTlkOverride& operator=(const CTlkOverride&) = delete;

	bool Init();
	char* ResolveAuxString(ieStrRef strref, size_t& Length);
	ieStrRef UpdateString(ieStrRef strref, const String& newvalue);
	ieStrRef GetNextStrRef();
};

}

#endif //TLKOVERRIDE_H
