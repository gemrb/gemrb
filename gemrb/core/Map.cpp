/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003-2004 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

// This class represents the .ARE (game area) files in the engine

#include "Map.h"

#include "Ambient.h"
#include "AmbientMgr.h"
#include "Audio.h"
#include "DisplayMessage.h"
#include "Game.h"
#include "GameData.h"
#include "IniSpawn.h"
#include "MapMgr.h"
#include "MusicMgr.h"
#include "ImageMgr.h"
#include "Palette.h"
#include "Particles.h"
#include "PluginMgr.h"
#include "Projectile.h"
#include "SaveGameIterator.h"
#include "ScriptedAnimation.h"
#include "TileMap.h"
#include "VEFObject.h"
#include "Video/Video.h"
#include "strrefs.h"
#include "ie_cursors.h"
#include "GameScript/GSUtils.h"
#include "GUI/GameControl.h"
#include "GUI/Window.h"
#include "RNG.h"
#include "Scriptable/Container.h"
#include "Scriptable/Door.h"
#include "Scriptable/InfoPoint.h"
#include "System/StringBuffer.h"

#include <array>
#include <cassert>
#include <limits>
#include <utility>
#include <unordered_map>

namespace GemRB {

#define YESNO(x) ( (x)?"Yes":"No")

struct Spawns {
	ResRefMap<SpawnGroup> vars;
	
	static const Spawns& Get() {
		static Spawns spawns;
		return spawns;
	}

private:
	Spawns() noexcept {
		AutoTable tab = gamedata->LoadTable("spawngrp", true);

		if (!tab)
			return;

		int i = tab->GetColNamesCount();
		while (i--) {
			int j=tab->GetRowCount();
			std::vector<ResRef> resrefs(j);
			while (j--) {
				const char *crename = tab->QueryField( j,i );
				if (strcmp(crename, tab->QueryDefault()) != 0) break;
			}
			if (j>0) {
				//difficulty
				int level = atoi(tab->QueryField(0, i));
				for (;j;j--) {
					resrefs[j - 1] = tab->QueryField(j, i);
				}
				ResRef GroupName = MakeLowerCaseResRef(tab->GetColumnName(i));
				vars.insert(std::make_pair(GroupName, SpawnGroup(std::move(resrefs), level)));
			}
		}
	}
};

struct Explore {
	int LargeFog;
	static constexpr int MaxVisibility = 30;
	int VisibilityPerimeter; //calculated from MaxVisibility
	std::array<std::vector<Point>, MaxVisibility> VisibilityMasks;

	static const Explore& Get() {
		static Explore explore;
		return explore;
	}

private:
	void AddLOS(int destx, int desty, int slot)
	{
		for (int i=0;i<MaxVisibility;i++) {
			int x = ((destx*i + MaxVisibility/2) / MaxVisibility) * 16;
			int y = ((desty*i + MaxVisibility/2) / MaxVisibility) * 12;
			if (LargeFog) {
				x += 16;
				y += 12;
			}
			VisibilityMasks[i][slot].x = x;
			VisibilityMasks[i][slot].y = y;
		}
	}

	Explore() noexcept {
		LargeFog = !core->HasFeature(GF_SMALL_FOG);

		//circle perimeter size for MaxVisibility
		int x = MaxVisibility;
		int y = 0;
		int xc = 1 - ( 2 * MaxVisibility );
		int yc = 1;
		int re = 0;
		VisibilityPerimeter = 0;
		while (x>=y) {
			VisibilityPerimeter+=8;
			y++;
			re += yc;
			yc += 2;
			if (( ( 2 * re ) + xc ) > 0) {
				x--;
				re += xc;
				xc += 2;
			}
		}

		for (int i = 0; i < MaxVisibility; i++) {
			VisibilityMasks[i].resize(VisibilityPerimeter);
		}

		x = MaxVisibility;
		y = 0;
		xc = 1 - ( 2 * MaxVisibility );
		yc = 1;
		re = 0;
		VisibilityPerimeter = 0;
		while (x>=y) {
			AddLOS (x, y, VisibilityPerimeter++);
			AddLOS (-x, y, VisibilityPerimeter++);
			AddLOS (-x, -y, VisibilityPerimeter++);
			AddLOS (x, -y, VisibilityPerimeter++);
			AddLOS (y, x, VisibilityPerimeter++);
			AddLOS (-y, x, VisibilityPerimeter++);
			AddLOS (-y, -x, VisibilityPerimeter++);
			AddLOS (y, -x, VisibilityPerimeter++);
			y++;
			re += yc;
			yc += 2;
			if (( ( 2 * re ) + xc ) > 0) {
				x--;
				re += xc;
				xc += 2;
			}
		}
	}
};

// TODO: fix this hardcoded resource reference
static const ResRef PortalResRef = "EF03TPR3";
static unsigned int PortalTime = 15;
static unsigned int MAX_CIRCLESIZE = 8;

static ieDword oldGameTime = 0;

static inline AnimationObjectType SelectObject(const Actor *actor, int q, const AreaAnimation *a, const VEFObject *sca, const Particles *spark, const Projectile *pro, const Container *pile)
{
	int actorh;
	if (actor) {
		actorh = actor->Pos.y;
		if (q) actorh = 0;
	} else {
		actorh = 0x7fffffff;
	}

	int aah;
	if (a) {
		//aah = a->Pos.y;//+a->height;
		aah = a->GetHeight();
	} else {
		aah = 0x7fffffff;
	}

	int scah;
	if (sca) {
		scah = sca->Pos.y;//+sca->ZPos;
	} else {
		scah = 0x7fffffff;
	}

	int spah;
	if (spark) {
		//no idea if this should be plus or minus (or here at all)
		spah = spark->GetHeight();//+spark->pos.h;
	} else {
		spah = 0x7fffffff;
	}

	int proh;
	if (pro) {
		proh = pro->GetHeight();
	} else {
		proh = 0x7fffffff;
	}

	// piles should always be drawn last, except if there is a corpse in the way
	if (actor && (actor->GetStat(IE_STATE_ID) & STATE_DEAD)) {
		return AOT_ACTOR;
	}
	if (pile) {
		return AOT_PILE;
	}

	if (proh<actorh && proh<scah && proh<aah && proh<spah) return AOT_PROJECTILE;

	if (spah<actorh && spah<scah && spah<aah) return AOT_SPARK;

	if (aah<actorh && aah<scah) return AOT_AREA;

	if (scah<actorh) return AOT_SCRIPTED;

	return AOT_ACTOR;
}

//returns true if creature must be embedded in the area
//npcs in saved game shouldn't be embedded either
static inline bool MustSave(const Actor *actor)
{
	if (actor->Persistent()) {
		return false;
	}

	//check for familiars, summons?
	return true;
}

Point Map::ConvertCoordToTile(const Point& p)
{
	return Point(p.x / 16, p.y / 12);
}

Point Map::ConvertCoordFromTile(const Point& p)
{
	return Point(p.x * 16, p.y * 12);
}

Map::Map(TileMap *tm, Holder<Sprite2D> props, Holder<Sprite2D> sm)
: Scriptable(ST_AREA), TMap(tm), SmallMap(std::move(sm)),
ExploredBitmap(FogMapSize()), VisibleBitmap(FogMapSize()),
reverb(*this),
tileProps(std::move(props))
{
	ExploredBitmap.fill(0);
	VisibleBitmap.fill(0);
	
	area=this;
	queue[PR_SCRIPT] = NULL;
	queue[PR_DISPLAY] = NULL;
	INISpawn = NULL;
	//no one needs this queue
	//queue[PR_IGNORE] = NULL;
	Qcount[PR_SCRIPT] = 0;
	Qcount[PR_DISPLAY] = 0;
	//no one needs this queue
	//Qcount[PR_IGNORE] = 0;
	lastActorCount[PR_SCRIPT] = 0;
	lastActorCount[PR_DISPLAY] = 0;
	//no one needs this
	//lastActorCount[PR_IGNORE] = 0;
	version = 0;
	MasterArea = core->GetGame()->MasterArea(scriptName.CString());
	Background = NULL;
	BgDuration = 0;
	LastGoCloser = 0;
	AreaFlags = AreaDifficulty = 0;
	trackString = trackFlag = trackDiff = 0;
	RestHeader.Difficulty = RestHeader.CreatureNum = RestHeader.Maximum = RestHeader.Enabled = 0;
	RestHeader.DayChance = RestHeader.NightChance = RestHeader.sduration = RestHeader.rwdist = RestHeader.owdist = 0;
	SongHeader.reverbID = SongHeader.MainDayAmbientVol = SongHeader.MainNightAmbientVol = 0;
	wallStencil = NULL;
}

Map::~Map(void)
{
	//close the current container if it was owned by this map, this avoids a crash
	const Container *c = core->GetCurrentContainer();
	if (c && c->GetCurrentArea()==this) {
		core->CloseCurrentContainer();
	}

	delete TMap;
	delete INISpawn;

	for (auto actor : actors) {
		//don't delete NPC/PC
		if (actor && !actor->Persistent()) {
			delete actor;
		}
	}

	for (auto entrance : entrances) {
		delete entrance;
	}
	for (auto spawn : spawns) {
		delete spawn;
	}

	for (auto& q : queue) {
		free(q);
		q = nullptr;
	}

	for (auto projectile : projectiles) {
		delete projectile;
	}

	for (auto vvc : vvcCells) {
		delete vvc;
	}

	for (auto particle : particles) {
		delete particle;
	}

	AmbientMgr *ambim = core->GetAudioDrv()->GetAmbientMgr();
	ambim->reset();
	for (auto ambient : ambients) {
		delete ambient;
	}
}

void Map::SetTileMapProps(Holder<Sprite2D> props)
{
	tileProps = std::move(props);
}

void Map::AutoLockDoors() const
{
	GetTileMap()->AutoLockDoors();
}

void Map::MoveToNewArea(const ResRef &area, const char *entrance, unsigned int direction, int EveryOne, Actor *actor) const
{
	char command[256];

	//change loader MOS image here
	//check worldmap entry, if that doesn't contain anything,
	//make a random pick

	Game* game = core->GetGame();
	if (EveryOne & CT_GO_CLOSER) {
		//copy the area name if it exists on the worldmap
		unsigned int index;

		const WMPAreaEntry* entry = core->GetWorldMap()->FindNearestEntry(area, index);
		if (entry) {
			game->PreviousArea = entry->AreaName;
		}

		//perform autosave
		core->GetSaveGameIterator()->CreateSaveGame(0, false);
	}
	const Map *map = game->GetMap(area, false);
	if (!map) {
		Log(ERROR, "Map", "Invalid map: %s", area.CString());
		command[0]=0;
		return;
	}
	const Entrance *ent = nullptr;
	if (entrance[0]) {
		ent = map->GetEntrance( entrance );
		if (!ent) {
			Log(ERROR, "Map", "Invalid entrance '%s' for area %s", entrance, area.CString());
		}
	}
	int X,Y, face;
	if (!ent) {
		// no entrance found, try using direction flags

		face = -1; // should this be handled per-case?

		// ok, so the original engine tries these in a different order
		// (north first, then south) but it doesn't seem to matter
		if (direction & ADIRF_NORTH) {
			X = map->TMap->XCellCount * 32;
			Y = 0;
		} else if (direction & ADIRF_EAST) {
			X = map->TMap->XCellCount * 64;
			Y = map->TMap->YCellCount * 32;
		} else if (direction & ADIRF_SOUTH) {
			X = map->TMap->XCellCount * 32;
			Y = map->TMap->YCellCount * 64;
		} else if (direction & ADIRF_WEST) {
			X = 0;
			Y = map->TMap->YCellCount * 32;
		} else if (direction & ADIRF_CENTER) {
			X = map->TMap->XCellCount * 32;
			Y = map->TMap->YCellCount * 32;
		} else {
			// crashes in original engine
			Log(WARNING, "Map", "WARNING!!! EntryPoint '%s' does not exist and direction %d is invalid",
				entrance, direction);
			X = map->TMap->XCellCount * 64;
			Y = map->TMap->YCellCount * 64;
		}
	} else {
		X = ent->Pos.x;
		Y = ent->Pos.y;
		face = ent->Face;
	}
	//LeaveArea is the same in ALL engine versions
	snprintf(command, sizeof(command), "LeaveArea(\"%s\",[%d.%d],%d)", area.CString(), X, Y, face);

	if (EveryOne&CT_GO_CLOSER) {
		int i=game->GetPartySize(false);
		while (i--) {
			Actor *pc = game->GetPC(i,false);
			if (pc->GetCurrentArea()==this) {
			  pc->MovementCommand(command);
			}
		}
		i = game->GetNPCCount();
		while(i--) {
			Actor *npc = game->GetNPC(i);
			if ((npc->GetCurrentArea()==this) && (npc->GetStat(IE_EA)<EA_GOODCUTOFF) ) {
				npc->MovementCommand(command);
			}
		}
		return;
	}
	if (EveryOne&CT_SELECTED) {
		int i=game->GetPartySize(false);
		while (i--) {
			Actor *pc = game->GetPC(i,false);

			if (!pc->IsSelected()) {
				continue;
			}
			if (pc->GetCurrentArea()==this) {
				pc->MovementCommand(command);
			}
		}
		i = game->GetNPCCount();
		while(i--) {
			Actor *npc = game->GetNPC(i);
			if (npc->IsSelected() && (npc->GetCurrentArea()==this)) {
				npc->MovementCommand(command);
			}
		}
		return;
	}

	actor->MovementCommand(command);
}

void Map::UseExit(Actor *actor, InfoPoint *ip)
{
	const Game *game = core->GetGame();

	int EveryOne = ip->CheckTravel(actor);
	switch(EveryOne) {
	case CT_GO_CLOSER:
		if (LastGoCloser<game->Ticks) {
			displaymsg->DisplayConstantString(STR_WHOLEPARTY, DMC_WHITE); //white
			LastGoCloser = game->Ticks+6000;
		}
		if (game->EveryoneStopped()) {
			ip->Flags&=~TRAP_RESET; //exit triggered
		}
		return;
		//no ingame message for these events
	case CT_CANTMOVE: case CT_SELECTED:
		return;
	case CT_ACTIVE: case CT_WHOLE: case CT_MOVE_SELECTED:
		break;
	}

	if (ip->Destination[0] != 0) {
		// the 0 here is default orientation, can infopoints specify that or
		// is an entrance always provided?
		MoveToNewArea(ip->Destination, ip->EntranceName, 0, EveryOne, actor);
		return;
	}
	if (ip->Scripts[0]) {
		ip->AddTrigger(TriggerEntry(trigger_entered, actor->GetGlobalID()));
		// FIXME
		ip->ExecuteScript( 1 );
		ip->ProcessActions();
	}
}

//Draw two overlapped animations to achieve the original effect
//PlayOnce makes sure that if we stop drawing them, they will go away
void Map::DrawPortal(const InfoPoint *ip, int enable)
{
	ieDword gotportal = HasVVCCell(PortalResRef, ip->Pos);

	if (enable) {
		if (gotportal>PortalTime) return;
		ScriptedAnimation *sca = gamedata->GetScriptedAnimation(PortalResRef, false);
		if (sca) {
			sca->SetBlend();
			sca->PlayOnce();
			//exact position, because HasVVCCell depends on the coordinates, PST had no coordinate offset anyway
			sca->Pos = ip->Pos;
			//this is actually ordered by time, not by height
			sca->ZOffset = gotportal;
			AddVVCell( new VEFObject(sca));
		}
		return;
	}
}

void Map::UpdateScripts()
{
	bool has_pcs = false;
	for (auto actor : actors) {
		if (actor->InParty) {
			has_pcs = true;
			break;
		}
	}

	GenerateQueues();
	SortQueues();

	// if masterarea, then we allow 'any' actors
	// if not masterarea, we allow only players
	// if (!GetActorCount(MasterArea) ) {
	// fuzzie changed this because the previous code was wrong
	// (GetActorCount(false) returns only non-PCs) - it is not
	// well-tested so feel free to change if there are problems
	// (for example, the CanFree seems like it would be needed to
	// check for any running scripts, such as following, but it seems
	// to work ok anyway in my testing - if you change it you probably
	// also want to change the actor updating code below so it doesn't
	// add new actions while we are trying to get rid of the area!)
	if (!has_pcs && !(MasterArea && !actors.empty()) /*&& !CanFree()*/) {
		return;
	}

	// fuzzie added this check because some area scripts (eg, AR1600 when
	// escaping Brynnlaw) were executing after they were meant to be done,
	// and this seems the nicest way of handling that for now - it's quite
	// possibly wrong (so if you have problems, revert this and find
	// another way)
	if (has_pcs) {
		//Run all the Map Scripts (as in the original)
		//The default area script is in the last slot anyway
		//ExecuteScript( MAX_SCRIPTS );
		Update();
	} else {
		ProcessActions();
	}

	// If scripts frozen, return.
	// This fixes starting a new IWD game. The above ProcessActions pauses the
	// game for a textscreen, but one of the actor->ProcessActions calls
	// below starts a cutscene, hiding the mouse. - wjp, 20060805
	if (core->GetGameControl()->GetDialogueFlags() & DF_FREEZE_SCRIPTS) return;

	Game *game = core->GetGame();
	bool timestop = game->IsTimestopActive();
	if (!timestop) {
		game->SetTimestopOwner(NULL);
	}
	
	ieDword time = game->Ticks; // make sure everything moves at the same time

	//Run actor scripts (only for 0 priority)
	int q = Qcount[PR_SCRIPT];
	while (q--) {
		Actor* actor = queue[PR_SCRIPT][q];
		//actor just moved away, don't run its script from this side
		if (actor->GetCurrentArea()!=this) {
			continue;
		}

		if (game->TimeStoppedFor(actor)) {
			continue;
		}

		//Avenger moved this here from ApplyAllEffects (this one modifies the effect queue)
		//.. but then fuzzie moved this here from UpdateActorState, because otherwise
		//immobile actors (see check below) never become mobile again!
		//Avenger again: maybe this should be before the timestop check above
		//definitely try to move it up if you experience freezes after timestop
		actor->fxqueue.Cleanup();

		//if the actor is immobile, don't run the scripts
		//FIXME: this is not universally true, only some states have this effect
		// paused targets do something similar, but are handled in the effect
		if (!game->StateOverrideFlag && !game->StateOverrideTime) {
			//it looks like STATE_SLEEP allows scripts, probably it is STATE_HELPLESS what disables scripts
			//if that isn't true either, remove this block completely
			if (actor->GetStat(IE_STATE_ID) & STATE_HELPLESS) {
				actor->SetInternalFlag(IF_JUSTDIED, OP_NAND);
				continue;
			}
		}

		/*
		 * we run scripts all at once because one of the actions in ProcessActions
		 * might remove us from a cutscene and then bad things can happen when
		 * scripts are queued unexpectedly (such as an ogre in a cutscene -> dialog
		 * -> cutscene transition in the first bg1 cutscene exploiting the race
		 * condition to murder player1) - it is entirely possible that we should be
		 * doing this differently (for example by storing the cutscene state at the
		 * start of this function, or by changing the cutscene state at a later
		 * point, etc), but i did it this way for now because it seems least painful
		 * and we should probably be staggering the script executions anyway
		 */
		actor->Update();
		actor->UpdateActorState();
		actor->SetSpeed(false);
		
		if (actor->GetRandomBackoff()) {
			actor->DecreaseBackoff();
			if (!actor->GetRandomBackoff() && actor->GetSpeed() > 0) {
				actor->NewPath();
			}
		} else if (actor->GetStep() && actor->GetSpeed()) {
			// Make actors pathfind if there are others nearby
			// in order to avoid bumping when possible
			const Actor* nearActor = GetActorInRadius(actor->Pos, GA_NO_DEAD|GA_NO_UNSCHEDULED, actor->GetAnims()->GetCircleSize());
			if (nearActor && nearActor != actor) {
				actor->NewPath();
			}
			DoStepForActor(actor, time);
		} else {
			DoStepForActor(actor, time);
		}
	}

	//clean up effects on dead actors too
	q = Qcount[PR_DISPLAY];
	while(q--) {
		Actor* actor = queue[PR_DISPLAY][q];
		actor->fxqueue.Cleanup();
	}

	//Check if we need to start some door scripts
	int doorCount = 0;
	while (true) {
		Door* door = TMap->GetDoor( doorCount++ );
		if (!door)
			break;
		door->Update();
	}

	//Check if we need to start some container scripts
	int containerCount = 0;
	while (true) {
		Container* container = TMap->GetContainer( containerCount++ );
		if (!container)
			break;
		container->Update();
	}

	//Check if we need to start some trap scripts
	int ipCount = 0;
	while (true) {
		//For each InfoPoint in the map
		InfoPoint* ip = TMap->GetInfoPoint( ipCount++ );
		if (!ip)
			break;

		if (ip->IsPortal()) {
			DrawPortal(ip, ip->Trapped&PORTAL_TRAVEL);
		}

		//If this InfoPoint has no script and it is not a Travel Trigger, skip it
		// InfoPoints of all types don't run scripts if TRAP_DEACTIVATED is set
		// (eg, TriggerActivation changes this, see lightning room from SoA)
		int wasActive = (!(ip->Flags&TRAP_DEACTIVATED) ) || (ip->Type==ST_TRAVEL);
		if (!wasActive) continue;

		if (ip->Type == ST_TRIGGER) {
			ip->Update();
			continue;
		}

		q = Qcount[PR_SCRIPT];
		ieDword exitID = ip->GetGlobalID();
		while (q--) {
			Actor *actor = queue[PR_SCRIPT][q];
			if (ip->Type == ST_PROXIMITY) {
				if (ip->Entered(actor)) {
					// if trap triggered, then mark actor
					actor->SetInTrap(ipCount);
					wasActive |= _TRAP_USEPOINT;
				}
			} else {
				// ST_TRAVEL
				// don't move if doing something else
				// added CurrentAction as part of blocking action fixes
				if (actor->CannotPassEntrance(exitID)) {
					continue;
				}
				// this is needed, otherwise the travel
				// trigger would be activated anytime
				// Well, i don't know why is it here, but lets try this
				if (ip->Entered(actor)) {
					UseExit(actor, ip);
				}
			}
		}

		// Play the PST specific enter sound
		if (wasActive & _TRAP_USEPOINT) {
			core->GetAudioDrv()->Play(ip->EnterWav, SFX_CHAN_ACTIONS,
				ip->TrapLaunch);
		}
		ip->Update();
	}

	UpdateSpawns();
	GenerateQueues();
	SortQueues();
}

ResRef Map::ResolveTerrainSound(const ResRef& resref, const Point &p) const
{
	struct TerrainSounds {
		std::map<ResRef, std::array<ResRef, 16>> refs;
		
		TerrainSounds() noexcept {
			AutoTable tm = gamedata->LoadTable("terrain");
			
			int rc = tm->GetRowCount() - 2;
			while (rc--) {
				ResRef group = tm->GetRowName(rc+2);
				refs[group] = {};
				int i = 0;
				for (auto& ref : refs[group]) {
					ref = tm->QueryField(rc + 2, i++);
				}
			}
		}
	} static const terrainsounds;
	
	if (terrainsounds.refs.count(resref)) {
		uint8_t type = tileProps->GetPixel(Map::ConvertCoordToTile(p)).g;
		const auto& array = terrainsounds.refs.at(resref);
		return array[type];
	}

	return ResRef();
}

void Map::DoStepForActor(Actor *actor, ieDword time) const
{
	int walkScale = actor->GetSpeed();
	// Immobile, dead and actors in another map can't walk here
	if (actor->Immobile() || walkScale == 0 || actor->GetCurrentArea() != this
		|| !actor->ValidTarget(GA_NO_DEAD)) {
		return;
	}

	if (!(actor->GetBase(IE_STATE_ID)&STATE_CANTMOVE) ) {
		actor->DoStep(walkScale, time);
	}
}

void Map::ClearSearchMapFor(const Movable *actor) {
	std::vector<Actor *> nearActors = GetAllActorsInRadius(actor->Pos, GA_NO_SELF|GA_NO_DEAD|GA_NO_LOS|GA_NO_UNSCHEDULED, MAX_CIRCLE_SIZE*3, actor);
	BlockSearchMap(actor->Pos, actor->size, PathMapFlags::UNMARKED);

	// Restore the searchmap areas of any nearby actors that could
	// have been cleared by this BlockSearchMap(..., PathMapFlags::UNMARKED).
	// (Necessary since blocked areas of actors may overlap.)
	for (const Actor *neighbour : nearActors) {
		if (neighbour->BlocksSearchMap()) {
			BlockSearchMap(neighbour->Pos, neighbour->size, neighbour->IsPartyMember() ? PathMapFlags::PC : PathMapFlags::NPC);
		}
	}
}

Size Map::FogMapSize() const
{
	// Ratio of bg tile size and fog tile size
	constexpr int CELL_RATIO = 2;
	const int largefog = Explore::Get().LargeFog;
	return Size(TMap->XCellCount * CELL_RATIO + largefog, TMap->YCellCount * CELL_RATIO + largefog);
}

Size Map::PropsSize() const noexcept
{
	return tileProps->Frame.size;
}

// Returns true if map at (x;y) was explored, else false.
bool Map::FogTileUncovered(const Point &p, const Bitmap* mask) const
{
	if (mask == nullptr) return true;

	// out of bounds is always foggy
	return mask->GetAt(p, false);
}

void Map::DrawFogOfWar(const Bitmap* explored_mask, const Bitmap* visible_mask, const Region& vp) const
{
	// Size of Fog-Of-War shadow tile (and bitmap)
	constexpr int CELL_SIZE = 32;
	
	// the amount of fuzzing to apply to map edges wehn the viewport overscans
	constexpr int FUZZ_AMT = 8;

	// size for explored_mask and visible_mask
	const Size fogSize = FogMapSize();

	const int largefog = Explore::Get().LargeFog;
	const Point start = Clamp(ConvertPointToFog(vp.origin), Point(), Point(fogSize.w, fogSize.h));
	const Point end = Clamp(ConvertPointToFog(vp.Maximum()) + Point(2 + largefog, 2 + largefog), Point(), Point(fogSize.w, fogSize.h));
	const int x0 = (start.x * CELL_SIZE - vp.x) - (largefog * CELL_SIZE / 2);
	const int y0 = (start.y * CELL_SIZE - vp.y) - (largefog * CELL_SIZE / 2);
	
	const Size mapSize = GetSize();
	
	enum Directions : uint8_t {
		N = 1,
		W = 2,
		NW = N|W,
		S = 4,
		SW = S|W,
		E = 8,
		NE = N|E,
		SE = S|E
	};
		
	Video* vid = core->GetVideoDriver();
	if (vp.y < 0) { // top border
		Region r(0, 0, vp.w, -vp.y);
		vid->DrawRect(r, ColorBlack, true);
		r.y += r.h;
		r.h = FUZZ_AMT;
		for (int x = r.x + x0; x < r.w; x += CELL_SIZE) {
			vid->BlitSprite(core->FogSprites[N], Point(x, r.y), &r);
		}
	}
	
	if (vp.y + vp.h > mapSize.h) { // bottom border
		Region r(0, mapSize.h - vp.y, vp.w, vp.y + vp.h - mapSize.h);
		vid->DrawRect(r, ColorBlack, true);
		r.y -= FUZZ_AMT;
		r.h = FUZZ_AMT;
		for (int x = r.x + x0; x < r.w; x += CELL_SIZE) {
			vid->BlitSprite(core->FogSprites[S], Point(x, r.y), &r);
		}
	}
	
	if (vp.x < 0) { // left border
		Region r(0, std::max(0, -vp.y), -vp.x, mapSize.h);
		vid->DrawRect(r, ColorBlack, true);
		r.x += r.w;
		r.w = FUZZ_AMT;
		for (int y = r.y + y0; y < r.h; y += CELL_SIZE) {
			vid->BlitSprite(core->FogSprites[W], Point(r.x, y), &r);
		}
	}
	
	if (vp.x + vp.w > mapSize.w) { // right border
		Region r(mapSize.w -vp.x, std::max(0, -vp.y), vp.x + vp.w - mapSize.w, mapSize.h);
		vid->DrawRect(r, ColorBlack, true);
		r.x -= FUZZ_AMT;
		r.w = FUZZ_AMT;
		for (int y = r.y + y0; y < r.h; y += CELL_SIZE) {
			vid->BlitSprite(core->FogSprites[E], Point(r.x, y), &r);
		}
	}
	
	auto IsExplored = [=, &explored_mask](int x, int y) {
		return FogTileUncovered(Point(x, y), explored_mask);
	};
	
	auto IsVisible = [=, &visible_mask](int x, int y) {
		return FogTileUncovered(Point(x, y), visible_mask);
	};
	
	auto ConvertPointToScreen = [=](int x, int y) {
		x = (x - start.x) * CELL_SIZE + x0;
		y = (y - start.y) * CELL_SIZE + y0;
		return Point(x, y);
	};
	
	auto FillFog = [=](int x, int y, int count, BlitFlags flags) {
		Region r(ConvertPointToScreen(x, y), Size(CELL_SIZE * count, CELL_SIZE));
		vid->DrawRect(r, ColorBlack, true, flags);
	};
	
	auto Fill = [=](int x, int y, uint8_t dirs, BlitFlags flags) {
		// If an explored tile is adjacent to an
		//   unexplored one, we draw border sprite
		//   (gradient black <-> transparent)
		// Tiles in four cardinal directions have these
		//   values.
		//
		//      1
		//    2   8
		//      4
		//
		// Values of those unexplored are
		//   added together, the resulting number being
		//   an index of shadow sprite to use. For now,
		//   some tiles are made 'on the fly' by
		//   drawing two or more tiles

		assert((dirs & 0xf0) == 0);

		Point p = ConvertPointToScreen(x, y);
		switch (dirs & 0x0f) {
			case N:
			case W:
			case NW:
			case S:
			case SW:
			case E:
			case NE:
			case SE:
				vid->BlitGameSprite(core->FogSprites[dirs], p, flags);
				return true;
			case N|S:
				vid->BlitGameSprite(core->FogSprites[N], p, flags);
				vid->BlitGameSprite(core->FogSprites[S], p, flags);
				return true;
			case NW|SW:
				vid->BlitGameSprite(core->FogSprites[NW], p, flags);
				vid->BlitGameSprite(core->FogSprites[SW], p, flags);
				return true;
			case W|E:
				vid->BlitGameSprite(core->FogSprites[W], p, flags);
				vid->BlitGameSprite(core->FogSprites[E], p, flags);
				return true;
			case NW|NE:
				vid->BlitGameSprite(core->FogSprites[NW], p, flags);
				vid->BlitGameSprite(core->FogSprites[NE], p, flags);
				return true;
			case NE|SE:
				vid->BlitGameSprite(core->FogSprites[NE], p, flags);
				vid->BlitGameSprite(core->FogSprites[SE], p, flags);
				return true;
			case SW|SE:
				vid->BlitGameSprite(core->FogSprites[SW], p, flags);
				vid->BlitGameSprite(core->FogSprites[SE], p, flags);
				return true;
			default: // a fully surrounded tile is filled
				return false;
		}
	};
	
	const static BlitFlags opaque = BlitFlags::NONE;
	const static BlitFlags trans = BlitFlags::HALFTRANS | BlitFlags::BLENDED;
	
	auto FillExplored = [=](int x, int y) {
		int dirs = !IsExplored(x, y - 1); // N
		if (!IsExplored(x - 1, y)) dirs |= W;
		if (!IsExplored(x, y + 1)) dirs |= S;
		if (!IsExplored(x + 1, y )) dirs |= E;

		if (dirs && !Fill(x, y, dirs, BlitFlags::BLENDED)) {
			FillFog(x, y, 1, opaque);
		}
	};
	
	auto FillVisible = [=](int x, int y) {
		int dirs = !IsVisible( x, y - 1); // N
		if (!IsVisible(x - 1, y)) dirs |= W;
		if (!IsVisible(x, y + 1)) dirs |= S;
		if (!IsVisible(x + 1, y)) dirs |= E;

		if (dirs && !Fill(x, y, dirs, trans)) {
			FillFog(x, y, 1, trans);
		}
	};

	for (int y = start.y; y < end.y; y++) {
		int unexploredQueue = 0;
		int shroudedQueue = 0;
		int x = start.x;
		for (; x < end.x; x++) {
			if (IsExplored(x, y)) {
				if (unexploredQueue) {
					FillFog(x - unexploredQueue, y, unexploredQueue, opaque);
					unexploredQueue = 0;
				}
				
				if (IsVisible(x, y)) {
					if (shroudedQueue) {
						FillFog(x - shroudedQueue, y, shroudedQueue, trans);
						shroudedQueue = 0;
					}
					FillVisible(x, y);
				} else {
					// coalese all horizontally adjacent shrouded cells
					++shroudedQueue;
				}
				
				FillExplored(x, y);
			} else {
				// coalese all horizontally adjacent unexplored cells
				++unexploredQueue;
				if (shroudedQueue) {
					FillFog(x - shroudedQueue, y, shroudedQueue, trans);
					shroudedQueue = 0;
				}
			}
		}
		
		if (shroudedQueue) {
			FillFog(x - (shroudedQueue + unexploredQueue), y, shroudedQueue, trans);
		}
		
		if (unexploredQueue) {
			FillFog(x - unexploredQueue, y, unexploredQueue, opaque);
		}
	}
}

void Map::DrawHighlightables(const Region& viewport) const
{
	// NOTE: piles are drawn in the main queue
	unsigned int i = 0;
	Container *c;
	while ((c = TMap->GetContainer(i++)) != NULL) {
		if (c->containerType != IE_CONTAINER_PILE) {
			// don't highlight containers behind closed doors
			// how's ar9103 chest has a Pos outside itself, so we check the bounding box instead
			// FIXME: inefficient, check for overlap in AREImporter and only recheck here if a flag was set
			const Door *door = TMap->GetDoor(c->BBox.Center());
			if (door && !(door->Flags & (DOOR_OPEN|DOOR_TRANSPARENT))) continue;
			if (c->Highlight) {
				c->DrawOutline(viewport.origin);
			} else if (debugFlags & DEBUG_SHOW_CONTAINERS) {
				c->outlineColor = ColorCyan;
				c->DrawOutline(viewport.origin);
			}
		}
	}

	Door *d;
	i = 0;
	while ( (d = TMap->GetDoor(i++))!=NULL ) {
		if (d->Highlight) {
			d->outlineColor = gamedata->GetColor("HOVERDOOR");
			d->DrawOutline(viewport.origin);
		} else if (debugFlags & DEBUG_SHOW_DOORS && !(d->Flags & DOOR_SECRET)) {
			d->outlineColor = gamedata->GetColor("ALTDOOR");
			d->DrawOutline(viewport.origin);
		} else if (debugFlags & DEBUG_SHOW_DOORS_SECRET && d->Flags & DOOR_FOUND) {
			d->outlineColor = ColorMagenta;
			d->DrawOutline(viewport.origin);
		}
	}

	InfoPoint *p;
	i = 0;
	while ( (p = TMap->GetInfoPoint(i++))!=NULL ) {
		if (p->Highlight) {
			p->DrawOutline(viewport.origin);
		} else if (debugFlags & DEBUG_SHOW_INFOPOINTS) {
			if (p->VisibleTrap(true)) {
				p->outlineColor = ColorRed;
			} else {
				p->outlineColor = ColorBlue;
			}
			p->DrawOutline(viewport.origin);
		}
	}
}

Container *Map::GetNextPile(int &index) const
{
	Container *c = TMap->GetContainer(index++);

	while (c) {
		if (c->containerType == IE_CONTAINER_PILE) {
			return c;
		}
		c = TMap->GetContainer(index++);
	}
	return NULL;
}

Actor *Map::GetNextActor(int &q, int &index) const
{
retry:
	switch(q) {
		case PR_SCRIPT:
			if (index--)
				return queue[q][index];
			q--;
			return NULL;
		case PR_DISPLAY:
			if (index--)
				return queue[q][index];
			q--;
			index = Qcount[q];
			goto retry;
		default:
			return NULL;
	}
}

const AreaAnimation *Map::GetNextAreaAnimation(aniIterator &iter, ieDword gametime) const
{
retry:
	if (iter==animations.end()) {
		return NULL;
	}
	const AreaAnimation &a = *(iter++);
	if (!a.Schedule(gametime) ) {
		goto retry;
	}
	if ((a.Flags & A_ANI_NOT_IN_FOG) ? !IsVisible(a.Pos) : !IsExplored(a.Pos)) {
		goto retry;
	}

	return &a;
}

Particles *Map::GetNextSpark(const spaIterator &iter) const
{
	if (iter==particles.end()) {
		return NULL;
	}
	return *iter;
}

//doesn't increase iterator, because we might need to erase it from the list
Projectile *Map::GetNextProjectile(const proIterator &iter) const
{
	if (iter==projectiles.end()) {
		return NULL;
	}
	return *iter;
}

const Projectile *Map::GetNextTrap(proIterator &iter) const
{
	const Projectile *pro;

	do {
		pro=GetNextProjectile(iter);
		if (pro) iter++;
		// find dormant traps (thieves', skull traps, glyphs of warding ...)
		if (pro && pro->GetPhase() == P_TRIGGER) break;
	} while(pro);
	return pro;
}

size_t Map::GetProjectileCount(proIterator &iter) const
{
	iter = projectiles.begin();
	return projectiles.size();
}

int Map::GetTrapCount(proIterator &iter) const
{
	int cnt = 0;
	iter=projectiles.begin();
	while(GetNextTrap(iter)) {
		cnt++;
	}
	//
	iter = projectiles.begin();
	return cnt;
}


//doesn't increase iterator, because we might need to erase it from the list
VEFObject *Map::GetNextScriptedAnimation(const scaIterator &iter) const
{
	if (iter==vvcCells.end()) {
		return NULL;
	}
	return *iter;
}

//Draw the game area (including overlays, actors, animations, weather)
void Map::DrawMap(const Region& viewport, uint32_t dFlags)
{
	assert(TMap);
	debugFlags = dFlags;

	Game *game = core->GetGame();
	ieDword gametime = game->GameTime;
	bool timestop = game->IsTimestopActive();

	//area specific spawn.ini files (a PST feature)
	if (INISpawn) {
		INISpawn->CheckSpawn();
	}

	// Map Drawing Strategy
	// 1. Draw background
	// 2. Draw overlays (weather)
	// 3. Create a stencil set: a WF_COVERANIMS wall stencil and an opaque wall stencil
	// 4. set the video stencil buffer to animWallStencil
	// 5. Draw background animations (BlitFlags::STENCIL_GREEN)
	// 6. set the video stencil buffer to wallStencil
	// 7. draw scriptables (depending on scriptable->ForceDither() return value)
	// 8. draw fog (BlitFlags::BLENDED)
	// 9. draw text (BlitFlags::BLENDED)

	//Blit the Background Map Animations (before actors)
	Video* video = core->GetVideoDriver();
	int bgoverride = false;

	if (Background) {
		if (BgDuration < gametime) {
			Background = nullptr;
		} else {
			video->BlitSprite(Background, Point());
			bgoverride = true;
		}
	}

	if (!bgoverride) {
		int rain = 0;
		BlitFlags flags = BlitFlags::NONE;

		if (timestop) {
			flags = BlitFlags::GREY;
		} else if (AreaFlags&AF_DREAM) {
			flags = BlitFlags::SEPIA;
		}

		if (HasWeather()) {
			//zero when the weather particles are all gone
			rain = game->weather->GetPhase()-P_EMPTY;
		}

		TMap->DrawOverlays( viewport, rain, flags );
	}

	const auto& viewportWalls = WallsIntersectingRegion(viewport, false);
	RedrawScreenStencil(viewport, viewportWalls.first);
	video->SetStencilBuffer(wallStencil);
	
	//draw all background animations first
	aniIterator aniidx = animations.begin();

	auto DrawAreaAnimation = [&, this](const AreaAnimation *a) {
		BlitFlags flags = SetDrawingStencilForAreaAnimation(a, viewport);
		flags |= BlitFlags::COLOR_MOD | BlitFlags::BLENDED;
		
		if (timestop) {
			flags |= BlitFlags::GREY;
		}
		
		Color tint = ColorWhite;
		if (a->Flags & A_ANI_NO_SHADOW) {
			tint = GetLighting(a->Pos);
		}
		
		game->ApplyGlobalTint(tint, flags);

		a->Draw(viewport, tint, flags);
		return GetNextAreaAnimation(aniidx, gametime);
	};
	
	const AreaAnimation *a = GetNextAreaAnimation(aniidx, gametime);
	while (a && a->GetHeight() == ANI_PRI_BACKGROUND) {
		a = DrawAreaAnimation(a);
	}

	if (!bgoverride) {
		//Draw Outlines
		DrawHighlightables(viewport);
	}

	//drawing queues 1 and 0
	//starting with lower priority
	//so displayed, but inactive actors (dead) will be drawn over
	int q = PR_DISPLAY;
	int index = Qcount[q];
	Actor* actor = GetNextActor(q, index);

	scaIterator scaidx = vvcCells.begin();
	proIterator proidx = projectiles.begin();
	spaIterator spaidx = particles.begin();
	int pileidx = 0;
	const Container *pile = GetNextPile(pileidx);

	VEFObject *sca = GetNextScriptedAnimation(scaidx);
	Projectile *pro = GetNextProjectile(proidx);
	Particles *spark = GetNextSpark(spaidx);

	// TODO: In at least HOW/IWD2 actor ground circles will be hidden by
	// an area animation with height > 0 even if the actors themselves are not
	// hidden by it.

	while (actor || a || sca || spark || pro || pile) {
		switch(SelectObject(actor,q,a,sca,spark,pro,pile)) {
		case AOT_ACTOR:
			{
				bool visible = false;
				// always update the animations even if we arent visible
				if (actor->UpdateDrawingState() && IsExplored(actor->Pos)) {
					// apparently birds and the dead are always visible?
					visible = IsVisible(actor->Pos) || (actor->Modified[IE_DONOTJUMP] & DNJ_BIRD) || (actor->GetInternalFlag() & IF_REALLYDIED);
					if (visible) {
						BlitFlags flags = SetDrawingStencilForScriptable(actor, viewport);
						if (game->TimeStoppedFor(actor)) {
							// when time stops, almost everything turns dull grey,
							// the caster and immune actors being the most notable exceptions
							flags |= BlitFlags::GREY;
						}
						
						Color baseTint = area->GetLighting(actor->Pos);
						Color tint(baseTint);
						game->ApplyGlobalTint(tint, flags);
						actor->Draw(viewport, baseTint, tint, flags|BlitFlags::BLENDED);
					}
				}

				if (!visible || (actor->GetInternalFlag() & (IF_REALLYDIED|IF_ACTIVE)) == (IF_REALLYDIED|IF_ACTIVE)) {
					actor->SetInternalFlag(IF_TRIGGER_AP, OP_NAND);
					//turning actor inactive if there is no action next turn
					actor->HibernateIfAble();
				}

				actor = GetNextActor(q, index);
			}
			break;
		case AOT_PILE:
			// draw piles
			if (!bgoverride) {
				const Container* c = TMap->GetContainer(pileidx - 1);
				
				BlitFlags flags = SetDrawingStencilForScriptable(c, viewport);
				flags |= BlitFlags::COLOR_MOD | BlitFlags::BLENDED;
				
				if (timestop) {
					flags |= BlitFlags::GREY;
				}
				
				Color tint = GetLighting(c->Pos);
				game->ApplyGlobalTint(tint, flags);

				if (c->Highlight || (debugFlags & DEBUG_SHOW_CONTAINERS)) {
					c->Draw(true, viewport, tint, flags);
				} else {
					c->Draw(false, viewport, tint, flags);
				}
				pile = GetNextPile(pileidx);
			}
			break;
		case AOT_AREA:
			{
				a = DrawAreaAnimation(a);
			}
			break;
		case AOT_SCRIPTED:
			{
				bool endReached = sca->UpdateDrawingState(-1);
				if (endReached) {
					delete sca;
					scaidx = vvcCells.erase(scaidx);
				} else {
					video->SetStencilBuffer(wallStencil);
					Color tint = GetLighting(sca->Pos);
					tint.a = 255;
					
					// FIXME: these should actually make use of SetDrawingStencilForObject too
					BlitFlags flags = (core->DitherSprites) ? BlitFlags::STENCIL_BLUE : BlitFlags::STENCIL_RED;
					
					if (timestop) {
						flags |= BlitFlags::GREY;
					}

					game->ApplyGlobalTint(tint, flags);

					sca->Draw(viewport, tint, 0, flags);
					scaidx++;
				}
			}
			sca = GetNextScriptedAnimation(scaidx);
			break;
		case AOT_PROJECTILE:
			{
				int drawn;
				if (gametime > oldGameTime) {
					drawn = pro->Update();
				} else {
					drawn = 1;
				}
				if (drawn) {
					pro->Draw( viewport );
					proidx++;
				} else {
					delete pro;
					proidx = projectiles.erase(proidx);
				}
			}
			pro = GetNextProjectile(proidx);
			break;
		case AOT_SPARK:
			{
				int drawn;
				if (gametime > oldGameTime) {
					drawn = spark->Update();
				} else {
					drawn = 1;
				}
				if (drawn) {
					spark->Draw(viewport.origin);
					spaidx++;
				} else {
					delete( spark );
					spaidx=particles.erase(spaidx);
				}
			}
			spark = GetNextSpark(spaidx);
			break;
		default:
			error("Map", "Trying to draw unknown animation type.\n");
		}
	}

	video->SetStencilBuffer(NULL);
	
	bool update_scripts = (core->GetGameControl()->GetDialogueFlags() & DF_FREEZE_SCRIPTS) == 0;
	game->DrawWeather(update_scripts);
	
	if (dFlags & (DEBUG_SHOW_LIGHTMAP | DEBUG_SHOW_HEIGHTMAP | DEBUG_SHOW_MATERIALMAP | DEBUG_SHOW_SEARCHMAP)) {
		DrawDebugOverlay(viewport, dFlags);
	}
	
	const Bitmap* exploredBits = (dFlags & DEBUG_SHOW_FOG_UNEXPLORED) ? nullptr : &ExploredBitmap;
	const Bitmap* visibleBits = (dFlags & DEBUG_SHOW_FOG_INVISIBLE) ? nullptr : &VisibleBitmap;

	DrawFogOfWar(exploredBits, visibleBits, viewport);

	int ipCount = 0;
	while (true) {
		//For each InfoPoint in the map
		InfoPoint* ip = TMap->GetInfoPoint( ipCount++ );
		if (!ip)
			break;
		ip->DrawOverheadText();
	}

	int cnCount = 0;
	while (true) {
		//For each Container in the map
		Container* cn = TMap->GetContainer( cnCount++ );
		if (!cn)
			break;
		cn->DrawOverheadText();
	}

	int drCount = 0;
	while (true) {
		//For each Door in the map
		Door* dr = TMap->GetDoor( drCount++ );
		if (!dr)
			break;
		dr->DrawOverheadText();
	}

	size_t i = actors.size();
	while (i--) {
		//For each Actor present
		//This must go AFTER the fog!
		//(maybe we should be using the queue?)
		Actor* actor = actors[i];
		actor->DrawOverheadText();
	}

	oldGameTime = gametime;

	// Show wallpolygons
	if (debugFlags & (DEBUG_SHOW_WALLS_ALL|DEBUG_SHOW_DOORS_DISABLED)) {
		const auto& viewportWalls = WallsIntersectingRegion(viewport, true);
		for (const auto& poly : viewportWalls.first) {
			const Point& origin = poly->BBox.origin - viewport.origin;

			if (poly->wall_flag&WF_DISABLED) {
				if (debugFlags & DEBUG_SHOW_DOORS_DISABLED) {
					video->DrawPolygon( poly.get(), origin, ColorGray, true, BlitFlags::BLENDED|BlitFlags::HALFTRANS);
				}
				continue;
			}

			if ((debugFlags & (DEBUG_SHOW_WALLS|DEBUG_SHOW_WALLS_ANIM_COVER)) == 0) {
				continue;
			}

			Color c = ColorYellow;

			if (debugFlags & DEBUG_SHOW_WALLS_ANIM_COVER) {
				if (poly->wall_flag & WF_COVERANIMS) {
					// darker yellow for walls with WF_COVERANIMS
					c.r -= 0x80;
					c.g -= 0x80;
				}
			} else if ((debugFlags & DEBUG_SHOW_WALLS) == 0) {
				continue;
			}

			video->DrawPolygon( poly.get(), origin, c, true, BlitFlags::BLENDED|BlitFlags::HALFTRANS);
			
			if (poly->wall_flag & WF_BASELINE) {
				video->DrawLine(poly->base0 - viewport.origin, poly->base1 - viewport.origin, ColorMagenta);
			}
		}
	}
}

WallPolygonSet Map::WallsIntersectingRegion(Region r, bool includeDisabled, const Point* loc) const
{
	// WallGroups are collections that contain a reference to all wall polygons intersecting
	// a 640x480 region moving from top left to bottom right of the map

	constexpr uint32_t groupHeight = 480;
	constexpr uint32_t groupWidth = 640;
	
	if (r.x < 0) {
		r.w += r.x;
		r.x = 0;
	}
	
	if (r.y < 0) {
		r.h += r.y;
		r.y = 0;
	}

	uint32_t pitch = CeilDiv<uint32_t>(TMap->XCellCount * 64, groupWidth);
	uint32_t ymin = r.y / groupHeight;
	uint32_t maxHeight = CeilDiv<uint32_t>(TMap->YCellCount * 64, groupHeight);
	uint32_t ymax = std::min(maxHeight, CeilDiv<uint32_t>(r.y + r.h, groupHeight));
	uint32_t xmin = r.x / groupWidth;
	uint32_t xmax = std::min(pitch, CeilDiv<uint32_t>(r.x + r.w, groupWidth));

	WallPolygonSet set;
	WallPolygonGroup& infront = set.first;
	WallPolygonGroup& behind = set.second;

	for (uint32_t y = ymin; y < ymax; ++y) {
		for (uint32_t x = xmin; x < xmax; ++x) {
			const auto& group = wallGroups[y * pitch + x];
			
			for (const auto& wp : group) {
				if ((wp->wall_flag&WF_DISABLED) && includeDisabled == false) {
					continue;
				}
				
				if (!r.IntersectsRegion(wp->BBox)) {
					continue;
				}
				
				if (loc == nullptr || wp->PointBehind(*loc)) {
					infront.push_back(wp);
				} else {
					behind.push_back(wp);
				}
			}
		}
	}

	return set;
}

void Map::SetDrawingStencilForObject(const void* object, const Region& objectRgn, const WallPolygonSet& walls, const Point& viewPortOrigin)
{
	VideoBufferPtr stencil = nullptr;
	Video* video = core->GetVideoDriver();
	Color debugColor = ColorGray;
	
	const bool behindWall = !walls.first.empty();
	const bool inFrontOfWall = !walls.second.empty();

	if (behindWall && inFrontOfWall) {
		// we need a custom stencil if both behind and in front of a wall
		auto it = objectStencils.find(object);
		if (it != objectStencils.end()) {
			// we already made one
			const auto& pair = it->second;
			if (pair.second.RectInside(objectRgn)) {
				// and it is still good
				stencil = pair.first;
			}
		}
		
		if (stencil == nullptr) {
			Region stencilRgn = Region(objectRgn.origin - viewPortOrigin, objectRgn.size);
			if (stencilRgn.size.IsInvalid()) {
				stencil = wallStencil;
			} else {
				stencil = video->CreateBuffer(stencilRgn, Video::BufferFormat::DISPLAY_ALPHA);
				DrawStencil(stencil, objectRgn, walls.first);
				objectStencils[object] = std::make_pair(stencil, objectRgn);
			}
		} else {
			// TODO: we only need to do this because a door might have changed state over us
			// if we could detect that we could avoid doing this expensive operation
			// we could add another wall flag to mark doors and then we only need to do this if one of the "walls" over us has that flag set
			stencil->Clear();
			stencil->SetOrigin(objectRgn.origin - viewPortOrigin);
			DrawStencil(stencil, objectRgn, walls.first);
		}
		
		debugColor = ColorRed;
	} else {
		stencil = wallStencil;
		
		if (behindWall) {
			debugColor = ColorBlue;
		} else if (inFrontOfWall) {
			debugColor = ColorMagenta;
		}
	}
	
	assert(stencil);
	video->SetStencilBuffer(stencil);
	
	if (debugFlags & DEBUG_SHOW_WALLS) {
		const Region& r = Region(objectRgn.origin - viewPortOrigin, objectRgn.size);
		video->DrawRect(r, debugColor, false);
	}
}

BlitFlags Map::SetDrawingStencilForScriptable(const Scriptable* scriptable, const Region& vp)
{
	if (scriptable->Type == ST_ACTOR) {
		const Actor* actor = static_cast<const Actor*>(scriptable);
		// birds are never occluded
		if (actor->GetStat(IE_DONOTJUMP) & DNJ_BIRD) {
			return BlitFlags::NONE;
		}
	}
	
	const Region& bbox = scriptable->DrawingRegion();
	if (bbox.IntersectsRegion(vp) == false) {
		return BlitFlags::NONE;
	}
	
	WallPolygonSet walls = WallsIntersectingRegion(bbox, false, &scriptable->Pos);
	SetDrawingStencilForObject(scriptable, bbox, walls, vp.origin);
	
	// check this after SetDrawingStencilForObject for debug drawing purposes
	if (walls.first.empty()) {
		return BlitFlags::NONE; // not behind a wall, no stencil required
	}
	
	ieDword always_dither;
	core->GetDictionary()->Lookup("Always Dither", always_dither);
	
	BlitFlags flags = BlitFlags::STENCIL_DITHER; // TODO: make dithering configurable
	if (always_dither) {
		flags |= BlitFlags::STENCIL_ALPHA;
	} else if (core->DitherSprites == false) {
		// dithering is set to disabled
		flags |= BlitFlags::STENCIL_BLUE;
	} else if (scriptable->Type == ST_ACTOR) {
		const Actor* a = static_cast<const Actor*>(scriptable);
		if (a->IsSelected() || a->Over) {
			flags |= BlitFlags::STENCIL_ALPHA;
		} else {
			flags |= BlitFlags::STENCIL_RED;
		}
	} else if (scriptable->Type == ST_CONTAINER) {
		const Container* c = static_cast<const Container*>(scriptable);
		if (c->Highlight) {
			flags |= BlitFlags::STENCIL_ALPHA;
		} else {
			flags |= BlitFlags::STENCIL_RED;
		}
	}
	
	assert(flags & BLIT_STENCIL_MASK); // we needed a stencil so we must require a stencil flag
	return flags;
}

BlitFlags Map::SetDrawingStencilForAreaAnimation(const AreaAnimation* anim, const Region& vp)
{
	const Region& bbox = anim->DrawingRegion();
	if (bbox.IntersectsRegion(vp) == false) {
		return BlitFlags::NONE;
	}
	
	Point p = anim->Pos;
	p.y += anim->height;

	WallPolygonSet walls = WallsIntersectingRegion(bbox, false, &p);
	
	SetDrawingStencilForObject(anim, bbox, walls, vp.origin);
	
	// check this after SetDrawingStencilForObject for debug drawing purposes
	if (walls.first.empty()) {
		return BlitFlags::NONE; // not behind a wall, no stencil required
	}

	return (anim->Flags & A_ANI_NO_WALL) ? BlitFlags::NONE : BlitFlags::STENCIL_GREEN;
}

void Map::DrawDebugOverlay(const Region &vp, uint32_t dFlags) const
{
	const static struct DebugPalettes {
		PaletteHolder searchMapPal;
		PaletteHolder materialMapPal;
		PaletteHolder heightMapPal;
		// lightmap pal is the sprite pal
		
		DebugPalettes() noexcept {
			searchMapPal = MakeHolder<Palette>();
			std::fill(&searchMapPal->col[0], &searchMapPal->col[255], Color()); // passable is transparent
			searchMapPal->col[0] = Color(128, 64, 64, 128); // IMPASSABLE, red-ish
			
			for (uint8_t i = 1; i < 255; ++i) {
				if (i & uint8_t(PathMapFlags::SIDEWALL)) {
					searchMapPal->col[uint8_t(PathMapFlags::SIDEWALL)] = Color(64, 64, 128, 128); // blues-ish
				} else if (i & uint8_t(PathMapFlags::ACTOR)) {
					searchMapPal->col[uint8_t(PathMapFlags::SIDEWALL)] = Color(128, 64, 128, 128); // actor, purple-ish
				} else if ((i & uint8_t(PathMapFlags::PASSABLE)) == 0) {
					// anything else that isnt PASSABLE
					searchMapPal->col[uint8_t(PathMapFlags::SIDEWALL)] = ColorGray;
				}
			}
			
			materialMapPal = MakeHolder<Palette>();
			materialMapPal->col[0] = ColorBlack; // impassable, light blocking
			materialMapPal->col[1] = Color(0xB9, 0xAB, 0x79, 128); // sand
			materialMapPal->col[2] = Color(0x6C, 0x4D, 0x2E, 128); // wood
			materialMapPal->col[3] = Color(0x6C, 0x4D, 0x2E, 128); // wood
			materialMapPal->col[4] = Color(0x84, 0x86, 0x80, 128); // stone
			materialMapPal->col[5] = Color(0, 0xFF, 0, 128); // grass
			materialMapPal->col[6] = ColorBlue; // water
			materialMapPal->col[7] = Color(0x84, 0x86, 0x80, 128); // stone
			materialMapPal->col[8] = ColorWhite; // obstacle, non light blocking
			materialMapPal->col[9] = Color(0x6C, 0x4D, 0x2E, 128); // wood
			materialMapPal->col[10] = ColorGray; // wall, impassable
			materialMapPal->col[11] = ColorBlue; // water
			materialMapPal->col[12] = ColorBlueDark; // water, impassable
			materialMapPal->col[13] = Color(0xFF, 0x00, 0xFF, 128); // roof
			materialMapPal->col[14] = Color(128, 0, 128, 128); // exit
			materialMapPal->col[15] = Color(0, 0xFF, 0, 128); // grass
			
			heightMapPal = MakeHolder<Palette>();
			for (uint8_t i = 0; i < 255; ++i) {
				heightMapPal->col[i] = Color(i, i, i, 128);
			}
		}
	} debugPalettes;
	
	Video *vid=core->GetVideoDriver();
	Region block(0,0,16,12);

	int w = vp.w/16+2;
	int h = vp.h/12+2;
	
	BlitFlags flags = BlitFlags::BLENDED;
	if (dFlags & DEBUG_SHOW_LIGHTMAP) {
		flags |= BlitFlags::HALFTRANS;
	}

	for(int x=0;x<w;x++) {
		for(int y=0;y<h;y++) {
			block.x = x * 16 - (vp.x % 16);
			block.y = y * 12 - (vp.y % 12);
			
			Point p = Point(x, y) + ConvertCoordToTile(vp.origin);

			Color col;
			Color vals = tileProps->GetPixel(p);
			if (dFlags & DEBUG_SHOW_SEARCHMAP) {
				col = debugPalettes.searchMapPal->col[vals.r];
			} else if (dFlags & DEBUG_SHOW_MATERIALMAP) {
				col = debugPalettes.materialMapPal->col[vals.g];
			} else if (dFlags & DEBUG_SHOW_HEIGHTMAP) {
				col = debugPalettes.heightMapPal->col[vals.b];
			} else if (dFlags & DEBUG_SHOW_LIGHTMAP) {
				col = tileProps->GetPalette()->col[vals.a];
			}
			
			vid->DrawRect(block, col, true, flags);
		}
	}
	
	if (dFlags & DEBUG_SHOW_SEARCHMAP) {
		// draw also pathfinding waypoints
		const Actor *act = core->GetFirstSelectedActor();
		if (!act) return;
		const PathNode *path = act->GetPath();
		if (!path) return;
		const PathNode *step = path->Next;
		Color waypoint(0, 64, 128, 128); // darker blue-ish
		int i = 0;
		block.w = 8;
		block.h = 6;
		while (step) {
			block.x = (step->x+64) - vp.x;
			block.y = (step->y+6) - vp.y;
			print("Waypoint %d at (%d, %d)", i, step->x, step->y);
			vid->DrawRect(block, waypoint);
			step = step->Next;
			i++;
		}
	}
}

//adding animation in order, based on its height parameter
void Map::AddAnimation(AreaAnimation anim)
{
	int Height = anim.GetHeight();
	auto iter = animations.begin();
	for (; (iter != animations.end()) && (iter->GetHeight() < Height); ++iter) ;
	animations.insert(iter, std::move(anim));
}

//reapplying all of the effects on the actors of this map
//this might be unnecessary later
void Map::UpdateEffects()
{
	size_t i = actors.size();
	while (i--) {
		actors[i]->RefreshEffects(NULL);
	}
}

void Map::Shout(const Actor* actor, int shoutID, bool global) const
{
	for (auto listener : actors) {
		// skip the shouter, so gpshout's InMyGroup(LastHeardBy(Myself)) can get two distinct actors
		if (listener == actor) {
			continue;
		}

		if (!global) {
			if (!WithinAudibleRange(actor, listener->Pos)) {
				continue;
			}
		}
		if (shoutID) {
			listener->AddTrigger(TriggerEntry(trigger_heard, actor->GetGlobalID(), shoutID));
			listener->LastHeard = actor->GetGlobalID();
		} else {
			listener->AddTrigger(TriggerEntry(trigger_help, actor->GetGlobalID()));
			listener->LastHelp = actor->GetGlobalID();
		}
	}
}

int Map::CountSummons(ieDword flags, ieDword sex) const
{
	int count = 0;

	for (const Actor *actor : actors) {
		if (!actor->ValidTarget(flags) ) {
			continue;
		}
		if (actor->GetStat(IE_SEX)==sex) {
			count++;
		}
	}
	return count;
}

bool Map::AnyEnemyNearPoint(const Point &p) const
{
	ieDword gametime = core->GetGame()->GameTime;
	for (const Actor *actor : actors) {
		if (!actor->Schedule(gametime, true) ) {
			continue;
		}
		if (actor->IsDead() ) {
			continue;
		}
		if (actor->GetStat(IE_AVATARREMOVAL)) {
			continue;
		}
		if (Distance(actor->Pos, p) > SPAWN_RANGE) {
			continue;
		}
		if (actor->GetStat(IE_EA)<=EA_EVILCUTOFF) {
			continue;
		}

		return true;
	}
	return false;
}

void Map::ActorSpottedByPlayer(const Actor *actor) const
{
	unsigned int animid;

	if(core->HasFeature(GF_HAS_BEASTS_INI)) {
		animid=actor->BaseStats[IE_ANIMATION_ID];
		if(core->HasFeature(GF_ONE_BYTE_ANIMID)) {
			animid&=0xff;
		}
		if (animid < (ieDword)CharAnimations::GetAvatarsCount()) {
			const AvatarStruct &avatar = CharAnimations::GetAvatarStruct(animid);
			core->GetGame()->SetBeastKnown(avatar.Bestiary);
		}
	}
}

// Call this for any visible actor.  do_pause can be false if hostile
// actors were already seen on the map.  We used to check AnyPCInCombat,
// which is less reliable.  Returns true if this is a hostile enemy
// that should trigger pause.
bool Map::HandleAutopauseForVisible(Actor *actor, bool doPause) const
{
	if (actor->Modified[IE_EA] > EA_EVILCUTOFF && !(actor->GetInternalFlag() & IF_STOPATTACK)) {
		if (doPause && !(actor->GetInternalFlag() & IF_TRIGGER_AP))
			core->Autopause(AP_ENEMY, actor);
		actor->SetInternalFlag(IF_TRIGGER_AP, OP_OR);
		return true;
	}
	return false;
}

//call this once, after area was loaded
void Map::InitActors()
{
	// setting the map can run effects, so play on the safe side and ignore any actors that might get added
	size_t i = actors.size();
	while (i--) {
		Actor *actor = actors[i];
		actor->SetMap(this);
		// make sure to bump away in case someone or something is already there
		actor->SetPosition(actor->Pos, 1);
		MarkVisited(actor);
	}
}

void Map::MarkVisited(const Actor *actor) const
{
	if (actor->InParty && core->HasFeature(GF_AREA_VISITED_VAR)) {
		char key[32];
		const size_t len = snprintf(key, sizeof(key),"%s_visited", scriptName.CString());
		if (len > sizeof(key)) {
			Log(ERROR, "Map", "Area %s has a too long script name for generating _visited globals!", scriptName.CString());
		}
		core->GetGame()->locals->SetAt(key, 1);
	}
}

void Map::AddActor(Actor* actor, bool init)
{
	//setting the current area for the actor as this one
	actor->Area = MakeLowerCaseResRef(scriptName);
	if (!HasActor(actor)) {
		actors.push_back( actor );
	}
	if (init) {
		actor->SetMap(this);
		MarkVisited(actor);
	}
}

bool Map::AnyPCSeesEnemy() const
{
	ieDword gametime = core->GetGame()->GameTime;
	for (const Actor *actor : actors) {
		if (actor->Modified[IE_EA]>=EA_EVILCUTOFF) {
			if (IsVisible(actor->Pos) && actor->Schedule(gametime, true) ) {
				return true;
			}
		}
	}
	return false;
}

//Make an actor gone for (almost) good
//If the actor was in the party, it will be moved to the npc storage
//If the actor is in the NPC storage, its area and some other fields
//that are needed for proper reentry will be zeroed out
//If the actor isn't in the NPC storage, it is destructed
void Map::DeleteActor(int i)
{
	Actor *actor = actors[i];
	if (actor) {
		Game *game = core->GetGame();
		//this makes sure that a PC will be demoted to NPC
		game->LeaveParty( actor );
		//this frees up the spot under the feet circle
		ClearSearchMapFor( actor );
		//remove the area reference from the actor
		actor->SetMap(NULL);
		actor->Area.Reset();
		objectStencils.erase(actor);
		//don't destroy the object in case it is a persistent object
		//otherwise there is a dead reference causing a crash on save
		if (game->InStore(actor) < 0) {
			delete actor;
		}
	}
	//remove the actor from the area's actor list
	actors.erase( actors.begin()+i );
}

Scriptable *Map::GetScriptableByGlobalID(ieDword objectID)
{
	if (!objectID) return NULL;
	
	Scriptable *scr = GetActorByGlobalID(objectID);
	if (scr)
		return scr;

	scr = GetInfoPointByGlobalID(objectID);
	if (scr)
		return scr;

	scr = GetContainerByGlobalID(objectID);
	if (scr)
		return scr;

	scr = GetDoorByGlobalID(objectID);
	if (scr)
		return scr;

	if (GetGlobalID() == objectID)
		scr = this;

	return scr;
}

Door *Map::GetDoorByGlobalID(ieDword objectID) const
{
	if (!objectID) return NULL;

	int doorCount = 0;
	while (true) {
		Door* door = TMap->GetDoor( doorCount++ );
		if (!door)
			return NULL;
		if (door->GetGlobalID() == objectID)
			return door;
	}
}

Container *Map::GetContainerByGlobalID(ieDword objectID) const
{
	if (!objectID) return NULL;

	int containerCount = 0;
	while (true) {
		Container* container = TMap->GetContainer( containerCount++ );
		if (!container)
			return NULL;
		if (container->GetGlobalID() == objectID)
			return container;
	}
}

InfoPoint *Map::GetInfoPointByGlobalID(ieDword objectID) const
{
	if (!objectID) return NULL;

	int ipCount = 0;
	while (true) {
		InfoPoint* ip = TMap->GetInfoPoint( ipCount++ );
		if (!ip)
			return NULL;
		if (ip->GetGlobalID() == objectID)
			return ip;
	}
}

Actor* Map::GetActorByGlobalID(ieDword objectID) const
{
	if (!objectID) {
		return nullptr;
	}
	for (const auto& actor : actors) {
		if (actor->GetGlobalID()==objectID) {
			return actor;
		}
	}
	return nullptr;
}

/** flags:
 GA_SELECT    16  - unselectable actors don't play
 GA_NO_DEAD   32  - dead actors don't play
 GA_POINT     64  - not actor specific
 GA_NO_HIDDEN 128 - hidden actors don't play
*/
Actor* Map::GetActor(const Point &p, int flags, const Movable *checker) const
{
	for (auto actor : actors) {
		if (!actor->IsOver( p ))
			continue;
		if (!actor->ValidTarget(flags, checker) ) {
			continue;
		}
		return actor;
	}
	return NULL;
}

Actor* Map::GetActorInRadius(const Point &p, int flags, unsigned int radius) const
{
	for (auto actor : actors) {
		if (PersonalDistance( p, actor ) > radius)
			continue;
		if (!actor->ValidTarget(flags) ) {
			continue;
		}
		return actor;
	}
	return NULL;
}

std::vector<Actor *> Map::GetAllActorsInRadius(const Point &p, int flags, unsigned int radius, const Scriptable *see) const
{
	std::vector<Actor *> neighbours;
	for (auto actor : actors) {
		if (!WithinRange(actor, p, radius)) {
			continue;
		}
		if (!actor->ValidTarget(flags, see) ) {
			continue;
		}
		if (!(flags&GA_NO_LOS)) {
			//line of sight visibility
			if (!IsVisibleLOS(actor->Pos, p)) {
				continue;
			}
		}
		neighbours.emplace_back(actor);
	}
	return neighbours;
}


Actor* Map::GetActor(const char* Name, int flags) const
{
	for (auto actor : actors) {
		if (strnicmp( actor->GetScriptName(), Name, 32 ) == 0) {
			// there can be more with the same scripting name, see bg2/ar0014.baf
			if (!actor->ValidTarget(flags) ) {
				continue;
			}
			return actor;
		}
	}
	return NULL;
}

int Map::GetActorCount(bool any) const
{
	if (any) {
		return (int) actors.size();
	}
	int ret = 0;
	for (const Actor *actor : actors) {
		if (MustSave(actor)) {
			ret++;
		}
	}
	return ret;
}

void Map::JumpActors(bool jump)
{
	for (auto actor : actors) {
		if (actor->Modified[IE_DONOTJUMP]&DNJ_JUMP) {
			if (jump && !(actor->GetStat(IE_DONOTJUMP) & DNJ_BIRD)) {
				ClearSearchMapFor(actor);
				AdjustPositionNavmap(actor->Pos);
				actor->ImpedeBumping();
			}
			actor->SetBase(IE_DONOTJUMP,0);
		}
	}
}

void Map::SelectActors() const
{
	for (auto actor : actors) {
		if (actor->Modified[IE_EA]<EA_CONTROLLABLE) {
			core->GetGame()->SelectActor(actor, true, SELECT_QUIET);
		}
	}
}

//before writing the area out, perform some cleanups
void Map::PurgeArea(bool items)
{
	InternalFlags |= IF_JUSTDIED; //area marked for swapping out

	//1. remove dead actors without 'keep corpse' flag
	int i=(int) actors.size();
	while (i--) {
		Actor *ac = actors[i];
		//we're going to drop the map from memory so clear the reference
		ac->SetMap(NULL);

		if (ac->Modified[IE_STATE_ID]&STATE_NOSAVE) {
			if (ac->Modified[IE_MC_FLAGS] & MC_KEEP_CORPSE) {
				continue;
			}

			if (ac->RemovalTime > core->GetGame()->GameTime) {
				continue;
			}

			//don't delete persistent actors
			if (ac->Persistent()) {
				continue;
			}
			//even if you delete it, be very careful!
			DeleteActor (i);
		}
	}
	//2. remove any non critical items
	if (items) {
		i=(int) TMap->GetContainerCount();
		while (i--) {
			Container *c = TMap->GetContainer(i);
			unsigned int j=c->inventory.GetSlotCount();
			while (j--) {
				const CREItem *itemslot = c->inventory.GetSlotItem(j);
				if (itemslot->Flags&IE_INV_ITEM_CRITICAL) {
					continue;
				}
			}
			TMap->CleanupContainer(c);
			objectStencils.erase(c);
		}
	}
	// 3. reset living neutral actors to their HomeLocation,
	// in case they RandomWalked/flew themselves into a "corner" (mirroring original behaviour)
	for (Actor *actor : actors) {
		if (!actor->GetRandomWalkCounter()) continue;
		if (actor->GetStat(IE_MC_FLAGS) & MC_IGNORE_RETURN) continue;
		if (!actor->ValidTarget(GA_NO_DEAD|GA_NO_UNSCHEDULED|GA_NO_ALLY|GA_NO_ENEMY)) continue;
		if (!actor->HomeLocation.IsZero() && !actor->HomeLocation.IsInvalid() && actor->Pos != actor->HomeLocation) {
			actor->Pos = actor->HomeLocation;
		}
	}
}

Actor* Map::GetActor(int index, bool any) const
{
	if (any) {
		return actors[index];
	}
	unsigned int i=0;
	while (i<actors.size() ) {
		Actor *ac = actors[i++];
		if (MustSave(ac) ) {
			if (!index--) {
				return ac;
			}
		}
	}
	return NULL;
}

Scriptable *Map::GetScriptableByDialog(const ResRef &resref) const
{
	for (auto actor : actors) {
		//if a busy or hostile actor shouldn't be found
		//set this to GD_CHECK
		if (actor->GetDialog(GD_NORMAL) == resref) {
			return actor;
		}
	}

	if (!core->HasFeature(GF_INFOPOINT_DIALOGS)) {
		return NULL;
	}

	// pst has plenty of talking infopoints, eg. in ar0508 (Lothar's cabinet)
	size_t i = TMap->GetInfoPointCount();
	while (i--) {
		InfoPoint* ip = TMap->GetInfoPoint(i);
		if (ip->GetDialog() == resref) {
			return ip;
		}
	}

	// move higher if someone needs talking doors
	i = TMap->GetDoorCount();
	while (i--) {
		Door* door = TMap->GetDoor(i);
		if (door->GetDialog() == resref) {
			return door;
		}
	}
	return NULL;
}

// NOTE: this function is not as general as it sounds
// currently only looks at the party, since it is enough for the only known user
// relies on an override item we create, with the resref matching the dialog one!
// currently only handles dmhead, since no other users have been found yet (to avoid checking whole inventory)
Actor *Map::GetItemByDialog(const ResRef &resref) const
{
	const Game *game = core->GetGame();
	// choose the owner of the dialog via passed dialog ref
	if (resref != ResRef("dmhead")) {
		Log(WARNING, "Map", "Encountered new candidate item for GetItemByDialog? %s", resref.CString());
		return NULL;
	}
	ResRef itemref = "mertwyn";

	int i = game->GetPartySize(true);
	while (i--) {
		const Actor *pc = game->GetPC(i, true);
		int slot = pc->inventory.FindItem(itemref, 0);
		if (slot == -1) continue;
		const CREItem *citem = pc->inventory.GetSlotItem(slot);
		if (!citem) continue;
		const Item *item = gamedata->GetItem(citem->ItemResRef);
		if (!item) continue;
		if (item->Dialog != resref) continue;

		// finally, spawn (dmhead.cre) from our override as a substitute talker
		// the cre file is set up to be invisible, invincible and immune to several things
		Actor *surrogate = gamedata->GetCreature(resref);
		if (!surrogate) {
			error("Map", "GetItemByDialog found the right item, but creature is missing: %s!", resref.CString());
			// error is fatal
		}
		Map *map = pc->GetCurrentArea();
		map->AddActor(surrogate, true);
		surrogate->SetPosition(pc->Pos, 0);

		return surrogate;
	}
	return nullptr;
}

//this function finds an actor by its original resref (not correct yet)
Actor *Map::GetActorByResource(const char *resref) const
{
	for (auto actor : actors) {
		if (strnicmp( actor->GetScriptName(), resref, 8 ) == 0) { //temporarily!
			return actor;
		}
	}
	return NULL;
}

Actor *Map::GetActorByScriptName(const char *name) const
{
	for (auto actor : actors) {
		if (strnicmp( actor->GetScriptName(), name, 8 ) == 0) {
			return actor;
		}
	}
	return NULL;
}

int Map::GetActorsInRect(Actor**& actorlist, const Region& rgn, int excludeFlags) const
{
	actorlist = ( Actor * * ) malloc( actors.size() * sizeof( Actor * ) );
	int count = 0;
	for (auto actor : actors) {
		if (!actor->ValidTarget(excludeFlags))
			continue;
		if (!rgn.PointInside(actor->Pos)
			&& !actor->IsOver(rgn.origin)) // imagine drawing a tiny box inside the circle, but not over the center
			continue;

		actorlist[count++] = actor;
	}
	if (count) {
		actorlist = (Actor **) realloc(actorlist, count * sizeof(Actor *));
	}
	return count;
}

bool Map::SpawnsAlive() const
{
	for (auto actor : actors) {
		if (!actor->ValidTarget(GA_NO_DEAD|GA_NO_UNSCHEDULED))
			continue;
		if (actor->Spawned) {
			return true;
		}
	}
	return false;
}

void Map::PlayAreaSong(int SongType, bool restart, bool hard) const
{
	//Ok, we use a non constant pointer here, so it is easy to disable
	//a faulty music list on the fly. I don't want to add a method just for that
	//crap when we already have that pointer at hand!
	char* poi = core->GetMusicPlaylist( SongHeader.SongList[SongType] );
	// for subareas fall back to the main list
	// needed eg. in bg1 ar2607 (intro candlekeep ambush south)
	// it's not the correct music, perhaps it needs the one from the master area
	// it would match for ar2607 and ar2600, but very annoying (see GetMasterArea)
	if (!poi && !MasterArea && SongType == SONG_BATTLE) {
		poi = core->GetMusicPlaylist(SongType);
	}
	if (!poi) return;

	//check if restart needed (either forced or the current song is different)
	if (!restart && core->GetMusicMgr()->CurrentPlayList(poi)) return;
	int ret = core->GetMusicMgr()->SwitchPlayList( poi, hard );
	//Here we disable the faulty musiclist entry
	if (ret) {
		//Apparently, the playlist manager prefers a *
		*poi='*';
		return;
	}
	if (SongType == SONG_BATTLE) {
		core->GetGame()->CombatCounter = 150;
	}
}

int Map::GetHeight(const Point &p) const
{
	Point tilePos = Map::ConvertCoordToTile(p);
	if (PropsSize().PointInside(tilePos)) {
		// Heightmaps are greyscale images where the top of the world is white and the bottom is black.
		// this covers the range -7 – +7
		// since the image is grey we can use any channel for the mapping
		int val = tileProps->GetPixel(tilePos).b;
		constexpr uint8_t input_range = 255;
		constexpr uint8_t output_range = 14;

		return val * output_range / input_range - 7;
	}
	return 0;
}

Color Map::GetLighting(const Point &p) const
{
	Point tilePos = Map::ConvertCoordToTile(p);
	if (PropsSize().PointInside(tilePos)) {
		uint8_t val = tileProps->GetPixel(tilePos).a;
		return tileProps->GetPalette()->col[val];
	}
	return Color();
}

PathMapFlags Map::QuerySearchMap(const Point &p) const
{
	// p is already in search map coords
	if (PropsSize().PointInside(p)) {
		return PathMapFlags(tileProps->GetPixel(p).r);
	}
	return PathMapFlags::IMPASSABLE;
}

// a more thorough, but more expensive version for the cases when it matters
PathMapFlags Map::GetBlocked(const Point &p, int size) const
{
	if (size == -1) {
		return GetBlocked(p);
	} else {
		return GetBlockedInRadius(ConvertCoordFromTile(p), size);
	}
}

PathMapFlags Map::GetBlockedNavmap(const Point &c) const
{
	return GetBlocked(ConvertCoordToTile(c));
}

// Args are in searchmap coordinates
// The default behavior is for actors to be blocking
// If they shouldn't be, the caller should check for PathMapFlags::PASSABLE | PathMapFlags::ACTOR
PathMapFlags Map::GetBlocked(const Point &p) const
{
	PathMapFlags ret = QuerySearchMap(p);
	if (bool(ret & (PathMapFlags::DOOR_IMPASSABLE|PathMapFlags::ACTOR))) {
		ret &= ~PathMapFlags::PASSABLE;
	}
	if (bool(ret & PathMapFlags::DOOR_OPAQUE)) {
		ret = PathMapFlags::SIDEWALL;
	}
	return ret;
}

// Args are in navmap coordinates
PathMapFlags Map::GetBlockedInRadius(const Point &p, unsigned int size, bool stopOnImpassable) const
{
	// We check a circle of radius size-2 around (px,py)
	// Note that this does not exactly match BG2. BG2's approximations of
	// these circles are slightly different for sizes 7 and up.

	if (size > MAX_CIRCLESIZE) size = MAX_CIRCLESIZE;
	if (size < 2) size = 2;
	PathMapFlags ret = PathMapFlags::IMPASSABLE;

	unsigned int r = (size - 2) * (size - 2) + 1;
	if (size == 2) r = 0;
	for (unsigned int i = 0; i < size - 1; i++) {
		for (unsigned int j = 0; j < size - 1; j++) {
			if (i * i + j * j <= r) {
				PathMapFlags retBotRight = GetBlockedNavmap(Point(p.x + i * 16, p.y + j * 12));
				PathMapFlags retTopRight = GetBlockedNavmap(Point(p.x + i * 16, p.y - j * 12));
				PathMapFlags retBotLeft = GetBlockedNavmap(Point(p.x - i * 16, p.y + j * 12));
				PathMapFlags retTopLeft = GetBlockedNavmap(Point(p.x - i * 16, p.y - j * 12));
				if (stopOnImpassable) {
					if (retBotRight == PathMapFlags::IMPASSABLE || retBotLeft == PathMapFlags::IMPASSABLE || retTopRight == PathMapFlags::IMPASSABLE || retTopLeft == PathMapFlags::IMPASSABLE) {
						return PathMapFlags::IMPASSABLE;
					}
				}
				ret |= (retBotRight | retTopRight | retBotLeft | retTopLeft);
			}
		}
	}
	if (bool(ret & (PathMapFlags::DOOR_IMPASSABLE|PathMapFlags::ACTOR|PathMapFlags::SIDEWALL))) {
		ret &= ~PathMapFlags::PASSABLE;
	}
	if (bool(ret & PathMapFlags::DOOR_OPAQUE)) {
		ret = PathMapFlags::SIDEWALL;
	}

	return ret;
}

PathMapFlags Map::GetBlockedInLine(const Point &s, const Point &d, bool stopOnImpassable, const Actor *caller) const
{
	PathMapFlags ret = PathMapFlags::IMPASSABLE;
	Point p = s;
	while (p != d) {
		double dx = d.x - p.x;
		double dy = d.y - p.y;
		double factor = caller && caller->GetSpeed() ? double(gamedata->GetStepTime()) / double(caller->GetSpeed()) : 1;
		NormalizeDeltas(dx, dy, factor);
		p.x += dx;
		p.y += dy;
		PathMapFlags blockStatus = GetBlockedNavmap(p);
		if (stopOnImpassable && blockStatus == PathMapFlags::IMPASSABLE) {
			return PathMapFlags::IMPASSABLE;
		}
		ret |= blockStatus;
	}
	if (bool(ret & (PathMapFlags::DOOR_IMPASSABLE|PathMapFlags::ACTOR|PathMapFlags::SIDEWALL))) {
		ret &= ~PathMapFlags::PASSABLE;
	}
	if (bool(ret & PathMapFlags::DOOR_OPAQUE)) {
		ret = PathMapFlags::SIDEWALL;
	}

	return ret;
}

// PathMapFlags::SIDEWALL obstructs LOS, while PathMapFlags::IMPASSABLE doesn't
bool Map::IsVisibleLOS(const Point &s, const Point &d, const Actor *caller) const
{
	PathMapFlags ret = GetBlockedInLine(s, d, false, caller);
	return !bool(ret & PathMapFlags::SIDEWALL);
}

// Used by the pathfinder, so PathMapFlags::IMPASSABLE obstructs walkability
bool Map::IsWalkableTo(const Point &s, const Point &d, bool actorsAreBlocking, const Actor *caller) const
{
	PathMapFlags ret = GetBlockedInLine(s, d, true, caller);
	PathMapFlags mask = PathMapFlags::PASSABLE | PathMapFlags::TRAVEL | (actorsAreBlocking ? PathMapFlags::UNMARKED : PathMapFlags::ACTOR);
	return bool(ret & mask);
}

void Map::RedrawScreenStencil(const Region& vp, const WallPolygonGroup& walls)
{
	// FIXME: how do we know if a door changed state?
	// we need to redraw the stencil when that happens
	// see TODO in Map::SetDrawingStencilForScriptable for another example of something that could use this

	if (stencilViewport == vp) {
		assert(wallStencil);
		return;
	}

	stencilViewport = vp;

	if (wallStencil == NULL) {
		// FIXME: this should be forced 8bit*4 color format
		// but currently that is forcing some performance killing conversion issues on some platforms
		// for now things will break if we use 16 bit color settings
		Video* video = core->GetVideoDriver();
		wallStencil = video->CreateBuffer(Region(Point(), vp.size), Video::BufferFormat::DISPLAY_ALPHA);
	}

	wallStencil->Clear();

	DrawStencil(wallStencil, vp, walls);
}

void Map::DrawStencil(const VideoBufferPtr& stencilBuffer, const Region& vp, const WallPolygonGroup& walls) const
{
	Video* video = core->GetVideoDriver();

	// color is used as follows:
	// the 'r' channel is for the native value for all walls
	// the 'g' channel is for the native value for only WF_COVERANIMS walls
	// the 'b' channel is for always opaque (always 0xff, 100% opaque)
	// the 'a' channel is for always dithered (always 0x80, 50% transparent)
	// IMPORTANT: 'a' channel must be always dithered because the "raw" SDL2 driver can only do one stencil and it must be 'a'
	Color stencilcol(0, 0, 0xff, 0x80);
	video->PushDrawingBuffer(stencilBuffer);

	for (const auto& wp : walls) {
		const Point& origin = wp->BBox.origin - vp.origin;

		if (wp->wall_flag & WF_DITHER) {
			stencilcol.r = 0x80;
		} else {
			stencilcol.r = 0xff;
		}

		if (wp->wall_flag & WF_COVERANIMS) {
			stencilcol.g = stencilcol.r;
		} else {
			stencilcol.g = 0;
		}

		video->DrawPolygon(wp.get(), origin, stencilcol, true);
	}

	video->PopDrawingBuffer();
}

bool Map::BehindWall(const Point& pos, const Region& r) const
{
	const auto& polys = WallsIntersectingRegion(r, false, &pos);
	return !polys.first.empty();
}

//this function determines actor drawing order
//it should be extended to wallgroups, animations, effects!
void Map::GenerateQueues()
{
	int priority;

	unsigned int i=(unsigned int) actors.size();
	for (priority=0;priority<QUEUE_COUNT;priority++) {
		if (lastActorCount[priority] != i) {
			if (queue[priority]) {
				free(queue[priority]);
				queue[priority] = NULL;
			}
			queue[priority] = (Actor **) calloc( i, sizeof(Actor *) );
			lastActorCount[priority] = i;
		}
		Qcount[priority] = 0;
	}

	ieDword gametime = core->GetGame()->GameTime;
	bool hostiles_new = false;
	while (i--) {
		Actor* actor = actors[i];

		if (actor->CheckOnDeath()) {
			DeleteActor( i );
			continue;
		}

		ieDword stance = actor->GetStance();
		ieDword internalFlag = actor->GetInternalFlag();

		if (internalFlag&IF_ACTIVE) {
			if ((stance == IE_ANI_TWITCH) && (internalFlag&IF_IDLE) ) {
				priority = PR_DISPLAY; //display
			} else {
				//if actor is unscheduled, don't run its scripts
				if (actor->Schedule(gametime, false) ) {
					priority = PR_SCRIPT; //run scripts and display
				} else {
					priority = PR_IGNORE; //don't run scripts for out of schedule actors
				}
			}
			if (IsVisible(actor->Pos))
				hostiles_new |= HandleAutopauseForVisible(actor, !hostiles_visible);
		} else {
			//dead actors are always visible on the map, but run no scripts
			if ((stance == IE_ANI_TWITCH) || (stance == IE_ANI_DIE) ) {
				priority = PR_DISPLAY;
			} else {
				//isvisible flag is false (visibilitymap) here,
				//coz we want to reactivate creatures that
				//just became visible
				if (IsVisible(actor->Pos) && actor->Schedule(gametime, false) ) {
					priority = PR_SCRIPT; //run scripts and display, activated now
					//more like activate!
					actor->Activate();
					ActorSpottedByPlayer(actor);
					hostiles_new |= HandleAutopauseForVisible(actor, !hostiles_visible);
				} else {
					priority = PR_IGNORE;
				}
			}
		}

		//we ignore priority 2
		if (priority>=PR_IGNORE) continue;

		queue[priority][Qcount[priority]] = actor;
		Qcount[priority]++;
	}
	hostiles_visible = hostiles_new;
}

//the original qsort implementation was flawed
void Map::SortQueues() const
{
	for (int q=0;q<QUEUE_COUNT;q++) {
		Actor **baseline=queue[q];
		int n = Qcount[q];
		int i = n/2;
		int parent, child;
		Actor * tmp;

		for (;;) {
			if (i>0) {
				i--;
				tmp = baseline[i];
			} else {
				n--;
				if (n<=0) break; //breaking loop
				tmp = baseline[n];
				baseline[n] = baseline[0];
			}
			parent = i;
			child = i*2+1;
			while(child<n) {
				int chp = child+1;
				if (chp<n && baseline[chp]->Pos.y < baseline[child]->Pos.y) {
					child=chp;
				}
				if (baseline[child]->Pos.y<tmp->Pos.y) {
					baseline[parent] = baseline[child];
					parent = child;
					child = parent*2+1;
				} else
					break;
			}
			baseline[parent]=tmp;
		}
	}
}

void Map::AddProjectile(Projectile *pro, const Point &source, ieDword actorID, bool fake)
{
	proIterator iter;

	pro->MoveTo(this,source);
	pro->SetTarget(actorID, fake);
	int height = pro->GetHeight();
	for(iter=projectiles.begin();iter!=projectiles.end() && (*iter)->GetHeight()<height; iter++) ;
	projectiles.insert(iter, pro);
}

//adding projectile in order, based on its height parameter
void Map::AddProjectile(Projectile* pro, const Point &source, const Point &dest)
{
	proIterator iter;

	pro->MoveTo(this,source);
	pro->SetTarget(dest);
	int height = pro->GetHeight();
	for(iter=projectiles.begin();iter!=projectiles.end() && (*iter)->GetHeight()<height; iter++) ;
	projectiles.insert(iter, pro);
}

//returns the longest duration of the VVC cell named 'resource' (if it exists)
//if P is empty, the position won't be checked
ieDword Map::HasVVCCell(const ResRef &resource, const Point &p) const
{
	ieDword ret = 0;

	for (const VEFObject *vvc: vvcCells) {
		if (!p.IsInvalid()) {
			if (vvc->Pos.x != p.x) continue;
			if (vvc->Pos.y != p.y) continue;
		}
		if (resource != vvc->ResName) continue;
		const ScriptedAnimation *sca = vvc->GetSingleObject();
		if (sca) {
			ieDword tmp = sca->GetSequenceDuration(AI_UPDATE_TIME)-sca->GetCurrentFrame();
			if (tmp>ret) {
				ret = tmp;
			}
		} else {
			ret = 1;
		}
	}
	return ret;
}

//adding videocell in order, based on its height parameter
void Map::AddVVCell(VEFObject* vvc)
{
	scaIterator iter;

	for(iter=vvcCells.begin();iter!=vvcCells.end() && (*iter)->Pos.y < vvc->Pos.y; iter++) ;
	vvcCells.insert(iter, vvc);
}

AreaAnimation *Map::GetAnimation(const char *Name)
{
	for (auto& anim : animations) {
		if (anim.Name[0] && (strnicmp(anim.Name, Name, 32) == 0)) {
			return &anim;
		}
	}
	return nullptr;
}

Spawn *Map::AddSpawn(const char* Name, const Point &p, std::vector<ResRef>&& creatures)
{
	Spawn* sp = new Spawn();
	strnspccpy(sp->Name, Name, 32);
	
	sp->Pos = p;
	sp->Creatures = std::move(creatures);
	spawns.push_back( sp );
	return sp;
}

void Map::AddEntrance(const char* Name, const Point &p, short Face)
{
	Entrance* ent = new Entrance();
	strlcpy( ent->Name, Name, sizeof(ent->Name) );
	ent->Pos = p;
	ent->Face = (ieWord) Face;
	entrances.push_back( ent );
}

Entrance *Map::GetEntrance(const char *Name) const
{
	for (auto entrance : entrances) {
		if (strnicmp(entrance->Name, Name, 32) == 0) {
			return entrance;
		}
	}
	return NULL;
}

bool Map::HasActor(const Actor *actor) const
{
	for (const Actor *act : actors) {
		if (act == actor) {
			return true;
		}
	}
	return false;
}

void Map::RemoveActor(Actor* actor)
{
	size_t i=actors.size();
	while (i--) {
		if (actors[i] == actor) {
			//path is invalid outside this area, but actions may be valid
			actor->ClearPath(true);
			ClearSearchMapFor(actor);
			actor->SetMap(NULL);
			actor->Area.Reset();
			actors.erase( actors.begin()+i );
			return;
		}
	}
	Log(WARNING, "Map", "RemoveActor: actor not found?");
}

//returns true if none of the partymembers are on the map
//and noone is trying to follow the party out
bool Map::CanFree()
{
	for (auto actor : actors) {
		if (actor->IsPartyMember()) {
			return false;
		}

		if (actor->GetInternalFlag()&IF_USEEXIT) {
			return false;
		}

		const Action *current = actor->GetCurrentAction();
		if (current && actionflags[current->actionID] & AF_CHASE) {
			return false;
		}
	}
	//we expect the area to be swapped out, so we simply remove the corpses now
	PurgeArea(false);
	return true;
}

void Map::dump(bool show_actors) const
{
	StringBuffer buffer;
	buffer.appendFormatted( "Debugdump of Area %s:\n", scriptName.CString());
	buffer.append("Scripts:");

	for (const auto script : Scripts) {
		ResRef poi = "<none>";
		if (script) {
			poi = script->GetName();
		}
		buffer.appendFormatted(" %.8s", poi.CString());
	}
	buffer.append("\n");
	buffer.appendFormatted( "Area Global ID:  %d\n", GetGlobalID());
	buffer.appendFormatted( "OutDoor: %s\n", YESNO(AreaType & AT_OUTDOOR ) );
	buffer.appendFormatted( "Day/Night: %s\n", YESNO(AreaType & AT_DAYNIGHT ) );
	buffer.appendFormatted( "Extended night: %s\n", YESNO(AreaType & AT_EXTENDED_NIGHT ) );
	buffer.appendFormatted( "Weather: %s\n", YESNO(AreaType & AT_WEATHER ) );
	buffer.appendFormatted( "Area Type: %d\n", AreaType & (AT_CITY|AT_FOREST|AT_DUNGEON) );
	buffer.appendFormatted("Can rest: %s\n", YESNO(!core->GetGame()->CanPartyRest(REST_AREA)));

	if (show_actors) {
		buffer.append("\n");
		for (const auto actor : actors) {
			if (actor->ValidTarget(GA_NO_DEAD|GA_NO_UNSCHEDULED)) {
				buffer.appendFormatted("Actor: %s (%d %s) at %d.%d\n", actor->GetName(1), actor->GetGlobalID(), actor->GetScriptName(), actor->Pos.x, actor->Pos.y);
			}
		}
	}
	Log(DEBUG, "Map", buffer);
}

bool Map::AdjustPositionX(Point &goal, int radiusx, int radiusy, int size) const
{
	int minx = 0;
	if (goal.x > radiusx)
		minx = goal.x - radiusx;
	int maxx = goal.x + radiusx + 1;
	
	const Size& mapSize = PropsSize();
	
	if (maxx > mapSize.w)
		maxx = mapSize.w;

	for (int scanx = minx; scanx < maxx; scanx++) {
		if (goal.y >= radiusy) {
			if (bool(GetBlocked(Point(scanx, goal.y - radiusy), size) & PathMapFlags::PASSABLE)) {
				goal.x = scanx;
				goal.y = goal.y - radiusy;
				return true;
			}
		}
		if (goal.y + radiusy < mapSize.h) {
			if (bool(GetBlocked(Point(scanx, goal.y + radiusy), size) & PathMapFlags::PASSABLE)) {
				goal.x = scanx;
				goal.y = goal.y + radiusy;
				return true;
			}
		}
	}
	return false;
}

bool Map::AdjustPositionY(Point &goal, int radiusx, int radiusy, int size) const
{
	int miny = 0;
	if (goal.y > radiusy)
		miny = goal.y - radiusy;
	int maxy = goal.y + radiusy + 1;
	
	const Size& mapSize = PropsSize();
	if (maxy > mapSize.h)
		maxy = mapSize.h;
	for (int scany = miny; scany < maxy; scany++) {
		if (goal.x >= radiusx) {
			if (bool(GetBlocked(Point(goal.x - radiusx, scany), size) & PathMapFlags::PASSABLE)) {
				goal.x = goal.x - radiusx;
				goal.y = scany;
				return true;
			}
		}
		if (goal.x + radiusx < mapSize.w) {
			if (bool(GetBlocked(Point(goal.x + radiusx, scany), size) & PathMapFlags::PASSABLE)) {
				goal.x = goal.x + radiusx;
				goal.y = scany;
				return true;
			}
		}
	}
	return false;
}

void Map::AdjustPositionNavmap(NavmapPoint &goal, int radiusx, int radiusy) const
{
	NavmapPoint smptGoal(goal.x / 16, goal.y / 12);
	AdjustPosition(smptGoal, radiusx, radiusy);
	goal.x = smptGoal.x * 16 + 8;
	goal.y = smptGoal.y * 12 + 6;
}

void Map::AdjustPosition(SearchmapPoint &goal, int radiusx, int radiusy, int size) const
{
	const Size& mapSize = PropsSize();

	if (goal.x > mapSize.w) {
		goal.x = mapSize.w;
	}
	if (goal.y > mapSize.h) {
		goal.y = mapSize.h;
	}

	while(radiusx < mapSize.w || radiusy < mapSize.h) {
		//lets make it slightly random where the actor will appear
		if (RAND(0,1)) {
			if (AdjustPositionX(goal, radiusx, radiusy, size)) {
				return;
			}
			if (AdjustPositionY(goal, radiusx, radiusy, size)) {
				return;
			}
		} else {
			if (AdjustPositionY(goal, radiusx, radiusy, size)) {
				return;
			}
			if (AdjustPositionX(goal, radiusx, radiusy, size)) {
				return;
			}
		}
		if (radiusx < mapSize.w) {
			radiusx++;
		}
		if (radiusy < mapSize.h) {
			radiusy++;
		}
	}
}

Point Map::ConvertPointToFog(const Point &p) const
{
	return Point(p.x / 32, p.y / 32);
}

bool Map::IsVisible(const Point &pos) const
{
	return FogTileUncovered(ConvertPointToFog(pos), &VisibleBitmap);
}

bool Map::IsExplored(const Point &pos) const
{
	return FogTileUncovered(ConvertPointToFog(pos), &ExploredBitmap);
}

//returns direction of area boundary, returns -1 if it isn't a boundary
WMPDirection Map::WhichEdge(const Point &s) const
{
	Point tileP = ConvertCoordToTile(s);
	if (!(GetBlocked(tileP) & PathMapFlags::TRAVEL)) {
		Log(DEBUG, "Map", "This isn't a travel region [%d.%d]?",
			tileP.x, tileP.y);
		return WMPDirection::NONE;
	}
	// FIXME: is this backwards?
	const Size& mapSize = PropsSize();
	tileP.x *= mapSize.h;
	tileP.y *= mapSize.w;
	if (tileP.x > tileP.y) { //north or east
		if (mapSize.w * mapSize.h > tileP.x + tileP.y) { //
			return WMPDirection::NORTH;
		}
		return WMPDirection::EAST;
	}
	//south or west
	if (mapSize.w * mapSize.h < tileP.x + tileP.y) { //
		return WMPDirection::SOUTH;
	}
	return WMPDirection::WEST;
}

//--------ambients----------------
void Map::SetupAmbients() const
{
	AmbientMgr *ambim = core->GetAudioDrv()->GetAmbientMgr();
	if (!ambim) return;
	ambim->reset();
	ambim->setAmbients( ambients );
}

ieWord Map::GetAmbientCount(bool toSave) const
{
	if (!toSave) return static_cast<ieWord>(ambients.size());

	ieWord ambiCount = 0;
	for (const Ambient *ambient : ambients) {
		if (!(ambient->flags & IE_AMBI_NOSAVE)) ambiCount++;
	}
	return ambiCount;
}

void Map::AddMapNote(const Point& point, ieWord color, const String &text, bool readonly)
{
	AddMapNote(point, MapNote(text, color, readonly));
}

void Map::AddMapNote(const Point& point, ieWord color, ieStrRef strref, bool readonly)
{
	AddMapNote(point, MapNote(strref, color, readonly));
}

void Map::AddMapNote(const Point &point, const MapNote& note)
{
	RemoveMapNote(point);
	mapnotes.push_back(note);
	mapnotes.back().Pos = point;
}

void Map::RemoveMapNote(const Point &point)
{
	std::vector<MapNote>::iterator it = mapnotes.begin();
	for (; it != mapnotes.end(); ++it) {
		if (!it->readonly && it->Pos == point) {
			mapnotes.erase(it);
			break;
		}
	}
}

const MapNote* Map::MapNoteAtPoint(const Point& point, unsigned int radius) const
{
	size_t i = mapnotes.size();
	while (i--) {
		if (Distance(point, mapnotes[i].Pos) < radius) {
			return &mapnotes[i];
		}
	}
	return NULL;
}

//--------spawning------------------
void Map::LoadIniSpawn()
{
	if (core->HasFeature(GF_RESDATA_INI)) {
		// 85 cases where we'd miss the ini and 1 where we'd use the wrong one
		INISpawn = new IniSpawn(this, ResRef(scriptName));
	} else {
		INISpawn = new IniSpawn(this, WEDResRef);
	}
}

bool Map::SpawnCreature(const Point &pos, const char *creResRef, int radiusx, int radiusy, ieWord rwdist, int *difficulty, unsigned int *creCount)
{
	bool spawned = false;
	const SpawnGroup *sg = nullptr;
	bool first = (creCount ? *creCount == 0 : true);
	int level = (difficulty ? *difficulty : core->GetGame()->GetTotalPartyLevel(true));
	size_t count = 1;

	if (Spawns::Get().vars.count(creResRef)) {
		sg = &Spawns::Get().vars.at(creResRef);
		if (first || (level >= sg->Level())) {
			count = sg->Count();
		} else {
			count = 0;
		}
	}

	while (count--) {
		Actor *creature = gamedata->GetCreature(sg ? (*sg)[count].CString() : creResRef);
		if (!creature) {
			continue;
		}

		// ensure a minimum power level, since many creatures have this as 0
		int cpl = creature->Modified[IE_XP] ? creature->Modified[IE_XP] : 1;

		// SpawnGroups are all or nothing but make sure we spawn
		// at least one creature if this is the first
		if (level >= cpl || sg || first) {
			AddActor(creature, true);
			creature->SetPosition(pos, true, radiusx, radiusy);
			creature->HomeLocation = pos;
			creature->maxWalkDistance = rwdist;
			creature->Spawned = true;
			creature->RefreshEffects(NULL);
			if (difficulty && !sg) *difficulty -= cpl;
			if (creCount) (*creCount)++;
			spawned = true;
		}
	}

	if (spawned && sg && difficulty) {
		*difficulty -= sg->Level();
	}
		
	return spawned;
}

void Map::TriggerSpawn(Spawn *spawn)
{
	//is it still active
	if (!spawn->Enabled) {
		return;
	}
	//temporarily disabled?
	if ((spawn->Method & (SPF_NOSPAWN|SPF_WAIT)) == (SPF_NOSPAWN|SPF_WAIT)) {
		return;
	}

	//check schedule
	ieDword time = core->GetGame()->GameTime;
	if (!Schedule(spawn->appearance, time)) {
		return;
	}

	//check day or night chance
	bool day = core->GetGame()->IsDay();
	int chance = RAND(0, 99);
	if ((day && chance > spawn->DayChance) ||
		(!day && chance > spawn->NightChance)) {
		spawn->NextSpawn = time + spawn->Frequency * AI_UPDATE_TIME * 60;
		spawn->Method |= SPF_WAIT;
		return;
	}
	//create spawns
	int difficulty = spawn->Difficulty * core->GetGame()->GetTotalPartyLevel(true);
	unsigned int spawncount = 0;
	size_t i = RAND(size_t(0), spawn->Creatures.size() - 1);
	while (difficulty >= 0 && spawncount < spawn->Maximum) {
		if (!SpawnCreature(spawn->Pos, spawn->Creatures[i], 0, 0, spawn->rwdist, &difficulty, &spawncount)) {
			break;
		}
		if (++i >= spawn->Creatures.size()) {
			i = 0;
		}
		
	}
	//disable spawnpoint
	if (spawn->Method & SPF_ONCE || !(spawn->Method & SPF_NOSPAWN)) {
		spawn->Enabled = 0;
	} else {
		spawn->NextSpawn = time + spawn->Frequency * AI_UPDATE_TIME * 60;
		spawn->Method |= SPF_WAIT;
	}
}

void Map::UpdateSpawns() const
{
	//don't reactivate if there are spawns left in the area
	if (SpawnsAlive()) {
		return;
	}
	ieDword time = core->GetGame()->GameTime;
	for (auto spawn : spawns) {
		if ((spawn->Method & (SPF_NOSPAWN|SPF_WAIT)) == (SPF_NOSPAWN|SPF_WAIT)) {
			//only reactivate the spawn point if the party cannot currently see it;
			//also make sure the party has moved away some
			if (spawn->NextSpawn < time && !IsVisible(spawn->Pos) &&
				!GetActorInRadius(spawn->Pos, GA_NO_DEAD|GA_NO_ENEMY|GA_NO_NEUTRAL|GA_NO_UNSCHEDULED, SPAWN_RANGE * 2)) {
				spawn->Method &= ~SPF_WAIT;
			}
		}
	}
}

//--------restheader----------------
/*
Every spawn has a difficulty associated with it. For CREs this is the xp stat
and for groups it's the value in the difficulty row.
For every spawn, the difficulty sum of all spawns up to now (including the
current) is compared against (party level * rest header difficulty). If it's
greater, the spawning is aborted. If all the other conditions are true, at
least one creature is summoned, regardless the difficulty cap.
*/
int Map::CheckRestInterruptsAndPassTime(const Point &pos, int hours, int day)
{
	if (!RestHeader.CreatureNum || !RestHeader.Enabled || !RestHeader.Maximum) {
		core->GetGame()->AdvanceTime(hours * core->Time.hour_size);
		return 0;
	}

	// TODO: it appears there was a limit on how many rest encounters can
	// be triggered in a row (or area?), since HOFMode should increase it
	// by 1. It doesn't look like it was stored in the header, so perhaps
	// it was just a hardcoded limit to make the game more forgiving
	// OR did it increase the number of spawned creatures by 1, 2?

	//based on ingame timer
	int chance=day?RestHeader.DayChance:RestHeader.NightChance;
	bool interrupt = RAND(0, 99) < chance;
	unsigned int spawncount = 0;
	int spawnamount = core->GetGame()->GetTotalPartyLevel(true) * RestHeader.Difficulty;
	if (spawnamount < 1) spawnamount = 1;
	for (int i=0;i<hours;i++) {
		if (interrupt) {
			int idx = RAND(0, RestHeader.CreatureNum-1);
			const Actor *creature = gamedata->GetCreature(RestHeader.CreResRef[idx]);
			if (!creature) {
				core->GetGame()->AdvanceTime(core->Time.hour_size);
				continue;
			}

			displaymsg->DisplayString( RestHeader.Strref[idx], DMC_GOLD, IE_STR_SOUND );
			while (spawnamount > 0 && spawncount < RestHeader.Maximum) {
				if (!SpawnCreature(pos, RestHeader.CreResRef[idx], 20, 20, RestHeader.rwdist, &spawnamount, &spawncount)) {
					break;
				}
			}
			return hours-i;
		}
		// advance the time in hourly steps, so an interruption is timed properly
		core->GetGame()->AdvanceTime(core->Time.hour_size);
	}
	return 0;
}
	
Size Map::GetSize() const
{
	return TMap->GetMapSize();
}

void Map::FillExplored(bool explored)
{
	ExploredBitmap.fill(explored ? 0xff : 0x00);
}

void Map::ExploreTile(const Point &p)
{
	Point fogP = ConvertPointToFog(p);

	const Size fogSize = FogMapSize();
	if (!fogSize.PointInside(fogP)) {
		return;
	}
	
	ExploredBitmap[fogP] = true;
	VisibleBitmap[fogP] = true;
}

void Map::ExploreMapChunk(const Point &Pos, int range, int los)
{
	Point Tile;
	const Explore& explore = Explore::Get();

	if (range > explore.MaxVisibility) {
		range = explore.MaxVisibility;
	}
	int p = explore.VisibilityPerimeter;
	while (p--) {
		int Pass = 2;
		bool block = false;
		bool sidewall = false ;
		for (int i=0;i<range;i++) {
			Tile.x = Pos.x + explore.VisibilityMasks[i][p].x;
			Tile.y = Pos.y + explore.VisibilityMasks[i][p].y;

			if (los) {
				if (!block) {
					PathMapFlags type = GetBlocked(ConvertCoordToTile(Tile));
					if (bool(type & PathMapFlags::NO_SEE)) {
						block=true;
					} else if (bool(type & PathMapFlags::SIDEWALL)) {
						sidewall = true;
					} else if (sidewall)
					{
						block=true ;
					}
				}
				if (block) {
					Pass--;
					if (!Pass) break;
				}
			}
			ExploreTile(Tile);
		}
	}
}

void Map::UpdateFog()
{
	VisibleBitmap.fill(0);
	
	for (const auto actor : actors) {
		if (!actor->Modified[IE_EXPLORE]) continue;

		int state = actor->Modified[IE_STATE_ID];
		if (state & STATE_CANTSEE) continue;
		
		int vis2 = actor->Modified[IE_VISUALRANGE];
		if ((state&STATE_BLIND) || (vis2<2)) vis2=2; //can see only themselves
		ExploreMapChunk (actor->Pos, vis2+actor->GetAnims()->GetCircleSize(), 1);
		
		Spawn *sp = GetSpawnRadius(actor->Pos, SPAWN_RANGE); //30 * 12
		if (sp) {
			TriggerSpawn(sp);
		}
	}
}

// Valid values are - PathMapFlags::UNMARKED, PathMapFlags::PC, PathMapFlags::NPC
void Map::BlockSearchMap(const Point &Pos, unsigned int size, PathMapFlags value)
{
	// We block a circle of radius size-1 around (px,py)
	// Note that this does not exactly match BG2. BG2's approximations of
	// these circles are slightly different for sizes 6 and up.

	// Note: this is a larger circle than the one tested in GetBlocked.
	// This means that an actor can get closer to a wall than to another
	// actor. This matches the behaviour of the original BG2.

	if (size > MAX_CIRCLESIZE) size = MAX_CIRCLESIZE;
	if (size < 1) size = 1;
	int ppx = Pos.x/16;
	int ppy = Pos.y/12;
	unsigned int r=(size-1)*(size-1)+1;
	uint32_t* SrchMap = (uint32_t*)tileProps->LockSprite();
	const Size& mapSize = PropsSize();

	auto readSrchMap = [&](int pos) -> PathMapFlags {
		if (pos < 0 || pos > mapSize.Area()) {
			return PathMapFlags::IMPASSABLE;
		}
		uint32_t pixel = SrchMap[pos];
		return PathMapFlags((pixel & searchMapMask) >> tileProps->Format().Rshift);
	};
	
	auto writeSrchMap = [&](int pos, PathMapFlags val) -> void {
		uint32_t& pixel = SrchMap[pos];
		pixel = (pixel & ~searchMapMask) | (uint32_t(val) << tileProps->Format().Rshift);
	};
	
	for (unsigned int i=0; i<size; i++) {
		for (unsigned int j=0; j<size; j++) {
			if (i*i+j*j <= r) {
				unsigned int ppxpi = ppx+i;
				unsigned int ppypj = ppy+j;
				unsigned int ppxmi = ppx-i;
				unsigned int ppymj = ppy-j;
				int pos = ppypj * mapSize.w + ppxpi;
				PathMapFlags mapval = readSrchMap(pos);
				if (mapval != PathMapFlags::IMPASSABLE) {
					writeSrchMap(pos, (mapval & PathMapFlags::NOTACTOR) | value);
				}
				pos = ppymj * mapSize.w + ppxpi;
				mapval = readSrchMap(pos);
				if (mapval != PathMapFlags::IMPASSABLE) {
					writeSrchMap(pos, (mapval & PathMapFlags::NOTACTOR) | value);
				}
				pos = ppypj * mapSize.w + ppxmi;
				mapval = readSrchMap(pos);
				if (mapval != PathMapFlags::IMPASSABLE) {
					writeSrchMap(pos, (mapval & PathMapFlags::NOTACTOR) | value);
				}
				pos = ppymj * mapSize.w + ppxmi;
				mapval = readSrchMap(pos);
				if (mapval != PathMapFlags::IMPASSABLE) {
					writeSrchMap(pos, (mapval & PathMapFlags::NOTACTOR) | value);
				}
			}
		}
	}
	tileProps->UnlockSprite();
}

Spawn* Map::GetSpawn(const char *Name) const
{
	for (auto spawn : spawns) {
		if (stricmp(spawn->Name, Name) == 0) {
			return spawn;
		}
	}
	return NULL;
}

Spawn *Map::GetSpawnRadius(const Point &point, unsigned int radius) const
{
	for (auto spawn : spawns) {
		if (Distance(point, spawn->Pos) < radius) {
			return spawn;
		}
	}
	return NULL;
}

int Map::ConsolidateContainers()
{
	int itemcount = 0;
	int containercount = (int) TMap->GetContainerCount();
	while (containercount--) {
		Container * c = TMap->GetContainer( containercount);

		if (TMap->CleanupContainer(c) ) {
			objectStencils.erase(c);
			continue;
		}
		itemcount += c->inventory.GetSlotCount();
	}
	return itemcount;
}

//Pos could be [-1,-1] in which case it copies the ground piles to their
//original position in the second area
void Map::CopyGroundPiles(Map *othermap, const Point &Pos) const
{
	int containercount = (int) TMap->GetContainerCount();
	while (containercount--) {
		Container * c = TMap->GetContainer( containercount);
		if (c->containerType == IE_CONTAINER_PILE) {
			//creating (or grabbing) the container in the other map at the given position
			Container *othercontainer;
			if (Pos.IsInvalid()) {
				othercontainer = othermap->GetPile(c->Pos);
			} else {
				othercontainer = othermap->GetPile(Pos);
			}
			//transfer the pile to the other container
			unsigned int i=c->inventory.GetSlotCount();
			while (i--) {
				CREItem *item = c->RemoveItem(i, 0);
				othercontainer->AddItem(item);
			}
		}
	}
}

// merges pile 1 into pile 2
static void MergePiles(Container *donorPile, Container *pile)
{
	unsigned int i = donorPile->inventory.GetSlotCount();
	while (i--) {
		CREItem *item = donorPile->RemoveItem(i, 0);
		int count = pile->inventory.CountItems(item->ItemResRef, false);
		if (count == 0) {
			pile->AddItem(item);
			continue;
		}

		// ensure slots are stacked fully before adding new ones
		int skipped = count;
		while (count) {
			int slot = pile->inventory.FindItem(item->ItemResRef, 0, --count);
			if (slot == -1) {
				// probably an inventory bug, shouldn't happen
				Log(DEBUG, "Map", "MoveVisibleGroundPiles found unaccessible pile item: %s", item->ItemResRef.CString());
				skipped--;
				continue;
			}
			const CREItem *otheritem = pile->inventory.GetSlotItem(slot);
			if (otheritem->Usages[0] == otheritem->MaxStackAmount) {
				// already full (or nonstackable), nothing to do here
				skipped--;
				continue;
			}
			if (pile->inventory.MergeItems(slot, item) != ASI_SUCCESS) {
				// the merge either failed (add whole) or went over the limit (add remainder)
				pile->AddItem(item);
			}
			skipped = 1; // just in case we would be eligible for the safety net below
			break;
		}

		// all found slots were already unsuitable, so just dump the item to a new one
		if (!skipped) {
			pile->AddItem(item);
		}
	}
}

void Map::MoveVisibleGroundPiles(const Point &Pos)
{
	//creating the container at the given position
	Container *othercontainer;
	othercontainer = GetPile(Pos);

	int containercount = (int) TMap->GetContainerCount();
	while (containercount--) {
		Container * c = TMap->GetContainer( containercount);
		if (c->containerType == IE_CONTAINER_PILE && IsExplored(c->Pos)) {
			//transfer the pile to the other container
			MergePiles(c, othercontainer);
		}
	}

	// reshuffle the items so they are sorted
	unsigned int i = othercontainer->inventory.GetSlotCount();
	if (i < 3) {
		// nothing to do
		return;
	}

	// sort by removing all items that have copies and readding them at the end
	while (i--) {
		const CREItem *item = othercontainer->inventory.GetSlotItem(i);
		int count = othercontainer->inventory.CountItems(item->ItemResRef, false);
		if (count == 1) continue;

		while (count) {
			int slot = othercontainer->inventory.FindItem(item->ItemResRef, 0, --count);
			if (slot == -1) continue;
			// containers don't really care about position, so every new item is placed at the last spot
			CREItem *item = othercontainer->RemoveItem(slot, 0);
			othercontainer->AddItem(item);
		}
	}
}

Container *Map::GetPile(Point position)
{
	char heapname[32];

	//converting to search square
	position.x=position.x/16;
	position.y=position.y/12;
	snprintf(heapname, sizeof(heapname), "heap_%d.%d", position.x, position.y);
	//pixel position is centered on search square
	position.x=position.x*16+8;
	position.y=position.y*12+6;
	Container *container = TMap->GetContainer(position,IE_CONTAINER_PILE);
	if (!container) {
		container = AddContainer(heapname, IE_CONTAINER_PILE, nullptr);
		container->Pos=position;
		//bounding box covers the search square
		container->BBox = Region::RegionFromPoints(Point(position.x-8, position.y-6), Point(position.x+8,position.y+6));
	}
	return container;
}

void Map::AddItemToLocation(const Point &position, CREItem *item)
{
	Container *container = GetPile(position);
	container->AddItem(item);
}

Container* Map::AddContainer(const char* Name, unsigned short Type,
							 const std::shared_ptr<Gem_Polygon>& outline)
{
	Container* c = new Container();
	c->SetScriptName( Name );
	c->containerType = Type;
	c->outline = outline;
	c->SetMap(this);
	if (outline) {
		c->BBox = outline->BBox;
	}
	TMap->AddContainer( c );
	return c;
}

int Map::GetCursor(const Point &p) const
{
	if (!IsExplored(p)) {
		return IE_CURSOR_INVALID;
	}
	switch (GetBlocked(ConvertCoordToTile(p)) & (PathMapFlags::PASSABLE | PathMapFlags::TRAVEL)) {
		case PathMapFlags::IMPASSABLE:
			return IE_CURSOR_BLOCKED;
		case PathMapFlags::PASSABLE:
			return IE_CURSOR_WALK;
		default:
			return IE_CURSOR_TRAVEL;
	}
}

bool Map::HasWeather() const
{
	if ((AreaType & (AT_WEATHER|AT_OUTDOOR) ) != (AT_WEATHER|AT_OUTDOOR) ) {
		return false;
	}
	ieDword tmp = 1;
	core->GetDictionary()->Lookup("Weather", tmp);
	return !!tmp;
}

int Map::GetWeather() const
{
	if (Rain>=core->Roll(1,100,0) ) {
		if (Lightning>=core->Roll(1,100,0) ) {
			return WB_RARELIGHTNING|WB_RAIN;
		}
		return WB_RAIN;
	}
	if (Snow>=core->Roll(1,100,0) ) {
		return WB_SNOW;
	}
	// TODO: handle WB_FOG the same way when we start drawing it
	return WB_NORMAL;
}

void Map::FadeSparkle(const Point &pos, bool forced) const
{
	for (auto particle : particles) {
		if (particle->MatchPos(pos)) {
			if (forced) {
				//particles.erase(iter);
				particle->SetPhase(P_EMPTY);
			} else {
				particle->SetPhase(P_FADE);
			}
			return;
		}
	}
}

void Map::Sparkle(ieDword duration, ieDword color, ieDword type, const Point &pos, unsigned int FragAnimID, int Zpos)
{
	int style, path, grow, size, width, ttl;

	if (!Zpos) {
		Zpos = 30;
	}

	//the high word is ignored in the original engine (compatibility hack)
	switch(type&0xffff) {
	case SPARKLE_SHOWER: //simple falling sparks
		path = SP_PATH_FALL;
		grow = SP_SPAWN_FULL;
		size = 100;
		width = 40;
		ttl = duration;
		break;
	case SPARKLE_PUFF:
		path = SP_PATH_FOUNT; //sparks go up and down
		grow = SP_SPAWN_SOME;
		size = 40;
		width = 40;
		ttl = core->GetGame()->GameTime+Zpos;
		break;
	case SPARKLE_EXPLOSION: //this isn't in the original engine, but it is a nice effect to have
		path = SP_PATH_EXPL;
		grow = SP_SPAWN_SOME;
		size = 10;
		width = 40;
		ttl = core->GetGame()->GameTime+Zpos;
		break;
	default:
		path = SP_PATH_FLIT;
		grow = SP_SPAWN_SOME;
		size = 100;
		width = 40;
		ttl = duration;
		break;
	}
	Particles *sparkles = new Particles(size);
	sparkles->SetOwner(this);
	sparkles->SetRegion(pos.x-width/2, pos.y-Zpos, width, Zpos);
	sparkles->SetTimeToLive(ttl);

	if (FragAnimID) {
		style = SP_TYPE_BITMAP;
		sparkles->SetBitmap(FragAnimID);
	}
	else {
		style = SP_TYPE_POINT;
	}
	sparkles->SetType(style, path, grow);
	sparkles->SetColor(color);
	sparkles->SetPhase(P_GROW);

	spaIterator iter;
	for(iter=particles.begin(); (iter!=particles.end()) && ((*iter)->GetHeight()<pos.y); iter++) ;
	particles.insert(iter, sparkles);
}

//remove flags from actor if it has left the trigger area it had last entered
void Map::ClearTrap(Actor *actor, ieDword InTrap) const
{
	const InfoPoint *trap = TMap->GetInfoPoint(InTrap);
	if (!trap) {
		actor->SetInTrap(0);
	} else {
		if(!trap->outline->PointIn(actor->Pos)) {
			actor->SetInTrap(0);
		}
	}
}

void Map::SetTrackString(ieStrRef strref, int flg, int difficulty)
{
	trackString = strref;
	trackFlag = flg;
	trackDiff = (ieWord) difficulty;
}

bool Map::DisplayTrackString(const Actor *target) const
{
	// this stat isn't saved
	// according to the HoW manual the chance of success is:
	// +5% for every three levels and +5% per point of wisdom
	int skill = target->GetStat(IE_TRACKING);
	int success;
	if (core->HasFeature(GF_3ED_RULES)) {
		// ~Wilderness Lore check. Wilderness Lore (skill + D20 roll + WIS modifier) =  %d vs. ((Area difficulty pct / 5) + 10) = %d ( Skill + WIS MOD = %d ).~
		skill += target->LuckyRoll(1, 20, 0) + target->GetAbilityBonus(IE_WIS);
		success = skill > (trackDiff/5 + 10);
	} else {
		skill += (target->GetStat(IE_LEVEL)/3)*5 + target->GetStat(IE_WIS)*5;
		success = core->Roll(1, 100, trackDiff) > skill;
	}
	if (!success) {
		displaymsg->DisplayConstantStringName(STR_TRACKINGFAILED, DMC_LIGHTGREY, target);
		return true;
	}
	if (trackFlag) {
			char * str = core->GetCString( trackString);
			core->GetTokenDictionary()->SetAt( "CREATURE", str);
			displaymsg->DisplayConstantStringName(STR_TRACKING, DMC_LIGHTGREY, target);
			return false;
	}
	displaymsg->DisplayStringName(trackString, DMC_LIGHTGREY, target, 0);
	return false;
}

// returns a lightness level in the range of [0-100]
// since the lightmap is much smaller than the area, we need to interpolate
unsigned int Map::GetLightLevel(const Point &p) const
{
	Color c = GetLighting(p);
	// at night/dusk/dawn the lightmap color is adjusted by the color overlay. (Only get's darker.)
	const Color *tint = core->GetGame()->GetGlobalTint();
	if (tint) {
		return ((c.r-tint->r)*114 + (c.g-tint->g)*587 + (c.b-tint->b)*299)/2550;
	}
	return (c.r*114+c.g*587+c.b*299)/2550;
}

////////////////////AreaAnimation//////////////////
//Area animation

AreaAnimation::AreaAnimation()
{
	appearance = sequence = frame = transparency = height = 0;
	Flags = originalFlags = startFrameRange = skipcycle = startchance = 0;
	unknown48 = 0;
}

AreaAnimation::AreaAnimation(const AreaAnimation &src)
{
	animation = src.animation;
	sequence = src.sequence;
	Flags = src.Flags;
	originalFlags = src.originalFlags;
	Pos = src.Pos;
	appearance = src.appearance;
	frame = src.frame;
	transparency = src.transparency;
	height = src.height;
	startFrameRange = src.startFrameRange;
	skipcycle = src.skipcycle;
	startchance = src.startchance;
	unknown48 = 0;

	PaletteRef = src.PaletteRef;
	Name = src.Name;
	BAM = src.BAM;

	palette = src.palette ? src.palette->Copy() : NULL;

	// handles the rest: animation, resets animcount
	InitAnimation();
}

void AreaAnimation::InitAnimation()
{
	AnimationFactory* af = ( AnimationFactory* )
		gamedata->GetFactoryResource( BAM, IE_BAM_CLASS_ID );
	if (!af) {
		print("Cannot load animation: %s", BAM.CString());
		return;
	}
	
	auto GetAnimationPiece = [af, this](Animation::index_t animCycle)
	{
		Animation *anim = af->GetCycle(animCycle);
		if (!anim)
			anim = af->GetCycle(0);
		
		assert(anim);
		//this will make the animation stop when the game is stopped
		//a possible gemrb feature to have this flag settable in .are
		anim->gameAnimation = true;
		anim->SetFrame(frame); // sanity check it first
		anim->Flags = Flags;
		anim->pos = Pos;
		if (anim->Flags&A_ANI_MIRROR) {
			anim->MirrorAnimation();
		}

		return anim;
	};

	size_t animcount = af->GetCycleCount();
	animation.reserve(animcount);
	size_t existingcount = std::min(animation.size(), animcount);

	if (Flags & A_ANI_ALLCYCLES && animcount > 0) {
		size_t i = 0;
		for (; i < existingcount; ++i) {
			Animation* anim = GetAnimationPiece(i);
			animation[i] = std::move(*anim);
			delete anim;
		}
		for (; i < animcount; ++i) {
			Animation* anim = GetAnimationPiece(i);
			animation.push_back(std::move(*anim));
			delete anim;
		}
	} else if (animcount) {
		Animation* anim = GetAnimationPiece(sequence);
		animation.push_back(std::move(*anim));
		delete anim;
	}
	
	if (Flags & A_ANI_PALETTE) {
		SetPalette(PaletteRef);
	}
	if (Flags&A_ANI_BLEND) {
		BlendAnimation();
	}
}

void AreaAnimation::SetPalette(const ResRef &pal)
{
	Flags |= A_ANI_PALETTE;
	PaletteRef = pal;
	palette = gamedata->GetPalette(PaletteRef);
	if (Flags&A_ANI_BLEND) {
		//re-blending after palette change
		BlendAnimation();
	}
}

void AreaAnimation::BlendAnimation()
{
	//Warning! This function will modify a shared palette
	if (!palette) {
		// CHECKME: what should we do here? Currently copying palette
		// from first frame of first animation

		if (animation.empty()) return;
		Holder<Sprite2D> spr = animation[0].GetFrame(0);
		if (!spr) return;
		palette = spr->GetPalette()->Copy();
		PaletteRef.Reset();
	}
	palette->CreateShadedAlphaChannel();
}

bool AreaAnimation::Schedule(ieDword gametime) const
{
	if (!(Flags&A_ANI_ACTIVE) ) {
		return false;
	}

	//check for schedule
	return GemRB::Schedule(appearance, gametime);
}

int AreaAnimation::GetHeight() const
{
	return (Flags&A_ANI_BACKGROUND) ? ANI_PRI_BACKGROUND : height;
}

Region AreaAnimation::DrawingRegion() const
{
	Region r(Pos, Size());
	size_t ac = animation.size();
	while (ac--) {
		const Animation &anim = animation[ac];
		Region animRgn = anim.animArea;
		animRgn.x += Pos.x;
		animRgn.y += Pos.y;
		
		r.ExpandToRegion(animRgn);
	}
	return r;
}

void AreaAnimation::Draw(const Region &viewport, Color tint, BlitFlags flags) const
{
	Video* video = core->GetVideoDriver();
	
	if (transparency) {
		tint.a = 255 - transparency;
		flags |= BlitFlags::ALPHA_MOD;
	} else {
		tint.a = 255;
	}

	size_t ac = animation.size();
	while (ac--) {
		Animation &anim = animation[ac];
		Holder<Sprite2D> frame = anim.NextFrame();
		
		video->BlitGameSpriteWithPalette(frame, palette, Pos - viewport.origin, flags, tint);
	}
}

//change the tileset if needed and possible, return true if changed
//day_or_night = 1 means the normal day lightmap
bool Map::ChangeMap(bool day_or_night)
{
	//no need of change if the area is not extended night
	//if (((AreaType&(AT_DAYNIGHT|AT_EXTENDED_NIGHT))!=(AT_DAYNIGHT|AT_EXTENDED_NIGHT))) return false;
	if (!(AreaType&AT_EXTENDED_NIGHT)) return false;
	//no need of change if the area already has the right tilemap
	if ((DayNight == day_or_night) && GetTileMap()) return false;

	auto mM = GetImporter<MapMgr>(IE_ARE_CLASS_ID);
	//no need to open and read the .are file again
	//using the ARE class for this because ChangeMap is similar to LoadMap
	//it loads the lightmap and the minimap too, besides swapping the tileset
	if (!mM->ChangeMap(this, day_or_night) && !day_or_night) {
		Log(WARNING, "Map", "Invalid night lightmap, falling back to day lightmap.");
		mM->ChangeMap(this, true);
		DayNight = day_or_night;
	}
	return true;
}

void Map::SeeSpellCast(Scriptable *caster, ieDword spell) const
{
	if (caster->Type!=ST_ACTOR) {
		return;
	}

	// FIXME: this seems clearly wrong, but matches old gemrb behaviour
	unsigned short triggerType = trigger_spellcast;
	if (spell >= 3000)
		triggerType = trigger_spellcastinnate;
	else if (spell < 2000)
		triggerType = trigger_spellcastpriest;

	caster->AddTrigger(TriggerEntry(triggerType, caster->GetGlobalID(), spell));

	size_t i = actors.size();
	while (i--) {
		const Actor *witness = actors[i];
		if (CanSee(witness, caster, true, 0)) {
			caster->AddTrigger(TriggerEntry(triggerType, caster->GetGlobalID(), spell));
		}
	}
}

PathMapFlags Map::GetInternalSearchMap(const Point& p) const
{
	// TODO: the subtle difference betweenm GetInternalSearchMap and QuerySearchMap
	// is that this returns PathMapFlags::UNMARKED instead of PathMapFlags::IMPASSABLE
	// for out of bounds points. I dont know if that is an important distiction
	if (!PropsSize().PointInside(p)) {
		return PathMapFlags::UNMARKED;
	}
	return QuerySearchMap(p);
}

void Map::SetInternalSearchMap(const Point& p, PathMapFlags value)
{
	const Size& mapSize = PropsSize();
	if (!mapSize.PointInside(p)) {
		return;
	}

	auto it = tileProps->GetIterator();
	it.Advance(p.x + p.y * mapSize.w);
	
	Color values = it.ReadRGBA();
	it.WriteRGBA(uint8_t(value), values.g, values.b, values.a);
}

void Map::SetBackground(const ResRef &bgResRef, ieDword duration)
{
	ResourceHolder<ImageMgr> bmp = GetResourceHolder<ImageMgr>(bgResRef);

	Background = bmp->GetSprite2D();
	BgDuration = duration;
}

}
