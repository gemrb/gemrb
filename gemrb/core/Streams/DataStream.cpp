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

#include "DataStream.h"

#include "ie_types.h"

namespace GemRB {

static const char* const GEM_ENCRYPTION_KEY = "\x88\xa8\x8f\xba\x8a\xd3\xb9\xf5\xed\xb1\xcf\xea\xaa\xe4\xb5\xfb\xeb\x82\xf9\x90\xca\xc9\xb5\xe7\xdc\x8e\xb7\xac\xee\xf7\xe0\xca\x8e\xea\xca\x80\xce\xc5\xad\xb7\xc4\xd0\x84\x93\xd5\xf0\xeb\xc8\xb4\x9d\xcc\xaf\xa5\x95\xba\x99\x87\xd2\x9d\xe3\x91\xba\x90\xca";

bool DataStream::NeedEndianSwap() const noexcept
{
	return IsBigEndian() != IsDataBigEndian;
}

/** Returns true if the stream is encrypted */
bool DataStream::CheckEncrypted()
{
	ieWord two = 0x0000; // if size < 2, two won't be initialized
	Seek(0, GEM_STREAM_START);
	Read(&two, 2);
	if (two == 0xFFFF) {
		Pos = 0;
		Encrypted = true;
		size -= 2;
		return true;
	}
	Seek(0, GEM_STREAM_START);
	Encrypted = false;
	return false;
}
/** No descriptions */
void DataStream::ReadDecrypted(void* buf, strpos_t encSize) const
{
	for (unsigned int i = 0; i < encSize; i++)
		((unsigned char*) buf)[i] ^= GEM_ENCRYPTION_KEY[(Pos + i) & 63];
}

void DataStream::Rewind()
{
	Seek(Encrypted ? 2 : 0, GEM_STREAM_START);
	Pos = 0;
}

strpos_t DataStream::GetPos() const
{
	return Pos;
}

strpos_t DataStream::Size() const
{
	return size;
}

strpos_t DataStream::Remains() const
{
	return size - Pos;
}

strret_t DataStream::WriteFilling(strpos_t len)
{
	const static char zerobuf[256] = {};

	strret_t ret = 0;
	while (len >= sizeof(zerobuf)) {
		ret += Write(zerobuf, sizeof(zerobuf));
		len -= sizeof(zerobuf);
	}

	ret += Write(zerobuf, len);
	return ret;
}

strret_t DataStream::ReadPoint(BasePoint& p)
{
	// in the data files Points are 16bit per coord as opposed to our 32ish
	strret_t ret = ReadScalar<int, ieWordSigned>(p.x);
	ret += ReadScalar<int, ieWordSigned>(p.y);
	return ret;
}

strret_t DataStream::WritePoint(const BasePoint& p)
{
	// in the data files Points are 16bit per coord as opposed to our 32ish
	strret_t ret = WriteScalar<int, ieWordSigned>(p.x);
	ret += WriteScalar<int, ieWordSigned>(p.y);
	return ret;
}

strret_t DataStream::ReadSize(class Size& s)
{
	strret_t ret = ReadScalar<int, ieWord>(s.w);
	ret += ReadScalar<int, ieWord>(s.h);
	return ret;
}

strret_t DataStream::ReadRegion(Region& r, bool asPoints)
{
	strret_t ret = ReadPoint(r.origin);
	ret += ReadSize(r.size);
	if (asPoints) { // size is really the "max" coord
		r.w -= r.x;
		r.h -= r.y;
	}
	return ret;
}

strret_t DataStream::ReadLine(std::string& buf, strpos_t maxlen)
{
	if (Pos >= size) {
		return Error;
	}

	buf.clear();
	buf.reserve(maxlen);
	strpos_t i = 0;

	while (Pos < size && i < (maxlen - 1)) {
		char ch;
		i += Read(&ch, 1);

		if (ch == '\r') {
			i += Read(&ch, 1);

			if (ch == '\n') {
				break;
			} else {
				i -= 1;
				Seek(-1, GEM_CURRENT_POS);
				break;
			}
		} else if (ch == '\n') {
			break;
		}

		if (ch == '\t') {
			ch = ' ';
		}

		buf.push_back(ch);
	}

	return i;
}

DataStream* DataStream::Clone() const noexcept
{
	return NULL;
}

void DataStream::SetBigEndianness(bool isBE) noexcept
{
	IsDataBigEndian = isBE;
}

}
