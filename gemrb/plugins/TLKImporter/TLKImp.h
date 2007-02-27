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
 * $Id$
 *
 */

#ifndef TLKIMP_H
#define TLKIMP_H

#include "../Core/StringMgr.h"

class TLKImp : public StringMgr {
private:
	DataStream* str;
	bool autoFree;

	//Data
	ieDword StrRefCount, Offset;
public:
	TLKImp(void);
	~TLKImp(void);
	bool Open(DataStream* stream, bool autoFree = true);
	char* GetString(ieStrRef strref, ieDword flags = 0);
	StringBlock GetStringBlock(ieStrRef strref, unsigned int flags = 0);
	void FreeString(char *str);
private:
	/** resolves day and monthname tokens */
	void GetMonthName(int dayandmonth);
	/** replaces tags in dest, don't exceed Length */
	bool ResolveTags(char* dest, char* source, int Length);
	/** returns the needed length in Length, 
		if there was no token, returns false */
	bool GetNewStringLength(char* string, int& Length);
	/**returns the decoded length of the built-in token
	   if dest is not NULL it also returns the decoded value
	   */
	int BuiltinToken(char* Token, char* dest);
	int RaceStrRef(int slot);
 	int GenderStrRef(int slot, int malestrref, int femalestrref);
	char *CharName(int slot);
public:
	void release(void)
	{
		delete this;
	}
};

#endif
