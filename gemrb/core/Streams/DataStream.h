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

/**
 * @file DataStream.h
 * Declares DataStream, abstract class for reading and writing data.
 * @author The GemRB Project
 */


#ifndef DATASTREAM_H
#define DATASTREAM_H

#include "Region.h"

#include "Strings/CString.h"
#include "Strings/String.h"
#include "System/VFS.h"
#include "System/swab.h"

namespace GemRB {

#define GEM_CURRENT_POS  0
#define GEM_STREAM_START 1
#define GEM_STREAM_END   2

/**
 * @class DataStream
 * Abstract base for streams, classes for reading and writing data.
 */

#define ReadWord             ReadScalar<ieWord>
#define ReadDword            ReadScalar<ieDword>
#define ReadStrRef           ReadEnum<ieStrRef>
#define ReadResRef(str)      ReadRTrimString(str, str.Size)
#define ReadVariable(str)    ReadRTrimString(str, str.Size)
#define ReadArray(arr)       Read(arr.data(), arr.size() * sizeof(decltype(arr)::value_type))
#define WriteWord            WriteScalar<ieWord>
#define WriteDword           WriteScalar<ieDword>
#define WriteStrRef          WriteEnum<ieStrRef>
#define WriteResRef(str)     WriteString(str, str.Size)
#define WriteResRefLC(str)   WriteStringLC(str, str.Size)
#define WriteResRefUC(str)   WriteStringUC(str, str.Size)
#define WriteVariable(str)   WriteString(str, str.Size)
#define WriteVariableLC(str) WriteStringLC(str, str.Size)
#define WriteVariableUC(str) WriteStringUC(str, str.Size)

// represents the byte position of the stream
using strpos_t = size_t;
// represents the number of bytes read or written. -1 on error
using strret_t = std::make_signed_t<strpos_t>;
// represents on offset to a strpos_t
using stroff_t = std::make_signed_t<strpos_t>;

class GEM_EXPORT DataStream {
public:
	FixedSizeString<16> filename; //8+1+3+1 padded to dword
	path_t originalfile;

public:
	static constexpr strpos_t InvalidPos = strpos_t(-1);
	static constexpr strret_t Error = -1;

	DataStream() noexcept = default;
	virtual ~DataStream() noexcept = default;

	DataStream(const DataStream&) = delete;
	DataStream& operator=(const DataStream&) = delete;

	// FIXME: these should wrap private implementations
	// so that we can properly swab the read data
	// if (NeedEndianSwap()) swabs(..., len);
	virtual strret_t Read(void* dest, strpos_t len) = 0;
	virtual strret_t Write(const void* src, strpos_t len) = 0;

	template<typename T>
	strret_t ReadScalar(T& dest)
	{
		strret_t len = Read(&dest, sizeof(T));
		if (NeedEndianSwap()) {
			swabs(&dest, sizeof(T));
		}
		return len;
	}

	template<typename DST, typename SRC>
	strret_t ReadScalar(DST& dest)
	{
		static_assert(sizeof(DST) >= sizeof(SRC), "This flavor of ReadScalar requires DST to be >= SRC.");
		SRC src;
		strret_t len = ReadScalar(src);
		dest = src; // preserve sign extension
		return len;
	}

	template<typename ENUM>
	std::enable_if_t<std::is_enum<ENUM>::value, strret_t>
		ReadEnum(ENUM& dest)
	{
		std::underlying_type_t<ENUM> scalar;
		strret_t ret = ReadScalar(scalar);
		dest = static_cast<ENUM>(scalar);
		return ret;
	}

	template<typename T>
	strret_t WriteScalar(const T& src)
	{
		strret_t len;
		if (NeedEndianSwap()) {
			T tmp;
			swab_const(&src, &tmp, sizeof(T));
			len = Write(&tmp, sizeof(T));
		} else {
			len = Write(&src, sizeof(T));
		}
		return len;
	}

	template<typename SRC, typename DST>
	strret_t WriteScalar(const SRC& src)
	{
		static_assert(sizeof(SRC) >= sizeof(DST), "This flavor of WriteScalar requires SRC to be >= DST.");
		DST dst = static_cast<DST>(src);
		return WriteScalar<DST>(dst);
	}

	template<typename ENUM>
	std::enable_if_t<std::is_enum<ENUM>::value, strret_t>
		WriteEnum(const ENUM& dest)
	{
		return WriteScalar(static_cast<std::underlying_type_t<ENUM>>(dest));
	}

	strret_t WriteFilling(strpos_t len);

	// NOTE: RTrim doesn't cut it when reading text files, since we may accidentally read more than one line
	// so in those situations rather use ReadLine and then convert to desired string type
	template<typename STR>
	strret_t ReadRTrimString(STR& dest, size_t len)
	{
		strret_t read = Read(dest.begin(), len);
		RTrim(dest);
		return read;
	}

	template<typename STR>
	strret_t WriteString(const STR& src)
	{
		auto beg = std::begin(src);
		return Write(beg, std::end(src) - beg);
	}

	template<typename STR>
	strret_t WriteString(const STR& src, size_t len)
	{
		return Write(src.c_str(), len);
	}

	template<typename STR>
	strret_t WriteStringLC(STR src, size_t len)
	{
		StringToLower(src.begin(), src.begin() + len, src.begin());
		return WriteString<STR>(src, len);
	}

	template<typename STR>
	strret_t WriteStringUC(STR src, size_t len)
	{
		StringToUpper(src.begin(), src.begin() + len, src.begin());
		return WriteString<STR>(src, len);
	}

	strret_t ReadPoint(BasePoint&);
	strret_t WritePoint(const BasePoint&);
	strret_t ReadSize(class Size&);
	strret_t ReadRegion(Region&, bool asPoints = false);

	virtual stroff_t Seek(stroff_t pos, strpos_t startpos) = 0;
	strpos_t Remains() const;
	strpos_t Size() const;
	strpos_t GetPos() const;
	void Rewind();
	/** Returns true if the stream is encrypted */
	bool CheckEncrypted();
	strret_t ReadLine(std::string& buf, strpos_t maxlen = 0);
	/** Create a copy of this stream.
	 *
	 *  Returns NULL on failure.
	 **/
	virtual DataStream* Clone() const noexcept;

	void SetBigEndianness(bool) noexcept;

protected:
	strpos_t Pos = 0;
	strpos_t size = 0;
	bool Encrypted = false;
	bool IsDataBigEndian = false;

	void ReadDecrypted(void* buf, strpos_t encSize) const;

private:
	bool NeedEndianSwap() const noexcept;
};

}

#endif // ! DATASTREAM_H
