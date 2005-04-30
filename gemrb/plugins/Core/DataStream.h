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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/DataStream.h,v 1.14 2005/04/30 22:48:19 avenger_teambg Exp $
 *
 */

#ifndef DATASTREAM_H
#define DATASTREAM_H

#include "../../includes/globals.h"

#define GEM_CURRENT_POS 0
#define GEM_STREAM_START 1

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

class GEM_EXPORT DataStream {
public:
	unsigned long Pos;
	unsigned long size;
	bool Encrypted;
	DataStream(void);
	virtual ~DataStream(void);
	virtual int Read(void* dest, unsigned int len) = 0;
	int ReadWord(ieWord* dest);
	int ReadDword(ieDword* dest);
	int ReadResRef(ieResRef dest);
	virtual int Write(void* src, unsigned int len) = 0;
	int WriteWord(ieWord* src);
	int WriteDword(ieDword* src);
	int WriteResRef(ieResRef src);
	virtual int Seek(int pos, int startpos) = 0;
	unsigned long Remains() const;
  unsigned long Size() const;
	/** Returns true if the stream is encrypted */
	bool CheckEncrypted();
	/** No descriptions */
	void ReadDecrypted(void* buf, unsigned int size);
	/** No descriptions */
	virtual int ReadLine(void* buf, unsigned int maxlen) = 0;
	/** Endian Switch setup*/
	static void SetEndianSwitch(int par);
	static bool IsEndianSwitch();
	char filename[_MAX_PATH];
};

#endif
