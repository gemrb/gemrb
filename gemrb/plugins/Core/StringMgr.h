/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/StringMgr.h,v 1.5 2004/02/24 22:20:36 balrog994 Exp $
 *
 */

#ifndef STRINGMGR_H
#define STRINGMGR_H

#include "Plugin.h"
#include "DataStream.h"

typedef struct StringBlock {
	char* text;
	char Sound[9];
} StringBlock;

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

class GEM_EXPORT StringMgr : public Plugin {
public:
	StringMgr(void);
	virtual ~StringMgr(void);
	virtual bool Open(DataStream* stream, bool autoFree = true) = 0;
	virtual char* GetString(unsigned long strref, int flag = 0) = 0;
	virtual StringBlock GetStringBlock(unsigned long strref, int flag = 0) = 0;
};

#endif
