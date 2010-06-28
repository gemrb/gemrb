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

#ifndef SAVEGAMEITERATOR_H
#define SAVEGAMEITERATOR_H

#include "exports.h"

#include "Holder.h"
#include "ResourceManager.h"
#include "System/FileStream.h"

#include <vector>

class ImageMgr;

#define SAVEGAME_DIRECTORY_MATCHER "%d - %[A-Za-z0-9- _]"

class GEM_EXPORT SaveGame : public Held<SaveGame> {
public:
	static const TypeID ID;
public:
	SaveGame(const char* path, const char* name, const char* prefix, const char* slotname, int pCount, int saveID);
	~SaveGame();
	int GetPortraitCount() const
	{
		return PortraitCount;
	}
	int GetSaveID() const
	{
		return SaveID;
	}
	const char* GetName() const
	{
		return Name;
	}
	const char* GetPrefix() const
	{
		return Prefix;
	}
	const char* GetPath() const
	{
		return Path;
	}
	const char* GetDate() const
	{
		return Date;
	}
	const char* GetGameDate() const;
	const char* GetSlotName() const
	{
		return SlotName;
	}

	Sprite2D* GetPortrait(int index) const;
	Sprite2D* GetPreview() const;
	DataStream* GetGame() const;
	DataStream* GetWmap() const;
	DataStream* GetSave() const;
private:
	char Path[_MAX_PATH];
	char Prefix[10];
	char Name[_MAX_PATH];
	char Date[_MAX_PATH];
	mutable char GameDate[_MAX_PATH];
	char SlotName[_MAX_PATH];
	int PortraitCount;
	int SaveID;
	ResourceManager manager;
};

class GEM_EXPORT SaveGameIterator {
private:
	typedef std::vector<Holder<SaveGame> > charlist;
	charlist save_slots;

public:
	SaveGameIterator(void);
	~SaveGameIterator(void);
	const charlist& GetSaveGames();
	void DeleteSaveGame(Holder<SaveGame>);
	int CreateSaveGame(Holder<SaveGame>, const char *slotname);
	int CreateSaveGame(int index, bool mqs = false);
private:
	bool RescanSaveGames();
	static Holder<SaveGame> GetSaveGame(const char *slotname);
	void PruneQuickSave(const char *folder);
};

#endif
