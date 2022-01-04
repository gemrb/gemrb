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

#include "System/DataStream.h"

#include "errors.h"
#include "Resource.h"

#include <cctype>

namespace GemRB {

static const char* const GEM_ENCRYPTION_KEY = "\x88\xa8\x8f\xba\x8a\xd3\xb9\xf5\xed\xb1\xcf\xea\xaa\xe4\xb5\xfb\xeb\x82\xf9\x90\xca\xc9\xb5\xe7\xdc\x8e\xb7\xac\xee\xf7\xe0\xca\x8e\xea\xca\x80\xce\xc5\xad\xb7\xc4\xd0\x84\x93\xd5\xf0\xeb\xc8\xb4\x9d\xcc\xaf\xa5\x95\xba\x99\x87\xd2\x9d\xe3\x91\xba\x90\xca";

const static ieWord endiantest = 1;
bool DataStream::IsBigEndian = ((char *)&endiantest)[1] == 1;

void DataStream::SetBigEndian(bool be)
{
	DataStream::IsBigEndian = be;
}

bool DataStream::BigEndian()
{
	return DataStream::IsBigEndian;
}

/** Returns true if the stream is encrypted */
bool DataStream::CheckEncrypted()
{
	ieWord two = 0x0000; // if size < 2, two won't be initialized
	Seek( 0, GEM_STREAM_START );
	Read( &two, 2 );
	if (two == 0xFFFF) {
		Pos = 0;
		Encrypted = true;
		size -= 2;
		return true;
	}
	Seek( 0, GEM_STREAM_START );
	Encrypted = false;
	return false;
}
/** No descriptions */
void DataStream::ReadDecrypted(void* buf, strpos_t encSize) const
{
	for (unsigned int i = 0; i < encSize; i++)
		( ( unsigned char * ) buf )[i] ^= GEM_ENCRYPTION_KEY[( Pos + i ) & 63];
}

void DataStream::Rewind()
{
	Seek( Encrypted ? 2 : 0, GEM_STREAM_START );
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
	return size-Pos;
}

strret_t DataStream::ReadResRef(ResRef& dest)
{
	char ref[9];
	strret_t len = Read(ref, 8);
	ref[len] = '\0';
	
	// remove trailing spaces
	for (strret_t i = len - 1; i >= 0; --i) {
		if (ref[i] == ' ') ref[i] = '\0';
		else break;
	}
	
	dest = MakeLowerCaseResRef(ref);
	return len;
}

strret_t DataStream::WriteResRef(const ResRef& src)
{
	return Write(src.CString(), 8);
}

strret_t DataStream::WriteResRefLC(const ResRef& src)
{
	return WriteResRef(MakeLowerCaseResRef(src.CString()));
}

strret_t DataStream::WriteResRefUC(const ResRef& src)
{
	return WriteResRef(MakeUpperCaseResRef(src.CString()));
}

strret_t DataStream::ReadVariable(ieVariable& dest)
{
	char ref[33];
	strret_t len = Read(ref, 32);
	ref[len] = '\0';

	// remove trailing spaces
	for (strret_t i = len - 1; i >= 0; --i) {
		if (ref[i] == ' ') ref[i] = '\0';
		else break;
	}

	dest = ref;
	return len;
}

strret_t DataStream::WriteVariable(const ieVariable& src)
{
	return Write(src.CString(), 32);
}

strret_t DataStream::ReadPoint(Point &p)
{
	// in the data files Points are 16bit per coord as opposed to our 32ish
	strret_t ret = ReadScalar<int, ieWord>(p.x);
	ret += ReadScalar<int, ieWord>(p.y);
	return ret;
}

strret_t DataStream::WritePoint(const Point &p)
{
	// in the data files Points are 16bit per coord as opposed to our 32ish
	strret_t ret = WriteScalar<int, ieWord>(p.x);
	ret += WriteScalar<int, ieWord>(p.y);
	return ret;
}

strret_t DataStream::ReadLine(void* buf, strpos_t maxlen)
{
	// FIXME: eof?
	if (!maxlen) {
		return 0;
	}
	unsigned char * p = ( unsigned char * ) buf;
	if (Pos >= size) {
		p[0]=0;
		return -1;
	}
	unsigned int i = 0;
	//TODO: fix this to handle any combination of \r and \n
	//Windows: \r\n
	//Old Mac: \r
	//otherOS: \n
	while (i < ( maxlen - 1 )) {
		char ch;
		Read(&ch, 1);
		if (ch == '\n')
			break;
		if (ch == '\t')
			ch = ' ';
		if (ch != '\r')
			p[i++] = ch;
		if (Pos == size)
			break;
	}
	p[i] = 0;
	return i;
}

DataStream* DataStream::Clone()
{
	return NULL;
}

}
