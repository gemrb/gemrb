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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/SaveGameIterator.h,v 1.17 2005/06/19 22:59:34 avenger_teambg Exp $
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

#define SAVEGAME_DIRECTORY_MATCHER "%d - %[A-Za-z0-9- ]"

class GEM_EXPORT SaveGame {
public:
	SaveGame(char* path, char* name, char* prefix, int pCount)
	{
		strncpy( Prefix, prefix, sizeof( Prefix ) );
		strncpy( Path, path, sizeof( Path ) );
		strncpy( Name, name, sizeof( Name ) );
		PortraitCount = pCount;
		char nPath[_MAX_PATH];
		struct stat my_stat;
		sprintf( nPath, "%s%s%s.bmp", Path, SPathDelimiter, Prefix );
#ifndef WIN32
		ResolveFilePath( nPath );
#endif
		stat( nPath, &my_stat );
		strftime( Date, _MAX_PATH, "%c", localtime( &my_stat.st_mtime ) );
	};
	~SaveGame()
	{
	};
	int GetPortraitCount()
	{
		return PortraitCount;
	};
	const char* GetName()
	{
		return Name;
	};
	const char* GetPrefix()
	{
		return Prefix;
	};
	const char* GetPath()
	{
		return Path;
	};
	const char* GetDate()
	{
		return Date;
	};

	DataStream* GetPortrait(int index)
	{
		if (index > PortraitCount) {
			return NULL;
		}
		char nPath[_MAX_PATH];
		sprintf( nPath, "%s%sPORTRT%d.bmp", Path, SPathDelimiter, index );
#ifndef WIN32
		ResolveFilePath( nPath );
#endif
		FileStream* fs = new FileStream();
		fs->Open( nPath, true );
		return fs;
	};
	DataStream* GetScreen()
	{
		char nPath[_MAX_PATH];
		sprintf( nPath, "%s%s%s.bmp", Path, SPathDelimiter, Prefix );
#ifndef WIN32
		ResolveFilePath( nPath );
#endif
		FileStream* fs = new FileStream();
		fs->Open( nPath, true );
		return fs;
	};
	DataStream* GetGame()
	{
		char nPath[_MAX_PATH];
		sprintf( nPath, "%s%s%s.gam", Path, SPathDelimiter, Prefix );
#ifndef WIN32
		ResolveFilePath( nPath );
#endif
		FileStream* fs = new FileStream();
		fs->Open( nPath, true );
		return fs;
	};
	DataStream* GetWmap()
	{
		char nPath[_MAX_PATH];
		sprintf( nPath, "%s%s%s.wmp", Path, SPathDelimiter, "worldmap" );
#ifndef WIN32
		ResolveFilePath( nPath );
#endif
		FileStream* fs = new FileStream();
		fs->Open( nPath, true );
		return fs;
	};
	DataStream* GetSave()
	{
		char nPath[_MAX_PATH];
		sprintf( nPath, "%s%s%s.sav", Path, SPathDelimiter, Prefix );
#ifndef WIN32
		ResolveFilePath( nPath );
#endif
		FileStream* fs = new FileStream();
		fs->Open( nPath, true );
		return fs;
	};
private:
	char Path[_MAX_PATH];
	char Prefix[10];
	char Name[_MAX_PATH];
	char Date[_MAX_PATH];
	int PortraitCount;
};

class GEM_EXPORT SaveGameIterator {
private:
	bool loaded;
	std::vector<char*> save_slots;

public:
	SaveGameIterator(void);
	~SaveGameIterator(void);
	bool RescanSaveGames();
	int GetSaveGameCount();
	SaveGame* GetSaveGame(int index);
	void DeleteSaveGame(int index);
	bool ExistingSlotName(int index);
	int CreateSaveGame(int index, const char *slotname);
};

#endif
