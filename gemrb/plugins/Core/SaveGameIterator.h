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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/SaveGameIterator.h,v 1.7 2003/11/25 13:48:03 balrog994 Exp $
 *
 */

#ifndef SAVEGAMEITERATOR_H
#define SAVEGAMEITERATOR_H

#include <time.h>
#include <sys/stat.h>
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

class GEM_EXPORT SaveGame {
public:
	SaveGame(char * path, char * name, char * prefix, int pCount)
	{
		strncpy(Prefix, prefix, sizeof(Prefix) );
		strncpy(Path, path, sizeof(Path) );
		strncpy(Name, name, sizeof(Name) );
		PortraitCount = pCount;
		char nPath[_MAX_PATH];
		struct stat my_stat;
		sprintf(nPath, "%s%s%s.bmp", Path, SPathDelimiter, Prefix);
		stat(nPath, &my_stat);
		strftime(Date, _MAX_PATH, "%c",localtime(&my_stat.st_mtime));
	};
	~SaveGame() {};
	int GetPortraitCount() { return PortraitCount; };
	const char *GetName() { return Name; };
	const char *GetPrefix() { return Prefix; };
	const char *GetPath() { return Path; };
	const char *GetDate() { return Date; };

	DataStream * GetPortrait(int index)
	{
		if(index > PortraitCount)
			return NULL;
		char nPath[_MAX_PATH];
		sprintf(nPath, "%s%sPORTRT%d.bmp", Path, SPathDelimiter, index);
		FileStream * fs = new FileStream();
		fs->Open(nPath, true);
		return fs;
	};
	DataStream * GetScreen()
	{
		char nPath[_MAX_PATH];
		sprintf(nPath, "%s%s%s.bmp", Path, SPathDelimiter, Prefix);
		FileStream * fs = new FileStream();
		fs->Open(nPath, true);
		return fs;
	};
private:
	char Path[_MAX_PATH];
	char Prefix[10];
	char Name[_MAX_PATH];
	char Date[_MAX_PATH];
	int  PortraitCount;
};

class GEM_EXPORT SaveGameIterator
{
public:
	SaveGameIterator(void);
	~SaveGameIterator(void);
	int GetSaveGameCount();
	SaveGame * GetSaveGame(int index, bool Remove=false);
};

#endif
