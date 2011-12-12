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

#include "exports.h"
#include "ie_types.h"

#include "DataFileMgr.h"
#include "Region.h"

class Map;

/**
 * @struct CritterEntry
 */

//critter flags
#define CF_IGNORECANSEE 1
#define CF_DEATHVAR    2
#define CF_NO_DIFF_1 4
#define CF_NO_DIFF_2 8
#define CF_NO_DIFF_3 16
#define CF_CHECKVIEWPORT 32
#define CF_CHECKCROWD 64
#define CF_SAFESTPOINT 128
#define CF_NO_DIFF_MASK 28
#define CF_CHECK_NAME 256
#define CF_GOOD 512
#define CF_LAW 1024
#define CF_LADY 2048
#define CF_MURDER 4096
#define CF_FACTION 8192
#define CF_TEAM 16384

//spec ids flags
#define AI_EA		0
#define AI_FACTION	1
#define AI_TEAM		2
#define AI_GENERAL	3
#define AI_RACE		4
#define AI_CLASS	5
#define AI_SPECIFICS	6
#define AI_GENDER	7
#define AI_ALIGNMENT	8

//spawn point could be:
// s - single
// r - random
// e - preset
// save_select_point saves the spawnpoint

struct CritterEntry {
	int creaturecount;
	ieResRef *CreFile;        //spawn one of these creatures
	ieByte Spec[9];		  //existance check IDS qualifier
	ieByte SetSpec[9];	  //set IDS qualifier
	ieVariable ScriptName;    //existance check scripting name
	ieVariable SpecVar;       //condition variable
	ieResRef SpecContext;     //condition variable context
	ieResRef OverrideScript;  //override override script
	ieResRef ClassScript;     //overrride class script
	ieResRef RaceScript;      //override race script
	ieResRef GeneralScript;   //override general script
	ieResRef DefaultScript;   //override default script
	ieResRef AreaScript;      //override area script
	ieResRef SpecificScript;  //override specific script
	ieResRef Dialog;          //override dialog
	ieVariable SpawnPointVar; //spawn point saved location
	Point SpawnPoint;         //spawn point
	int SpecVarOperator;      //operation performed on spec var
	int SpecVarValue;         //using this value with the operation
	int SpecVarInc;           //add this to spec var at each spawn
	int Orientation;          //spawn orientation
	int Flags;                //CF_IGNORENOSEE, CF_DEATHVAR, etc
	int TotalQuantity;        //total number
	int SpawnCount;           //create quantity
	ieDword TimeOfDay;        //spawn time of day (defaults to anytime)
	ieByte DeathCounters[4];  //4 bytes
};

/**
 * @class SpawnEntry
 */
class SpawnEntry {
public:
	ieDword interval;
	int crittercount;
	CritterEntry *critters;
	SpawnEntry() {
		interval = 0;
		crittercount = 0;
		critters = NULL;
	}
	~SpawnEntry() {
		if (critters) {
			for (int i=0;i<crittercount;i++) {
				delete[] critters[i].CreFile;
			}
			delete[] critters;
		}
	}
};

/**
 * @class Spawn
 * Class for the special creature re/spawn features that are unique to PST.
 */

struct VariableSpec {
	ieVariable Name;
	ieDword Value;
};

class GEM_EXPORT IniSpawn {
public:
	IniSpawn(Map *owner);
	~IniSpawn();

private:
	Map *map; //owner
	ieResRef NamelessSpawnArea;
	int namelessvarcount;
	VariableSpec *NamelessVar;
	int localscount;
	VariableSpec *Locals;
	Point NamelessSpawnPoint;
	int NamelessState;
	SpawnEntry enterspawn;
	int last_spawndate;
	int eventcount;
	SpawnEntry *eventspawns;

	void ReadCreature(DataFileMgr *inifile,
		const char *crittername, CritterEntry &critter) const;
	void ReadSpawnEntry(DataFileMgr *inifile,
		const char *entryname, SpawnEntry &entry) const;
	//spawns a single creature
	void SpawnCreature(CritterEntry &critter) const;
	void SpawnGroup(SpawnEntry &event);
	//gets the spec var operation code from a keyword
	int GetDiffMode(const char *keyword) const;
	bool Schedule(ieDword appearance, ieDword gametime) const;
public:
	/* called by action of the same name */
	void SetNamelessDeath(const ieResRef area, Point &pos, ieDword state);
	void InitSpawn(const ieResRef DefaultArea);
	void RespawnNameless();
	void InitialSpawn();
	void CheckSpawn();
};

#endif  // ! INISPAWN_H
