// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef SAVEGAME_H
#define SAVEGAME_H

#include "exports.h"

#include "ResourceManager.h"

#include "System/VFS.h"

namespace GemRB {

class ImageMgr;
class Sprite2D;

class GEM_EXPORT SaveGame {
public:
	static const TypeID ID;

public:
	SaveGame(path_t path, const path_t& name, const ResRef& prefix, std::string slotname, int pCount, int saveID);

	int GetPortraitCount() const
	{
		return PortraitCount;
	}
	int GetSaveID() const
	{
		return SaveID;
	}
	const String& GetName() const
	{
		return Name;
	}
	const ResRef& GetPrefix() const
	{
		return Prefix;
	}
	const path_t& GetPath() const
	{
		return Path;
	}
	const path_t& GetDate() const
	{
		return Date;
	}
	const std::string& GetGameDate() const;
	const std::string& GetSlotName() const
	{
		return SlotName;
	}

	Holder<Sprite2D> GetPortrait(int index) const;
	Holder<Sprite2D> GetPreview() const;
	DataStream* GetGame() const;
	DataStream* GetWmap(int idx) const;
	DataStream* GetSave() const;

private:
	path_t Path;
	String Name;
	ResRef Prefix;
	std::string Date;
	mutable std::string GameDate;
	std::string SlotName;
	int PortraitCount;
	int SaveID;
	ResourceManager manager;
};

}

#endif
