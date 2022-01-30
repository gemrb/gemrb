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

#include "globals.h"

#include "CharAnimations.h"
#include "Game.h"
#include "GameData.h"
#include "Interface.h"
#include "Map.h"
#include "PluginMgr.h"
#include "ScriptEngine.h"
#include "GameScript/GSUtils.h"
#include "GameScript/Matching.h"
#include "GUI/GameControl.h"
#include "Scriptable/Actor.h"

namespace GemRB {

static const int StatValues[9]={
IE_EA, IE_FACTION, IE_TEAM, IE_GENERAL, IE_RACE, IE_CLASS, IE_SPECIFIC,
IE_SEX, IE_ALIGNMENT };

static std::shared_ptr<DataFileMgr> GetIniFile(const ResRef& DefaultArea)
{
	//the lack of spawn ini files is not a serious problem, happens all the time
	if (!gamedata->Exists( DefaultArea, IE_INI_CLASS_ID)) {
		return {};
	}

	DataStream* inifile = gamedata->GetResource( DefaultArea, IE_INI_CLASS_ID );
	if (!inifile) {
		return {};
	}
	if (!core->IsAvailable( IE_INI_CLASS_ID )) {
		Log(ERROR, "IniSpawn", "No INI Importer Available.");
		return {};
	}

	PluginHolder<DataFileMgr> ini = MakePluginHolder<DataFileMgr>(IE_INI_CLASS_ID);
	ini->Open(inifile);
	return ini;
}

IniSpawn::IniSpawn(Map *owner, const ResRef& DefaultArea)
{
	map = owner;
	NamelessState = 35;
	//high detail level by default
	detail_level = 2;
	core->GetDictionary()->Lookup("Detail Level", detail_level);

	const auto inifile = GetIniFile(DefaultArea);
	if (!inifile) {
		NamelessSpawnArea = DefaultArea;
		return;
	}

	const char *s = inifile->GetKeyAsString("nameless", "destare", DefaultArea);
	NamelessSpawnArea = s;
	s = inifile->GetKeyAsString("nameless","point","[0.0]");
	if (sscanf(s, "[%d.%d]", &NamelessSpawnPoint.x, &NamelessSpawnPoint.y) != 2) {
		NamelessSpawnPoint.reset();
	}

	s = inifile->GetKeyAsString("nameless", "partyarea", DefaultArea);
	PartySpawnArea = s;
	s = inifile->GetKeyAsString("nameless", "partypoint", "[0.0]");
	if (sscanf(s, "[%d.%d]", &PartySpawnPoint.x, &PartySpawnPoint.y) != 2) {
		PartySpawnPoint = NamelessSpawnPoint;
	}

	// animstat.ids values
	//35 - already standing
	//36 - getting up
	NamelessState = inifile->GetKeyAsInt("nameless","state",36);

	auto namelessvarcount = inifile->GetKeysCount("namelessvar");
	NamelessVar.reserve(namelessvarcount);
	for (int y = 0; y < namelessvarcount; ++y) {
		const char* Key = inifile->GetKeyNameByIndex("namelessvar",y);
		auto val = inifile->GetKeyAsInt("namelessvar",Key,0);
		NamelessVar.emplace_back(Key, val);
	}

	auto localscount = inifile->GetKeysCount("locals");
	Locals.reserve(localscount);
	for (int y = 0; y < localscount; ++y) {
		const char* Key = inifile->GetKeyNameByIndex("locals",y);
		auto val = inifile->GetKeyAsInt("locals",Key,0);
		Locals.emplace_back(Key, val);
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
		auto events = GetElements<const char*>(s);
		auto eventcount = events.size();
		eventspawns.resize(eventcount);
		while(eventcount--) {
			ReadSpawnEntry(inifile.get(), events[eventcount], eventspawns[eventcount]);
		}
	}
	//maybe not correct
	InitialSpawn();
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
	if (!stricmp(keyword,"greater_or_equal_to") ) return GREATER_OR_EQUALS; //4 (gemrb ext)
	if (!stricmp(keyword,"not_equal_to") ) return NOT_EQUALS; //5
	return NO_OPERATION;
}

inline bool VarHasContext(const char *str)
{
	return strnlen(str, 40) > 9 && str[6] == ':' && str[7] == ':';
}

inline bool ParsePointDef(const char* pointString, Point& destPoint, int& orient)
{
	int parsed = sscanf(pointString, "[%d%*[,.]%d:%d]", &destPoint.x, &destPoint.y, &orient);
	if (parsed != 3 && sscanf(pointString, "[%d%*[,.]%d]", &destPoint.x, &destPoint.y) != 2) {
		Log(ERROR, "IniSpawn", "Malformed spawn point definition: {}", pointString);
		return false;
	}
	return true;
}

// determine the final spawn point
void IniSpawn::SelectSpawnPoint(CritterEntry &critter) const
{
	if (critter.SpawnMode == 'e') {
		// nothing to do, everyone will use the point stored in the var, which was handled on read
		return;
	}

	const auto spawnPointStrings = GetElements<const char*>(critter.SpawnPointsDef.c_str());
	int spawnCount = static_cast<int>(spawnPointStrings.size());
	Point chosenPoint;
	int orient = -1;

	if (critter.Flags & CF_SAFEST_POINT) {
		// try to find it, otherwise behave as if nothing happened and retry normally
		Point tmp;
		for (const auto& point : spawnPointStrings) {
			if (!ParsePointDef(point, tmp, orient)) {
				continue;
			}
			if (map->IsVisible(tmp)) continue;

			chosenPoint = tmp;
		}
	}

	if (chosenPoint.IsZero()) {
		// only spawn_point_global / spawn_facing_global support 'e'
		int idx = 0;
		if (critter.SpawnMode == 'r') {
			// select one of the spawnpoints randomly
			idx = core->Roll(1, spawnCount, -1);
		} else if (critter.SpawnMode == 'i' && critter.PointSelectVar) {
			// choose a point by spawn index
			idx = CheckVariable(map, critter.PointSelectVar + 8, critter.PointSelectVar) % spawnCount;
		} // else is 's' mode - single

		ParsePointDef(spawnPointStrings[idx], chosenPoint, orient);
	}

	critter.SpawnPoint = chosenPoint;
	if (orient != -1) {
		critter.Orientation = orient;
	} else if (critter.Orientation2 != -1) {
		critter.Orientation = critter.Orientation2;
	} else {
		critter.Orientation = core->Roll(1, 16, -1);
	}

	// store point and/or orientation in a global var
	if (critter.SaveSelectedPoint.CString()[0]) {
		if (VarHasContext(critter.SaveSelectedPoint)) {
			SetPointVariable(map, critter.SaveSelectedPoint + 8, critter.SpawnPoint, critter.SaveSelectedPoint);
		} else {
			SetPointVariable(map, critter.SaveSelectedPoint, critter.SpawnPoint, "GLOBAL");
		}
	}

	if (critter.SaveSelectedFacing.CString()[0]) {
		if (VarHasContext(critter.SaveSelectedFacing)) {
			SetVariable(map, critter.SaveSelectedFacing + 8, critter.Orientation, critter.SaveSelectedFacing);
		} else {
			SetVariable(map, critter.SaveSelectedFacing, critter.Orientation, "GLOBAL");
		}
	}
}

void IniSpawn::PrepareSpawnPoints(const DataFileMgr *iniFile, const char *critterName, CritterEntry &critter) const
{
	// spawn point could be (point_select):
	// s - single
	// r - random
	// e - preset (read from var)
	// i - indexed sequential (read from var)
	// but also find_safest_point can override that
	// NOTE: it affects several following keys
	const char *pointSelect = iniFile->GetKeyAsString(critterName, "point_select", nullptr);
	char spawnMode = 0;
	if (pointSelect) {
		spawnMode = pointSelect[0];
	}
	critter.SpawnMode = spawnMode;

	const char *spawnPoints = iniFile->GetKeyAsString(critterName, "spawn_point", nullptr);
	if (!spawnPoints) {
		Log(ERROR, "IniSpawn", "No spawn points defined, skipping creature: {}", critterName);
		return;
	}
	critter.SpawnPointsDef = spawnPoints;

	// indexed sequential mode
	const char *pointSelectVar = iniFile->GetKeyAsString(critterName, "point_select_var", nullptr);
	if (pointSelectVar) {
		critter.PointSelectVar = pointSelectVar;
	}
	bool incSpawnPointIndex = iniFile->GetKeyAsBool(critterName, "inc_spawn_point_index", false);
	if (incSpawnPointIndex && critter.SpawnMode == 'i') {
		critter.Flags |= CF_INC_INDEX;
	}

	// don't spawn when spawnpoint is visible
	bool ignoreCanSee = iniFile->GetKeyAsBool(critterName, "ignore_can_see", false);
	if (ignoreCanSee) {
		critter.Flags |= CF_IGNORECANSEE;
	}

	// find first in fog, unless ignore_can_see is on
	bool safestPoint = iniFile->GetKeyAsBool(critterName,"find_safest_point", false);
	if (safestPoint && !ignoreCanSee) {
		critter.Flags |= CF_SAFEST_POINT;
	}

	// Keys that store or retrieve spawn point and orientation ("facing").
	// take point from variable
	const char *spawnPointGlobal = iniFile->GetKeyAsString(critterName,"spawn_point_global", nullptr);
	if (spawnPointGlobal && critter.SpawnMode == 'e') {
		critter.SpawnPoint = CheckPointVariable(map, spawnPointGlobal + 8, spawnPointGlobal);
	}

	// take facing from variable
	// NOTE: not replicating original buggy behavior:
	// Due to a bug in the implementation "point_select_var" is also used to
	// determine the creature orientation if "point_select" is set to 'e'
	// However, both attributes had to be specified to work
	const char *spawnFacingGlobal = iniFile->GetKeyAsString(critterName,"spawn_facing_global", nullptr);
	if (spawnFacingGlobal  && critter.SpawnMode == 'e') {
		critter.Orientation = static_cast<int>(CheckVariable(map, spawnFacingGlobal + 8, spawnFacingGlobal));
	}

	// should all create_qty spawns use the same spawn point?
	bool holdSelectedPointKey = iniFile->GetKeyAsBool(critterName, "hold_selected_point_key", false);
	if (holdSelectedPointKey) {
		critter.Flags |= CF_HOLD_POINT;
	}
}

// tags not working in originals either, see IESDP for details:
// control_var, spec_area, check_crowd, spawn_time_of_day, check_view_port (used!) & check_by_view_port
void IniSpawn::ReadCreature(const DataFileMgr *inifile, const char *crittername, CritterEntry &critter) const
{
	const char *s;
	int ps;

	//first assume it is a simple numeric value
	critter.TimeOfDay = (ieDword) inifile->GetKeyAsInt(crittername,"time_of_day", 0xffffffff);

	//at this point critter.TimeOfDay is usually 0xffffffff
	s = inifile->GetKeyAsString(crittername,"time_of_day",NULL);
	if (s && strlen(s)>=24) {
		ieDword value = 0;
		ieDword j = 1;
		for(int i=0;i<24 && s[i];i++) {
			if (s[i]=='0' || s[i]=='o') value |= j;
			j<<=1;
		}
		//turn off individual bits marked by a 24 long string scheduling
		//example: '0000xxxxxxxxxxxxxxxx00000000'
		critter.TimeOfDay^=value;
	}

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

	// PSTEE only
	bool disableRenderer = inifile->GetKeyAsBool(crittername, "disable_renderer", false);
	if (disableRenderer) {
		critter.Flags |= CF_DISABLE_RENDERER;
	}

	//all specvars are using global, but sometimes it is explicitly given
	s = inifile->GetKeyAsString(crittername,"spec_var",NULL);
	if (s) {
		if (VarHasContext(s)) {
			critter.SpecContext.SNPrintF("%.6s", s);
			strnlwrcpy(critter.SpecVar, s+8, 32);
		} else {
			critter.SpecContext = "GLOBAL";
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
		critter.CreFile = GetElements<ResRef>(s);
	} else {
		Log(ERROR, "IniSpawn", "Invalid spawn entry: {}", crittername);
		return;
	}

	PrepareSpawnPoints(inifile, crittername, critter);

	// store point and/or orientation in a global var
	s = inifile->GetKeyAsString(crittername,"save_selected_point",NULL);
	if (s) critter.SaveSelectedPoint = s;

	s = inifile->GetKeyAsString(crittername,"save_selected_facing",NULL);
	if (s) critter.SaveSelectedFacing = s;

	//sometimes only the orientation is given, the point is stored in a variable
	ps = inifile->GetKeyAsInt(crittername,"facing",-1);
	critter.Orientation2 = ps;

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
		ieByte x[9];
		
		ps = sscanf(s,"[%hhu.%hhu.%hhu.%hhu.%hhu.%hhu.%hhu.%hhu.%hhu]", x, x+1, x+2, x+3, x+4, x+5,
			x+6, x+7, x+8);
		if (ps == 0) {
			strnuprcpy(critter.ScriptName, s, 32);
			critter.Flags|=CF_CHECK_NAME;
			memset(critter.Spec,-1,sizeof(critter.Spec));
		} else {
			while(ps--) {
				critter.Spec[ps] = x[ps];
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
		critter.AreaScript = MakeUpperCaseResRef(s);
	}
	//special 2 == class
	s = inifile->GetKeyAsString(crittername,"script_special_2",NULL);
	if (s) {
		critter.ClassScript = MakeUpperCaseResRef(s);
	}
	//special 3 == general
	s = inifile->GetKeyAsString(crittername,"script_special_3",NULL);
	if (s) {
		critter.GeneralScript = MakeUpperCaseResRef(s);
	}
	//team == specific
	s = inifile->GetKeyAsString(crittername,"script_team",NULL);
	if (s) {
		critter.SpecificScript = MakeUpperCaseResRef(s);
	}

	//combat == race
	s = inifile->GetKeyAsString(crittername,"script_combat",NULL);
	if (s) {
		critter.RaceScript = MakeUpperCaseResRef(s);
	}
	//movement == default
	s = inifile->GetKeyAsString(crittername,"script_movement",NULL);
	if (s) {
		critter.DefaultScript = MakeUpperCaseResRef(s);
	}

	//pst script names
	s = inifile->GetKeyAsString(crittername,"script_override",NULL);
	if (s) {
		critter.OverrideScript = MakeUpperCaseResRef(s);
	}
	s = inifile->GetKeyAsString(crittername,"script_class",NULL);
	if (s) {
		critter.ClassScript = MakeUpperCaseResRef(s);
	}
	s = inifile->GetKeyAsString(crittername,"script_race",NULL);
	if (s) {
		critter.RaceScript = MakeUpperCaseResRef(s);
	}
	s = inifile->GetKeyAsString(crittername,"script_general",NULL);
	if (s) {
		critter.GeneralScript = MakeUpperCaseResRef(s);
	}
	s = inifile->GetKeyAsString(crittername,"script_default",NULL);
	if (s) {
		critter.DefaultScript = MakeUpperCaseResRef(s);
	}
	s = inifile->GetKeyAsString(crittername,"script_area",NULL);
	if (s) {
		critter.AreaScript = MakeUpperCaseResRef(s);
	}
	s = inifile->GetKeyAsString(crittername,"script_specifics",NULL);
	if (s) {
		critter.SpecificScript = MakeUpperCaseResRef(s);
	}
	s = inifile->GetKeyAsString(crittername,"dialog",NULL);
	if (s) {
		critter.Dialog = MakeUpperCaseResRef(s);
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

void IniSpawn::ReadSpawnEntry(const DataFileMgr *inifile, const char *entryname, SpawnEntry &entry) const
{
	const char *s;
	entry.name = strdup(entryname);
	entry.interval = (unsigned int) inifile->GetKeyAsInt(entryname,"interval",0);
	if (entry.interval < 15) entry.interval = 15; // lower bound from the original
	//don't default to NULL here, some entries may be missing in original game
	//an empty default string here will create an empty but consistent entry
	s = inifile->GetKeyAsString(entryname,"critters","");
	auto critters = GetElements<const char*>(s);
	size_t crittercount = critters.size();
	entry.critters.resize(crittercount);
	
	while(crittercount--) {
		ReadCreature(inifile, critters[crittercount], entry.critters[crittercount]);
	}
}

/* set by action */
void IniSpawn::SetNamelessDeath(const ResRef& area, const Point &pos, ieDword state)
{
	NamelessSpawnArea = area;
	NamelessSpawnPoint = pos;
	NamelessState = state;
}

/*** events ***/

//respawn nameless after he bit the dust
void IniSpawn::RespawnNameless()
{
	Game *game = core->GetGame();
	Actor *nameless = game->GetPC(0, false);

	// the final fight is fatal
	ieDword finale = 0;
	game->locals->Lookup("Transcendent_Final_Speech", finale);
	if (finale) {
		nameless->Die(NULL);
		core->GetGUIScriptEngine()->RunFunction("GUICommonWindows", "OpenPSTDeathWindow");
		return;
	}

	if (NamelessSpawnPoint.IsZero()) {
		game->JoinParty(nameless, JP_INITPOS);
		NamelessSpawnPoint=nameless->Pos;
		NamelessSpawnArea = nameless->Area;
	}

	nameless->Resurrect(NamelessSpawnPoint);
	// resurrect leaves you at 1hp for raise dead, so manually bump it back to max
	nameless->RefreshEffects(NULL);
	nameless->SetBase(IE_HITPOINTS, 9999);

	// reselect nameless, since he didn't really 'die'
	// this matches the unconditional reselect behavior of the original
	game->SelectActor(nameless, true, SELECT_NORMAL);

	//hardcoded!!!
	if (NamelessState==36) {
		nameless->SetStance(IE_ANI_PST_START);
	}

	game->MovePCs(NamelessSpawnArea, NamelessSpawnPoint, -1);

	//certain variables are set when nameless dies
	for (const auto& var : NamelessVar) {
		SetVariable(game, var.Name.CString(), var.Value, "GLOBAL");
	}
	core->GetGameControl()->ChangeMap(nameless, true);
}

inline void SetScript(Actor *cre, const ResRef& script, int slot)
{
	if (!script.IsEmpty()) {
		cre->SetScript(script, slot);
	}
}

void IniSpawn::SpawnCreature(const CritterEntry &critter) const
{
	if (critter.CreFile.empty()) {
		return;
	}

	ieDword specvar = CheckVariable(map, critter.SpecVar.CString(), critter.SpecContext);

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
		if (map->IsVisible(critter.SpawnPoint)) {
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
		if (map->GetActor(critter.ScriptName.CString(), 0)) {
			return;
		}
	} else {
		Object object;
		//objectfields based on spec
		for (int i = 0; i <= 8; i++) {
			object.objectFields[i] = critter.Spec[i];
		}

		int cnt = GetObjectCount(map, &object);
		if (cnt>=critter.TotalQuantity) {
			return;
		}
	}

	int x = core->Roll(1, int(critter.CreFile.size()), -1);
	Actor* cre = gamedata->GetCreature(critter.CreFile[x]);
	if (!cre) {
		return;
	}

	// TODO: ee, verify and adjust after the action is added
	// disable_renderer = boolean_value
	//   Argent says: It looks like this attribute simply skips the rendering pass for the spawned creature.
	//   This state is not saved. This attribute seems to have the side effect that filtering is ignored —
	//   in my tests the creature was spawned continuously even if spec_qty and create_qty are defined.
	//   This attribute seems to be related to the script action SetRenderable.
	if (critter.Flags & CF_DISABLE_RENDERER) {
		cre->SetBase(IE_AVATARREMOVAL, 1);
	}

	if (critter.Flags & CF_INC_INDEX) {
		int value = CheckVariable(map, critter.PointSelectVar.CString());
		// NOTE: not replicating bug where it would increment the index twice if create_qty > 1
		SetVariable(map, critter.PointSelectVar.CString(), value + 1);
	}

	SetVariable(map, critter.SpecVar.CString(), specvar + (ieDword) critter.SpecVarInc, critter.SpecContext);
	map->AddActor(cre, true);
	for (x=0;x<9;x++) {
		if (critter.SetSpec[x]) {
			cre->SetBase(StatValues[x], critter.SetSpec[x]);
		}
	}
	cre->SetPosition(critter.SpawnPoint, 1, 0);
	cre->SetOrientation(critter.Orientation,false);

	cre->SetScriptName(critter.ScriptName.CString());

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

	SetScript(cre, critter.OverrideScript, SCR_OVERRIDE);
	SetScript(cre, critter.ClassScript, SCR_CLASS);
	SetScript(cre, critter.RaceScript, SCR_RACE);
	SetScript(cre, critter.GeneralScript, SCR_GENERAL);
	SetScript(cre, critter.DefaultScript, SCR_DEFAULT);
	SetScript(cre, critter.AreaScript, SCR_AREA);
	SetScript(cre, critter.SpecificScript, SCR_SPECIFICS);

	if (!critter.Dialog.IsEmpty()) {
		cre->SetDialog(critter.Dialog);
	}
}

void IniSpawn::SpawnGroup(SpawnEntry &event) const
{
	if (event.critters.empty()) {
		return;
	}
	unsigned int interval = event.interval;
	ieDword gameTime = core->GetGame()->GameTime;
	// gameTime can be 0 for the first area, so make sure to not exit prematurely
	if (interval && gameTime) {
		if (event.lastSpawndate + interval >= gameTime) {
			return;
		}
	}
	
	for (auto& critter : event.critters) {
		if (!Schedule(critter.TimeOfDay, event.lastSpawndate) ) {
			continue;
		}
		for(int j = 0; j < critter.SpawnCount; ++j) {
			// try a potentially different location unless specified
			if (j == 0 || !(critter.Flags & CF_HOLD_POINT)) {
				SelectSpawnPoint(critter);
			}
			SpawnCreature(critter);
		}
		event.lastSpawndate = gameTime;
	}
}

//execute the initial spawn
void IniSpawn::InitialSpawn()
{
	SpawnGroup(enterspawn);
	//these variables are set when entering first
	for (const auto& local : Locals) {
		SetVariable(map, local.Name.CString(), local.Value, "LOCALS");
	}

	// move the rest of the party if needed
	if (!PartySpawnPoint.IsZero()) {
		Game *game = core->GetGame();
		while (game->GetPartySize(false) > 1) {
			Actor *pc = game->GetPC(1, false); // skip TNO
			pc->Stop();
			MoveBetweenAreasCore(pc, PartySpawnArea, PartySpawnPoint, -1, true);
			game->LeaveParty(pc);
		}
	}
}

void IniSpawn::ExitSpawn()
{
	SpawnGroup(exitspawn);
}

//checks if a respawn event occurred
void IniSpawn::CheckSpawn()
{
	for (SpawnEntry& event : eventspawns) {
		SpawnGroup(event);
	}
}


}
