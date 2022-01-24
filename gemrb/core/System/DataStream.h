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

#include "Platform.h"
#include "Region.h"
#include "System/swab.h"

namespace GemRB {

#define GEM_CURRENT_POS 0
#define GEM_STREAM_START 1
#define GEM_STREAM_END 2

/**
 * @class DataStream
 * Abstract base for streams, classes for reading and writing data.
 */

#define ReadWord ReadScalar<ieWord>
#define ReadDword ReadScalar<ieDword>
#define WriteWord WriteScalar<ieWord>
#define WriteDword WriteScalar<ieDword>

// represents the byte position of the stream
using strpos_t = size_t;
// represents the number of bytes read or written. -1 on error
using strret_t = typename std::make_signed<strpos_t>::type;
// represents on offset to a strpos_t
using stroff_t = typename std::make_signed<strpos_t>::type;

class GEM_EXPORT DataStream {
public:
	char filename[16]; //8+1+3+1 padded to dword
	char originalfile[_MAX_PATH];
public:
	DataStream(void) = default;
	virtual ~DataStream() = default;
	
	DataStream(const DataStream&) = delete;
	DataStream& operator=(const DataStream&) = delete;
	
	virtual strret_t Read(void* dest, strpos_t len) = 0;
	virtual strret_t Write(const void* src, strpos_t len) = 0;
	
	template <typename T>
	strret_t ReadScalar(T& dest) {
		strret_t len = Read(&dest, sizeof(T));
		if (IsBigEndian) {
			swabs(&dest, sizeof(T));
		}
		return len;
	}
	
	template <typename DST, typename SRC>
	strret_t ReadScalar(DST& dest) {
		static_assert(sizeof(DST) >= sizeof(SRC), "This flavor of ReadScalar requires DST to be >= SRC.");
		SRC src;
		strret_t len = ReadScalar(src);
		dest = src; // preserve sign extension
		return len;
	}
	
	template <typename ENUM>
	typename std::enable_if<std::is_enum<ENUM>::value, strret_t>::type
	ReadEnum(ENUM& dest) {
		typename std::underlying_type<ENUM>::type scalar;
		strret_t ret = ReadScalar(scalar);
		dest = static_cast<ENUM>(scalar);
		return ret;
	}
	
	template <typename T>
	strret_t WriteScalar(const T& src) {
		strret_t len;
		if (IsBigEndian) {
			T tmp;
			swab_const(&src, &tmp, sizeof(T));
			len = Write(&tmp, sizeof(T));
		} else {
			len = Write(&src, sizeof(T));
		}
		return len;
	}
	
	template <typename SRC, typename DST>
	strret_t WriteScalar(const SRC& src) {
		static_assert(sizeof(SRC) >= sizeof(DST), "This flavor of WriteScalar requires SRC to be >= DST.");
		DST dst = static_cast<DST>(src);
		return WriteScalar<DST>(dst);
	}
	
	template <typename ENUM>
	typename std::enable_if<std::is_enum<ENUM>::value, strret_t>::type
	WriteEnum(const ENUM& dest) {
		return WriteScalar(static_cast<typename std::underlying_type<ENUM>::type>(dest));
	}

	strret_t ReadResRef(ResRef& dest);
	strret_t WriteResRef(const ResRef& src);
	strret_t WriteResRefLC(const ResRef& src);
	strret_t WriteResRefUC(const ResRef& src);

	strret_t ReadVariable(ieVariable& dest);
	strret_t WriteVariable(const ieVariable& src);

	strret_t ReadPoint(Point&);
	strret_t WritePoint(const Point&);
	
	virtual stroff_t Seek(stroff_t pos, strpos_t startpos) = 0;
	strpos_t Remains() const;
	strpos_t Size() const;
	strpos_t GetPos() const;
	void Rewind();
	/** Returns true if the stream is encrypted */
	bool CheckEncrypted();
	void ReadDecrypted(void* buf, strpos_t encSize) const;
	strret_t ReadLine(void* buf, strpos_t maxlen);
	/** Endian Switch setup */
	static void SetBigEndian(bool be);
	static bool BigEndian();
	/** Create a copy of this stream.
	 *
	 *  Returns NULL on failure.
	 **/
	virtual DataStream* Clone() const noexcept;
protected:
	strpos_t Pos = 0;
	strpos_t size = 0;
	bool Encrypted = false;

	static bool IsBigEndian;
};

}

#endif  // ! DATASTREAM_H
