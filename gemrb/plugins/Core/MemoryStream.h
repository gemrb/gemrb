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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/MemoryStream.h,v 1.9 2004/04/18 14:26:03 avenger_teambg Exp $
 *
 */

#ifndef MEMORYSTREAM_H
#define MEMORYSTREAM_H

#include "../../includes/globals.h"

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

class GEM_EXPORT MemoryStream : public DataStream {
private:
	void* ptr;
	unsigned long length;
	bool autoFree;
public:
	MemoryStream(void* buffer, int length, bool autoFree = true);
	~MemoryStream(void);
	int Read(void* dest, unsigned int length);
	int Seek(int pos, int startpos);
	unsigned long Size();
	/** No descriptions */
	int ReadLine(void* buf, unsigned int maxlen);
};

#endif
