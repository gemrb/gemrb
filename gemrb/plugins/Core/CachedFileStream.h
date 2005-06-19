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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/CachedFileStream.h,v 1.17 2005/06/19 22:59:33 avenger_teambg Exp $
 *
 */

#ifndef CACHEDFILESTREAM_H
#define CACHEDFILESTREAM_H

#include "FileStream.h"

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

class GEM_EXPORT CachedFileStream : public DataStream// : public FileStream
{
private:
	bool autoFree;
	unsigned long startpos;
	_FILE* str;
	char originalfile[_MAX_PATH];
public:
	CachedFileStream(const char* stream, bool autoFree = true);
	CachedFileStream(CachedFileStream* cfs, int startpos, int size,
		bool autoFree = true);
	~CachedFileStream(void);
	int Read(void* dest, unsigned int length);
	int Write(const void* src, unsigned int length);
	int Seek(int pos, int startpos);
	/** No descriptions */
	int ReadLine(void* buf, unsigned int maxlen);
};
#endif
