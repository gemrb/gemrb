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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/IDSImporter/IDSImp.h,v 1.11 2004/08/07 21:00:35 guidoj Exp $
 *
 */

#ifndef IDSIMP_H
#define IDSIMP_H

#include "../Core/SymbolMgr.h"

typedef struct Pair {
	int val;
	char* str;
} Pair;

class IDSImp : public SymbolMgr {
private:
	DataStream* str;
	bool autoFree;

	std::vector< Pair> pairs;
	std::vector< char*> ptrs;

public:
	IDSImp(void);
	~IDSImp(void);
	bool Open(DataStream* stream, bool autoFree = true);
	int GetValue(const char* txt);
	char* GetValue(int val);
	char* GetStringIndex(unsigned int Index);
	int GetValueIndex(unsigned int Index);
	int FindString(char *str, int len);
	int FindValue(int val);

public:
	void release(void)
	{
		delete this;
	}
};

#endif

