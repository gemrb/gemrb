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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/IniSpawn.h,v 1.2 2007/02/14 20:27:41 avenger_teambg Exp $
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

#include "../../includes/ie_types.h"
#include "Region.h"
#include "DataFileMgr.h"

class Map;

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

/**
 * @struct CritterEntry
 */

//critter flags
#define CF_IGNORENOSEE 1
#define CF_DEATHVAR    2

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
	int Orientation;          //spawn orientation
	int Flags;                //CF_IGNORENOSEE, CF_DEATHVAR
	int TotalQuantity;        //total number
	int SpawnCount;           //create quantity
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

class GEM_EXPORT IniSpawn {
public:
	IniSpawn(Map *owner);
	~IniSpawn();

private:
	Map *map; //owner
	ieResRef NamelessSpawnArea;
	Point NamelessSpawnPoint;
	int NamelessState;
	SpawnEntry enterspawn;
	int eventcount;
	int last_spawndate;
	SpawnEntry *eventspawns;

	void ReadCreature(DataFileMgr *inifile,
		const char *crittername, CritterEntry &critter);
	void ReadSpawnEntry(DataFileMgr *inifile,
		const char *entryname, SpawnEntry &entry);
	//spawns a single creature
	void SpawnCreature(CritterEntry &critter);
	void SpawnGroup(SpawnEntry &event);
public:
	void InitSpawn(ieResRef DefaultArea);
	void RespawnNameless();
	void InitialSpawn();
	void CheckSpawn();
};

#endif  // ! INISPAWN_H
