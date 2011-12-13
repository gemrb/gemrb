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

// This class handles the special spawn structures of planescape torment
// (stored in .ini format)

#include "IniSpawn.h"

#include "win32def.h"

#include "CharAnimations.h"
#include "Game.h"
#include "GameData.h"
#include "Interface.h"
#include "Map.h"
#include "PluginMgr.h"
#include "GameScript/GSUtils.h"
#include "GameScript/Matching.h"
#include "Scriptable/Actor.h"

static const int StatValues[9]={
IE_EA, IE_FACTION, IE_TEAM, IE_GENERAL, IE_RACE, IE_CLASS, IE_SPECIFIC, 
IE_SEX, IE_ALIGNMENT };

IniSpawn::IniSpawn(Map *owner)
{
	map = owner;
	NamelessSpawnArea[0] = 0;
	NamelessState = 35;
	NamelessVar = NULL;
	namelessvarcount = 0;
	Locals = NULL;
	localscount = 0;
	eventspawns = NULL;
	eventcount = 0;
	last_spawndate = 0;
	//high detail level by default
	detail_level = 2;
	core->GetDictionary()->Lookup("Detail Level", detail_level);
}

IniSpawn::~IniSpawn()
{
	if (eventspawns) {
		delete[] eventspawns;
		eventspawns = NULL;
	}

	if (Locals) {
		delete[] Locals;
		Locals = NULL;
	}

	if (NamelessVar) {
		delete[] NamelessVar;
		NamelessVar = NULL;
	}
}

static Holder<DataFileMgr> GetIniFile(const ieResRef DefaultArea)
{
	//the lack of spawn ini files is not a serious problem, happens all the time
	if (!gamedata->Exists( DefaultArea, IE_INI_CLASS_ID)) {
		return NULL;
	}

	DataStream* inifile = gamedata->GetResource( DefaultArea, IE_INI_CLASS_ID );
	if (!inifile) {
		return NULL;
	}
	if (!core->IsAvailable( IE_INI_CLASS_ID )) {
		printStatus( "ERROR", LIGHT_RED );
		printMessage( "IniSpawn","No INI Importer Available.\n",LIGHT_RED );
		return NULL;
	}

	PluginHolder<DataFileMgr> ini(IE_INI_CLASS_ID);
	ini->Open(inifile);
	return ini;
}

/*** initializations ***/

static inline int CountElements(const char *s, char separator)
{
	int ret = 1;
	while(*s) {
		if (*s==separator) ret++;
		s++;
	}
	return ret;
}

static inline void GetElements(const char *s, ieResRef *storage, int count)
{
	while(count--) {
		ieResRef *field = storage+count;
		strnuprcpy(*field, s, sizeof(ieResRef)-1);
		for(size_t i=0;i<sizeof(ieResRef) && (*field)[i];i++) {
			if ((*field)[i]==',') {
				(*field)[i]='\0';
				break;
			}
		}
		if (!count) break;
		while(*s && *s!=',') s++;
		s++;
		if (*s==' ') s++; //this is because there is one single screwed up entry in ar1100.ini
	}
}

static inline void GetElements(const char *s, ieVariable *storage, int count)
{
	while(count--) {
		ieVariable *field = storage+count;
		strnuprcpy(*field, s, sizeof(ieVariable)-1);
		for(size_t i=0;i<sizeof(ieVariable) && (*field)[i];i++) {
			if ((*field)[i]==',') {
				(*field)[i]='\0';
				break;
			}
		}
		while(*s && *s!=',') s++;
		s++;
	}
}

// possible values implemented in DiffMode, but not needed here
// BINARY_LESS_OR_EQUALS 6 //(left has only bits in right)
// BINARY_MORE_OR_EQUALS 7 //(left has equal or more bits than right)
// BINARY_INTERSECT 8      //(left and right has at least one common bit)
// BINARY_NOT_INTERSECT 9  //(no common bits)
// BINARY_MORE 10          //left has more bits than right
// BINARY_LESS 11          //left has less bits than right

int IniSpawn::GetDiffMode(const char *keyword) const
{
	if (!keyword) return NO_OPERATION; //-1
	if (keyword[0]==0) return NO_OPERATION; //-1
	if (!stricmp(keyword,"less_or_equal_to") ) return LESS_OR_EQUALS; //0 (gemrb ext)
	if (!stricmp(keyword,"equal_to") ) return EQUALS; // 1
	if (!stricmp(keyword,"less_than") ) return LESS_THAN; // 2
	if (!stricmp(keyword,"greater_than") ) return GREATER_THAN; //3
	if (!stricmp(keyword,"greater_or_equal_to") ) return GREATER_THAN; //4 (gemrb ext)
	if (!stricmp(keyword,"not_equal_to") ) return NOT_EQUALS; //5
	return NO_OPERATION;
}

//unimplemented tags (* marks partially implemented):
//*check_crowd
//*good_mod, law_mod, lady_mod, murder_mod
// control_var
// spec_area
//*death_faction
//*death_team
// check_by_view_port
//*do_not_spawn
//*time_of_day
// hold_selected_point_key
// inc_spawn_point_index
//*find_safest_point
// spawn_time_of_day
// exit - similar to enter[spawn], this is a spawn branch type (on exiting an area?)
// PST only
//*auto_buddy
//*detail_level
void IniSpawn::ReadCreature(DataFileMgr *inifile, const char *crittername, CritterEntry &critter) const
{
	const char *s;
	int ps;
	
	memset(&critter,0,sizeof(critter));

	critter.TimeOfDay = (ieDword) inifile->GetKeyAsInt(crittername,"time_of_day", 0xffffffff);

	if (inifile->GetKeyAsBool(crittername,"do_not_spawn",false)) {
		//if the do not spawn flag is true, ignore this entry
		return;
	}

	s = inifile->GetKeyAsString(crittername,"detail_level",NULL);
	if (s) {
		ieDword level;

		switch(s[0]) {
			case 'h': case 'H': level = 2; break;
			case 'm': case 'M': level = 1; break;
			default: level = 0; break;
		}
		//If the detail level is lower than this creature's detail level,
		//skip this entry, creature_count is 0, so it will be ignored at evaluation of the spawn
		if (level>detail_level) {
			return;
		}
	}

	//all specvars are using global, but sometimes it is explicitly given
	s = inifile->GetKeyAsString(crittername,"spec_var",NULL);
	if (s) {
		if ((strlen(s)>9) && s[6]==':' && s[7]==':') {
			strnuprcpy(critter.SpecContext, s, 6);
			strnlwrcpy(critter.SpecVar, s+8, 32);
		} else {
			strnuprcpy(critter.SpecContext, "GLOBAL", 6);
			strnlwrcpy(critter.SpecVar, s, 32);
		}
	}

	//add this to specvar at each spawn
	ps = inifile->GetKeyAsInt(crittername,"spec_var_inc", 0);
	critter.SpecVarInc=ps;

	//use this value with spec_var_operation to determine spawn
	ps = inifile->GetKeyAsInt(crittername,"spec_var_value",0);
	critter.SpecVarValue=ps;
	//this operation uses DiffCore
	s = inifile->GetKeyAsString(crittername,"spec_var_operation","");
	critter.SpecVarOperator=GetDiffMode(s);
	//the amount of critters to spawn
	critter.TotalQuantity = inifile->GetKeyAsInt(crittername,"spec_qty",1);
	critter.SpawnCount = inifile->GetKeyAsInt(crittername,"create_qty",critter.TotalQuantity);

	//the creature resource(s)
	s = inifile->GetKeyAsString(crittername,"cre_file",NULL);
	if (s) {
		critter.creaturecount = CountElements(s,',');
		critter.CreFile=new ieResRef[critter.creaturecount];
		GetElements(s, critter.CreFile, critter.creaturecount);
	} else {
		printMessage("IniSpawn", "Invalid spawn entry: %s\n", LIGHT_RED, crittername);
	}

	s = inifile->GetKeyAsString(crittername,"point_select",NULL);
	
	if (s) {
		ps=s[0];
	} else {
		ps=0;
	}

	s = inifile->GetKeyAsString(crittername,"spawn_point",NULL);
	if (s) {
		//expect more than one spawnpoint
		if (ps=='r') {
			//select one of the spawnpoints randomly
			int count = core->Roll(1,CountElements(s,']'),-1);
			//go to the selected spawnpoint
			while(count--) {
				while(*s++!=']') ;
			}
		}
		//parse the selected spawnpoint
		int x,y,o;
		if (sscanf(s,"[%d.%d:%d]", &x, &y, &o)==3) {
			critter.SpawnPoint.x=(short) x;
			critter.SpawnPoint.y=(short) y;
			critter.Orientation=o;
		} else {
			if (sscanf(s,"[%d.%d]", &x, &y)==2) {
				critter.SpawnPoint.x=(short) x;
				critter.SpawnPoint.y=(short) y;
				critter.Orientation=core->Roll(1,16,-1);
			}
		}
	}
	
	//store or retrieve spawn point
	s = inifile->GetKeyAsString(crittername,"spawn_point_global", NULL);
	if (s) {
		switch (ps) {
		case 'e':
			critter.SpawnPoint.fromDword(CheckVariable(map, s+8,s));
			break;
		default:
			//see save_selected_point
			//SetVariable(map, s+8, s, critter.SpawnPoint.asDword());
			break;
		}
	}

	//take facing from variable
	s = inifile->GetKeyAsString(crittername,"spawn_facing_global", NULL);
	if (s) {
		switch (ps) {
		case 'e':
			critter.Orientation=(int) CheckVariable(map, s+8,s);
			break;
		default:
			//see save_selected_point
			//SetVariable(map, s+8, s, (ieDword) critter.Orientation);
			break;
		}
	}

	s = inifile->GetKeyAsString(crittername,"save_selected_point",NULL);
	if (s) {
		if ((strlen(s)>9) && s[6]==':' && s[7]==':') {
			SetVariable(map, s+8, s, critter.SpawnPoint.asDword());
		} else {
			SetVariable(map, s, "GLOBAL", critter.SpawnPoint.asDword());
		}
	}
	s = inifile->GetKeyAsString(crittername,"save_selected_facing",NULL);
	if (s) {
		if ((strlen(s)>9) && s[6]==':' && s[7]==':') {
			SetVariable(map, s+8, s, (ieDword) critter.Orientation);
		} else {
			SetVariable(map, s, "GLOBAL", (ieDword) critter.Orientation);
		}
	}

	//sometimes only the orientation is given, the point is stored in a variable
	ps = inifile->GetKeyAsInt(crittername,"facing",-1);
	if (ps!=-1) critter.Orientation = ps;
	ps = inifile->GetKeyAsInt(crittername, "ai_ea",-1);
	if (ps!=-1) critter.SetSpec[AI_EA] = (ieByte) ps;
	ps = inifile->GetKeyAsInt(crittername, "ai_team",-1);
	if (ps!=-1) critter.SetSpec[AI_TEAM] = (ieByte) ps;
	ps = inifile->GetKeyAsInt(crittername, "ai_general",-1);
	if (ps!=-1) critter.SetSpec[AI_GENERAL] = (ieByte) ps;
	ps = inifile->GetKeyAsInt(crittername, "ai_race",-1);
	if (ps!=-1) critter.SetSpec[AI_RACE] = (ieByte) ps;
	ps = inifile->GetKeyAsInt(crittername, "ai_class",-1);
	if (ps!=-1) critter.SetSpec[AI_CLASS] = (ieByte) ps;
	ps = inifile->GetKeyAsInt(crittername, "ai_specifics",-1);
	if (ps!=-1) critter.SetSpec[AI_SPECIFICS] = (ieByte) ps;
	ps = inifile->GetKeyAsInt(crittername, "ai_gender",-1);
	if (ps!=-1) critter.SetSpec[AI_GENDER] = (ieByte) ps;
	ps = inifile->GetKeyAsInt(crittername, "ai_alignment",-1);
	if (ps!=-1) critter.SetSpec[AI_ALIGNMENT] = (ieByte) ps;

	s = inifile->GetKeyAsString(crittername,"spec",NULL);
	if (s) {
		int x[9];
		
		ps = sscanf(s,"[%d.%d.%d.%d.%d.%d.%d.%d.%d]", x, x+1, x+2, x+3, x+4, x+5,
			x+6, x+7, x+8);
		if (ps == 0) {
			strnuprcpy(critter.ScriptName, s, 32);
			critter.Flags|=CF_CHECK_NAME;
			memset(critter.Spec,-1,sizeof(critter.Spec));
		} else {
			while(ps--) {
				critter.Spec[ps]=(ieByte) x[ps];
			}
		}
	}

	s = inifile->GetKeyAsString(crittername,"script_name",NULL);
	if (s) {
		strnuprcpy(critter.ScriptName, s, 32);
	}

	//iwd2 script names (override remains the same)
	//special 1 == area
	s = inifile->GetKeyAsString(crittername,"script_special_1",NULL);
	if (s) {
		strnuprcpy(critter.AreaScript,s, 8);
	}
	//special 2 == class
	s = inifile->GetKeyAsString(crittername,"script_special_2",NULL);
	if (s) {
		strnuprcpy(critter.ClassScript,s, 8);
	}
	//special 3 == general
	s = inifile->GetKeyAsString(crittername,"script_special_3",NULL);
	if (s) {
		strnuprcpy(critter.GeneralScript,s, 8);
	}
	//team == specific
	s = inifile->GetKeyAsString(crittername,"script_team",NULL);
	if (s) {
		strnuprcpy(critter.SpecificScript,s, 8);
	}

	//combat == race
	s = inifile->GetKeyAsString(crittername,"script_combat",NULL);
	if (s) {
		strnuprcpy(critter.RaceScript,s, 8);
	}
	//movement == default
	s = inifile->GetKeyAsString(crittername,"script_movement",NULL);
	if (s) {
		strnuprcpy(critter.DefaultScript,s, 8);
	}

	//pst script names
	s = inifile->GetKeyAsString(crittername,"script_override",NULL);
	if (s) {
		strnuprcpy(critter.OverrideScript,s, 8);
	}
	s = inifile->GetKeyAsString(crittername,"script_class",NULL);
	if (s) {
		strnuprcpy(critter.ClassScript,s, 8);
	}
	s = inifile->GetKeyAsString(crittername,"script_race",NULL);
	if (s) {
		strnuprcpy(critter.RaceScript,s, 8);
	}
	s = inifile->GetKeyAsString(crittername,"script_general",NULL);
	if (s) {
		strnuprcpy(critter.GeneralScript,s, 8);
	}
	s = inifile->GetKeyAsString(crittername,"script_default",NULL);
	if (s) {
		strnuprcpy(critter.DefaultScript,s, 8);
	}
	s = inifile->GetKeyAsString(crittername,"script_area",NULL);
	if (s) {
		strnuprcpy(critter.AreaScript,s, 8);
	}
	s = inifile->GetKeyAsString(crittername,"script_specifics",NULL);
	if (s) {
		strnuprcpy(critter.SpecificScript,s, 8);
	}
	s = inifile->GetKeyAsString(crittername,"dialog",NULL);
	if (s) {
		strnuprcpy(critter.Dialog,s, 8);
	}

	//flags
	if (inifile->GetKeyAsBool(crittername,"death_scriptname",false)) {
		critter.Flags|=CF_DEATHVAR;
	}
	if (inifile->GetKeyAsBool(crittername,"death_faction",false)) {
		critter.Flags|=CF_FACTION;
	}
	if (inifile->GetKeyAsBool(crittername,"death_team",false)) {
		critter.Flags|=CF_TEAM;
	}
	ps = inifile->GetKeyAsInt(crittername,"good_mod",0);
	if (ps) {
		critter.Flags|=CF_GOOD;
		critter.DeathCounters[DC_GOOD] = ps;
	}
	ps = inifile->GetKeyAsInt(crittername,"law_mod",0);
	if (ps) {
		critter.Flags|=CF_LAW;
		critter.DeathCounters[DC_LAW] = ps;
	}
	ps = inifile->GetKeyAsInt(crittername,"lady_mod",0);
	if (ps) {
		critter.Flags|=CF_LADY;
		critter.DeathCounters[DC_LADY] = ps;
	}
	ps = inifile->GetKeyAsInt(crittername,"murder_mod",0);
	if (ps) {
		critter.Flags|=CF_MURDER;
		critter.DeathCounters[DC_MURDER] = ps;
	}
	if(inifile->GetKeyAsBool(crittername,"auto_buddy", false)) {
		critter.Flags|=CF_BUDDY;
	}

	//don't spawn when spawnpoint is visible
	if (inifile->GetKeyAsBool(crittername,"ignore_can_see",false)) {
		critter.Flags|=CF_IGNORECANSEE;
	}
	//unsure, but could be similar to previous
	if (inifile->GetKeyAsBool(crittername,"check_view_port", false)) {
		critter.Flags|=CF_CHECKVIEWPORT;
	}
	//unknown, this is used only in pst
	if (inifile->GetKeyAsBool(crittername,"check_crowd", false)) {
		critter.Flags|=CF_CHECKCROWD;
	}
	//unknown, this is used only in pst
	if (inifile->GetKeyAsBool(crittername,"find_safest_point", false)) {
		critter.Flags|=CF_SAFESTPOINT;
	}
	//disable spawn based on game difficulty
	if (inifile->GetKeyAsBool(crittername,"area_diff_1", false)) {
		critter.Flags|=CF_NO_DIFF_1;
	}
	if (inifile->GetKeyAsBool(crittername,"area_diff_2", false)) {
		critter.Flags|=CF_NO_DIFF_2;
	}
	if (inifile->GetKeyAsBool(crittername,"area_diff_3", false)) {
		critter.Flags|=CF_NO_DIFF_3;
	}
}

void IniSpawn::ReadSpawnEntry(DataFileMgr *inifile, const char *entryname, SpawnEntry &entry) const
{
	const char *s;
	
	entry.interval = (unsigned int) inifile->GetKeyAsInt(entryname,"interval",0);
	//don't default to NULL here, some entries may be missing in original game
	//an empty default string here will create an empty but consistent entry
	s = inifile->GetKeyAsString(entryname,"critters","");
	int crittercount = CountElements(s,',');
	entry.crittercount=crittercount;
	entry.critters=new CritterEntry[crittercount];
	ieVariable *critters = new ieVariable[crittercount];
	GetElements(s, critters, crittercount);
	while(crittercount--) {
		ReadCreature(inifile, critters[crittercount], entry.critters[crittercount]);
	}
	delete[] critters;
}

/* set by action */
void IniSpawn::SetNamelessDeath(const ieResRef area, Point &pos, ieDword state) 
{
	strnuprcpy(NamelessSpawnArea, area, 8);
	NamelessSpawnPoint = pos;
	NamelessState = state;
}

void IniSpawn::InitSpawn(const ieResRef DefaultArea)
{
	const char *s;

	Holder<DataFileMgr> inifile = GetIniFile(DefaultArea);
	if (!inifile) {
		strnuprcpy(NamelessSpawnArea, DefaultArea, 8);
		return;
	}

	s = inifile->GetKeyAsString("nameless","destare",DefaultArea);
	strnuprcpy(NamelessSpawnArea, s, 8);
	s = inifile->GetKeyAsString("nameless","point","[0.0]");
	int x,y;
	if (sscanf(s,"[%d.%d]", &x, &y)!=2) {
		x=0;
		y=0;
	}
	NamelessSpawnPoint.x=x;
	NamelessSpawnPoint.y=y;
	//35 - already standing
	//36 - getting up
	NamelessState = inifile->GetKeyAsInt("nameless","state",36);

	namelessvarcount = inifile->GetKeysCount("namelessvar");
	if (namelessvarcount) {
		NamelessVar = new VariableSpec[namelessvarcount];
		for (y=0;y<namelessvarcount;y++) {
			const char* Key = inifile->GetKeyNameByIndex("namelessvar",y);
			strnlwrcpy(NamelessVar[y].Name, Key, 32);
			NamelessVar[y].Value = inifile->GetKeyAsInt("namelessvar",Key,0);
		}
	}

	localscount = inifile->GetKeysCount("locals");
	if (localscount) {
		Locals = new VariableSpec[localscount];
		for (y=0;y<localscount;y++) {
			const char* Key = inifile->GetKeyNameByIndex("locals",y);
			strnlwrcpy(Locals[y].Name, Key, 32);
			Locals[y].Value = inifile->GetKeyAsInt("locals",Key,0);
		}
	}

	s = inifile->GetKeyAsString("spawn_main","enter",NULL);
	if (s) {
		ReadSpawnEntry(inifile.get(), s, enterspawn);
	}

	s = inifile->GetKeyAsString("spawn_main","exit",NULL);
	if (s) {
		ReadSpawnEntry(inifile.get(), s, exitspawn);
	}

	s = inifile->GetKeyAsString("spawn_main","events",NULL);
	if (s) {
		eventcount = CountElements(s,',');
		eventspawns = new SpawnEntry[eventcount];
		ieVariable *events = new ieVariable[eventcount];
		GetElements(s, events, eventcount);
		int ec = eventcount;
		while(ec--) {
			ReadSpawnEntry(inifile.get(), events[ec], eventspawns[ec]);
		}
		delete[] events;
	}
	//maybe not correct
	InitialSpawn();
}


/*** events ***/

//respawn nameless after he bit the dust
void IniSpawn::RespawnNameless()
{
	Game *game = core->GetGame();
	Actor *nameless = game->GetPC(0, false);

	if (NamelessSpawnPoint.isnull()) {
		core->GetGame()->JoinParty(nameless,JP_INITPOS);
		NamelessSpawnPoint=nameless->Pos;
		strnuprcpy(NamelessSpawnArea, nameless->Area, 8);
	}

	nameless->Resurrect();
	//hardcoded!!!
	if (NamelessState==36) {
		nameless->SetStance(IE_ANI_PST_START);
	}
	int i;

	for (i=0;i<game->GetPartySize(false);i++) {
		MoveBetweenAreasCore(game->GetPC(i, false),NamelessSpawnArea,NamelessSpawnPoint,-1, true);
	}

	//certain variables are set when nameless dies
	for (i=0;i<namelessvarcount;i++) {
		SetVariable(game, NamelessVar[i].Name,"GLOBAL", NamelessVar[i].Value);
	}
}

void IniSpawn::SpawnCreature(CritterEntry &critter) const
{
	if (!critter.creaturecount) {
		return;
	}

	ieDword specvar = CheckVariable(map, critter.SpecVar, critter.SpecContext);

	if (critter.SpecVar[0]) {
		if (critter.SpecVarOperator>=0) {
			// dunno if this should be negated
			if (!DiffCore(specvar, critter.SpecVarValue, critter.SpecVarOperator) ) {
				return;
			}
		} else {
			//ar0203 in PST seems to want the check this way.
			//if other areas conflict and you want to use (!specvar),
			//please research further
			//researched further - ar0203 respawns only if specvar is 1
			if (!specvar) {
				return;
			}
		}
	}

	if (!(critter.Flags&CF_IGNORECANSEE)) {
		if (map->IsVisible(critter.SpawnPoint, false) ) {
			return;
		}
	}

	if (critter.Flags&CF_NO_DIFF_MASK) {
		ieDword difficulty;
		ieDword diff_bit;

		core->GetDictionary()->Lookup("Difficulty Level", difficulty);
		switch (difficulty)
		{
		case 0:
			diff_bit = CF_NO_DIFF_1;
			break;
		case 1:
			diff_bit = CF_NO_DIFF_2;
			break;
		case 2:
			diff_bit = CF_NO_DIFF_3;
			break;
		default:
			diff_bit = 0;
		}
		if (critter.Flags&diff_bit) {
			return;
		}
	}

	if (critter.ScriptName[0] && (critter.Flags&CF_CHECK_NAME) ) {
		//maybe this one needs to be using getobjectcount as well
		//currently we cannot count objects with scriptname???
		if (map->GetActor( critter.ScriptName, 0 )) {
			return;
		}
	} else {
		//Object *object = new Object();
		Object object;
		//objectfields based on spec
		object.objectFields[0]=critter.Spec[0];
		object.objectFields[1]=critter.Spec[1];
		object.objectFields[2]=critter.Spec[2];
		object.objectFields[3]=critter.Spec[3];
		object.objectFields[4]=critter.Spec[4];
		object.objectFields[5]=critter.Spec[5];
		object.objectFields[6]=critter.Spec[6];
		object.objectFields[7]=critter.Spec[7];
		object.objectFields[8]=critter.Spec[8];
		int cnt = GetObjectCount(map, &object);
		if (cnt>=critter.TotalQuantity) {
			return;
		}
	}

	int x = core->Roll(1,critter.creaturecount,-1);
	Actor* cre = gamedata->GetCreature(critter.CreFile[x]);
	if (!cre) {
		return;
	}

	SetVariable(map, critter.SpecVar, critter.SpecContext, specvar+(ieDword) critter.SpecVarInc);
	map->AddActor(cre);
	for (x=0;x<9;x++) {
		if (critter.SetSpec[x]) {
			cre->SetBase(StatValues[x], critter.SetSpec[x]);
		}
	}
	cre->SetPosition( critter.SpawnPoint, 0, 0);//maybe critters could be repositioned
	cre->SetOrientation(critter.Orientation,false);
	if (critter.ScriptName[0]) {
		cre->SetScriptName(critter.ScriptName);
	}
	//increases death variable
	if (critter.Flags&CF_DEATHVAR) {
		cre->AppearanceFlags|=APP_DEATHVAR;
	}
	//increases faction specific variable
	if (critter.Flags&CF_FACTION) {
		cre->AppearanceFlags|=APP_FACTION;
	}
	//increases team specific variable
	if (critter.Flags&CF_TEAM) {
		cre->AppearanceFlags|=APP_TEAM;
	}
	//increases good variable
	if (critter.Flags&CF_GOOD) {
		cre->DeathCounters[DC_GOOD] = critter.DeathCounters[DC_GOOD];
		cre->AppearanceFlags|=APP_GOOD;
	}
	//increases law variable
	if (critter.Flags&CF_LAW) {
		cre->DeathCounters[DC_LAW] = critter.DeathCounters[DC_LAW];
		cre->AppearanceFlags|=APP_LAW;
	}
	//increases lady variable
	if (critter.Flags&CF_LADY) {
		cre->DeathCounters[DC_LADY] = critter.DeathCounters[DC_LADY];
		cre->AppearanceFlags|=APP_LADY;
	}
	//increases murder variable
	if (critter.Flags&CF_MURDER) {
		cre->DeathCounters[DC_MURDER] = critter.DeathCounters[DC_MURDER];
		cre->AppearanceFlags|=APP_MURDER;
	}
	//triggers help from same group
	if (critter.Flags&CF_BUDDY) {
		cre->AppearanceFlags|=APP_BUDDY;
	}

	if (critter.OverrideScript[0]) {
		cre->SetScript(critter.OverrideScript, SCR_OVERRIDE);
	}
	if (critter.ClassScript[0]) {
		cre->SetScript(critter.ClassScript, SCR_CLASS);
	}
	if (critter.RaceScript[0]) {
		cre->SetScript(critter.RaceScript, SCR_RACE);
	}
	if (critter.GeneralScript[0]) {
		cre->SetScript(critter.GeneralScript, SCR_GENERAL);
	}
	if (critter.DefaultScript[0]) {
		cre->SetScript(critter.DefaultScript, SCR_DEFAULT);
	}
	if (critter.AreaScript[0]) {
		cre->SetScript(critter.AreaScript, SCR_AREA);
	}
	if (critter.SpecificScript[0]) {
		cre->SetScript(critter.SpecificScript, SCR_SPECIFICS);
	}
	if (critter.Dialog[0]) {
		cre->SetDialog(critter.Dialog);
	}
}

bool IniSpawn::Schedule(ieDword appearance, ieDword gametime) const
{
        ieDword bit = 1<<((gametime/AI_UPDATE_TIME)%7200/300);
        if (appearance & bit) {
                return true;
        }
	return false;
}

void IniSpawn::SpawnGroup(SpawnEntry &event)
{
	if (!event.critters) {
		return;
	}
	unsigned int interval = event.interval;
	if (interval) {
		if(core->GetGame()->GameTime/interval<=last_spawndate/interval) {
			return;
		}
	}
	last_spawndate=core->GetGame()->GameTime;
	
	for(int i=0;i<event.crittercount;i++) {
		CritterEntry* critter = event.critters+i;
		if (!Schedule(critter->TimeOfDay, last_spawndate) ) {
			continue;
		}
		for(int j=0;j<critter->SpawnCount;j++) {
			SpawnCreature(*critter);
		}
	}
}

//execute the initial spawn
void IniSpawn::InitialSpawn()
{
	SpawnGroup(enterspawn);
	//these variables are set when entering first
	for (int i=0;i<localscount;i++) {
		SetVariable(map, Locals[i].Name,"LOCALS", Locals[i].Value);
	}
}

//FIXME:call this at the right time (this feature is not explored yet, and unused in original dataset)
void IniSpawn::ExitSpawn()
{
	SpawnGroup(exitspawn);
}

//checks if a respawn event occurred
void IniSpawn::CheckSpawn()
{
	for(int i=0;i<eventcount;i++) {
		SpawnGroup(eventspawns[i]);
	}
}

