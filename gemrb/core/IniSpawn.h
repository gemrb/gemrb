/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2007 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

/**
 * @file IniSpawn.h
 * Declares IniSpawn a class for the special creature re/spawn features
 * PST has. The information is originally stored in <arearesref>.ini files
 * @author The GemRB Project
 */

#ifndef INISPAWN_H
#define INISPAWN_H

#include "exports-core.h"
#include "ie_types.h"

#include "Region.h"

namespace GemRB {

class DataFileMgr;
class Map;

/**
 * @struct CritterEntry
 */

//critter flags
#define CF_IGNORECANSEE 1
#define CF_DEATHVAR     2
#define CF_NO_DIFF_1    4
#define CF_NO_DIFF_2    8
#define CF_NO_DIFF_3    16
// 32
#define CF_INC_INDEX        64
#define CF_HOLD_POINT       128
#define CF_NO_DIFF_MASK     28
#define CF_CHECK_NAME       256
#define CF_GOOD             512
#define CF_LAW              1024
#define CF_LADY             2048
#define CF_MURDER           4096
#define CF_FACTION          8192
#define CF_TEAM             16384
#define CF_BUDDY            0x8000
#define CF_DISABLE_RENDERER 0x10000
#define CF_SAFEST_POINT     0x20000

struct CritterEntry {
	std::vector<ResRef> CreFile; //spawn one of these creatures
	ieByte Spec[9] {}; // existence check IDS qualifier
	ieByte SetSpec[9] {}; // set IDS qualifier
	ieVariable ScriptName; // existence check scripting name
	ieVariable SpecVar; //condition variable
	ResRef SpecContext; //condition variable context
	ResRef OverrideScript; //override override script
	ResRef ClassScript; // override class script
	ResRef RaceScript; //override race script
	ResRef GeneralScript; //override general script
	ResRef DefaultScript; //override default script
	ResRef AreaScript; //override area script
	ResRef SpecificScript; //override specific script
	ResRef Dialog; //override dialog
	ResRef PointSelectContext;
	ieVariable PointSelectVar; // holds spawn point index to use
	ResRef SaveSelectedPointContext = "GLOBAL";
	ieVariable SaveSelectedPoint; // a var to save the selected spawn point location to
	ResRef SaveSelectedFacingContext = "GLOBAL";
	ieVariable SaveSelectedFacing; // a var to save the selected spawn point orientation to
	Point SpawnPoint; //spawn point
	std::string SpawnPointsDef; // the unparsed available spawn points
	char SpawnMode = '\0'; // the spawn point selection mode
	int SpecVarOperator = 0; // operation performed on spec var
	int SpecVarValue = 0; // using this value with the operation
	int SpecVarInc = 0; // add this to spec var at each spawn
	int Orientation = 0; // spawn orientation
	int Orientation2 = 0; // spawn orientation if the spawn point doesn't specify it
	int Flags = 0; // CF_IGNORENOSEE, CF_DEATHVAR, etc
	int TotalQuantity = 0; // total number
	int SpawnCount = 0; // create quantity
	ieDword TimeOfDay = 0; // spawn time of day (defaults to anytime)
	ieByte DeathCounters[4] {}; // 4 bytes
};

/**
 * @class SpawnEntry
 */
class SpawnEntry {
public:
	ieDword interval = 0;
	ieDword lastSpawndate = 0;
	std::vector<CritterEntry> critters;
	std::string name;
};

/**
 * @class Spawn
 * Class for the special creature re/spawn features that are unique to PST.
 */

struct VariableSpec {
	ieVariable Name {};
	ieDword Value = 0;

	VariableSpec(const ieVariable& name, ieDword val)
		: Name(name), Value(val)
	{}
};

class GEM_EXPORT IniSpawn {
public:
	IniSpawn(Map* owner, const ResRef& defaultArea);

private:
	Map* map; //owner
	ResRef NamelessSpawnArea;
	std::vector<VariableSpec> NamelessVar;
	std::vector<VariableSpec> Locals;
	Point NamelessSpawnPoint;
	Point PartySpawnPoint;
	ResRef PartySpawnArea;
	int NamelessState = 35;
	SpawnEntry enterspawn;
	SpawnEntry exitspawn;
	std::vector<SpawnEntry> eventspawns;
	ieDword detail_level = 2; // high detail level by default

	CritterEntry ReadCreature(const DataFileMgr* inifile, StringView crittername) const;
	void ReadSpawnEntry(const DataFileMgr* inifile, StringView entryname, SpawnEntry& entry) const;
	//spawns a single creature
	void SpawnCreature(const CritterEntry& critter) const;
	void SpawnGroup(SpawnEntry& event) const;
	//gets the spec var operation code from a keyword
	int GetDiffMode(const ieVariable& keyword) const;
	void PrepareSpawnPoints(const DataFileMgr* iniFile, StringView critterName, CritterEntry& critter) const;
	void SelectSpawnPoint(CritterEntry& critter) const;

public:
	/* called by action of the same name */
	void SetNamelessDeath(const ResRef& area, const Point& pos, ieDword state);
	void SetNamelessDeathParty(const Point& pos, int reserved);
	void RespawnNameless();
	void InitialSpawn();
	void ExitSpawn();
	void CheckSpawn();
};

}

#endif // ! INISPAWN_H
