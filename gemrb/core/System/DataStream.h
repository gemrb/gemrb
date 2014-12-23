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

#include "exports.h"
#include "globals.h"

namespace GemRB {

#define GEM_CURRENT_POS 0
#define GEM_STREAM_START 1
#define GEM_STREAM_END 2

/**
 * @class DataStream
 * Abstract base for streams, classes for reading and writing data.
 */

class GEM_EXPORT DataStream {
protected:
	unsigned long Pos;
	unsigned long size;
	bool Encrypted;

	static bool IsBigEndian;
public:
	char filename[16]; //8+1+3+1 padded to dword
	char originalfile[_MAX_PATH];
public:
	DataStream(void);
	virtual ~DataStream(void);
	virtual int Read(void* dest, unsigned int len) = 0;
	int ReadWord(ieWord* dest);
	int ReadWordSigned (ieWordSigned* dest);
	int ReadDword(ieDword* dest);
	int ReadResRef(ieResRef dest);
	virtual int Write(const void* src, unsigned int len) = 0;
	int WriteWord(const ieWord* src);
	int WriteDword(const ieDword* src);
	int WriteResRef(const ieResRef src);
	virtual int Seek(int pos, int startpos) = 0;
	unsigned long Remains() const;
	unsigned long Size() const;
	unsigned long GetPos() const;
	void Rewind();
	/** Returns true if the stream is encrypted */
	bool CheckEncrypted();
	void ReadDecrypted(void* buf, unsigned int size);
	int ReadLine(void* buf, unsigned int maxlen);
	/** Endian Switch setup */
	static void SetBigEndian(bool be);
	static bool BigEndian();
	/** Create a copy of this stream.
	 *
	 *  Returns NULL on failure.
	 **/
	virtual DataStream* Clone();
private:
	DataStream(const DataStream&);
};

}

#endif  // ! DATASTREAM_H
