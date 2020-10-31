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

#include "win32def.h"

#include "Ambient.h"
#include "AmbientMgr.h"
#include "Audio.h"
#include "DisplayMessage.h"
#include "Game.h"
#include "GameData.h"
#include "IniSpawn.h"
#include "MapMgr.h"
#include "MapReverb.h"
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
#include "Video.h"
#include "WorldMap.h"
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

#include <cmath>
#include <cassert>
#include <limits>

namespace GemRB {

#define YESNO(x) ( (x)?"Yes":"No")

#define ANI_PRI_BACKGROUND	-9999

// TODO: fix this hardcoded resource reference
static ieResRef PortalResRef={"EF03TPR3"};
static unsigned int PortalTime = 15;
static unsigned int MAX_CIRCLESIZE = 8;
static int MaxVisibility = 30;
static int VisibilityPerimeter; //calculated from MaxVisibility
static int NormalCost = 10;
static int AdditionalCost = 4;
static unsigned char Passable[16] = {
	4, 1, 1, 1, 1, 1, 1, 1, 0, 1, 8, 0, 0, 0, 3, 1
};
static Point **VisibilityMasks=NULL;

static bool PathFinderInited = false;
static Variables Spawns;
static int LargeFog;
static TerrainSounds *terrainsounds=NULL;
static int tsndcount = -1;

static void ReleaseSpawnGroup(void *poi)
{
	delete (SpawnGroup *) poi;
}

Spawn::Spawn() {
	Creatures = NULL;
	NextSpawn = Method = sduration = Count = Maximum = Difficulty = 0;
	DayChance = NightChance = Enabled = Frequency = 0;
	rwdist = owdist = appearance = 0;
	Name[0] = 0;
}

void Map::ReleaseMemory()
{
	if (VisibilityMasks) {
		for (int i=0;i<MaxVisibility;i++) {
			free(VisibilityMasks[i]);
		}
		free(VisibilityMasks);
		VisibilityMasks = NULL;
	}
	Spawns.RemoveAll(ReleaseSpawnGroup);
	PathFinderInited = false;
	if (terrainsounds) {
		delete [] terrainsounds;
		terrainsounds = NULL;
	}
}

static inline AnimationObjectType SelectObject(Actor *actor, int q, AreaAnimation *a, VEFObject *sca, Particles *spark, Projectile *pro, Container *pile)
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
		scah = sca->YPos;//+sca->ZPos;
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

//Preload spawn group entries (creature resrefs that reference groups of creatures)
static void InitSpawnGroups()
{
	ieResRef GroupName;

	AutoTable tab("spawngrp", true);

	Spawns.RemoveAll(NULL);
	Spawns.SetType( GEM_VARIABLES_POINTER );

	if (!tab)
		return;

	int i = tab->GetColNamesCount();
	while (i--) {
		int j=tab->GetRowCount();
		while (j--) {
			const char *crename = tab->QueryField( j,i );
			if (strcmp(crename, tab->QueryDefault())) break;
		}
		if (j>0) {
			SpawnGroup *creatures = new SpawnGroup(j);
			//difficulty
			creatures->Level = (ieDword) atoi( tab->QueryField(0,i) );
			for (;j;j--) {
				strnlwrcpy( creatures->ResRefs[j-1], tab->QueryField(j,i), 8 );
			}
			strnlwrcpy( GroupName, tab->GetColumnName( i ), 8 );
			Spawns.SetAt( GroupName, (void*) creatures );
		}
	}
}

//Preload the searchmap configuration
static void InitPathFinder()
{
	PathFinderInited = true;
	tsndcount = 0;
	AutoTable tm("pathfind");

	if (!tm) {
		return;
	}

	const char* poi;

	for (int i = 0; i < 16; i++) {
		poi = tm->QueryField( 0, i );
		if (*poi != '*')
			Passable[i] = atoi( poi );
	}
	poi = tm->QueryField( 1, 0 );
	if (*poi != '*')
		NormalCost = atoi( poi );
	poi = tm->QueryField( 1, 1 );
	if (*poi != '*')
		AdditionalCost = atoi( poi );
	int rc = tm->GetRowCount()-2;
	if (rc>0) {
		terrainsounds = new TerrainSounds[rc];
		tsndcount = rc;
		while(rc--) {
			strnuprcpy(terrainsounds[rc].Group,tm->GetRowName(rc+2), sizeof(ieResRef)-1 );
			for(int i = 0; i<16;i++) {
				strnuprcpy(terrainsounds[rc].Sounds[i], tm->QueryField(rc+2, i), sizeof(ieResRef)-1 );
			}
		}
	}
}

static void AddLOS(int destx, int desty, int slot)
{
	for (int i=0;i<MaxVisibility;i++) {
		int x = ((destx*i + MaxVisibility/2) / MaxVisibility) * 16;
		int y = ((desty*i + MaxVisibility/2) / MaxVisibility) * 12;
		if (LargeFog) {
			x += 16;
			y += 12;
		}
		VisibilityMasks[i][slot].x=(short) x;
		VisibilityMasks[i][slot].y=(short) y;
	}
}

static void InitExplore()
{
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

	VisibilityMasks = (Point **) malloc(MaxVisibility * sizeof(Point *) );
	for (int i = 0; i < MaxVisibility; i++) {
		VisibilityMasks[i] = (Point *) malloc(VisibilityPerimeter*sizeof(Point) );
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

Map::Map(void)
	: Scriptable( ST_AREA )
{
	area=this;
	TMap = NULL;
	LightMap = NULL;
	HeightMap = NULL;
	SmallMap = NULL;
	SrchMap = NULL;
	Walls = NULL;
	WallCount = 0;
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
	if (!PathFinderInited) {
		InitPathFinder();
		InitSpawnGroups();
		InitExplore();
	}
	ExploredBitmap = NULL;
	VisibleBitmap = NULL;
	version = 0;
	MasterArea = core->GetGame()->MasterArea(scriptName);
	Background = NULL;
	BgDuration = 0;
	LastGoCloser = 0;
	AreaFlags = AreaType = AreaDifficulty = 0;
	Rain = Snow = Fog = Lightning = DayNight = 0;
	trackString = trackFlag = trackDiff = 0;
	Width = Height = 0;
	RestHeader.Difficulty = RestHeader.CreatureNum = RestHeader.Maximum = RestHeader.Enabled = 0;
	RestHeader.DayChance = RestHeader.NightChance = RestHeader.sduration = RestHeader.rwdist = RestHeader.owdist = 0;
	SongHeader.reverbID = SongHeader.MainDayAmbientVol = SongHeader.MainNightAmbientVol = 0;
	reverb = NULL;
	MaterialMap = NULL;
}

Map::~Map(void)
{
	free( SrchMap );
	free( MaterialMap );

	//close the current container if it was owned by this map, this avoids a crash
	Container *c = core->GetCurrentContainer();
	if (c && c->GetCurrentArea()==this) {
		core->CloseCurrentContainer();
	}

	delete TMap;
	delete INISpawn;
	 for (auto anim : animations) {
		delete anim;
	}

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
	delete LightMap;
	delete HeightMap;
	Sprite2D::FreeSprite( SmallMap );
	for (int i = 0; i < QUEUE_COUNT; i++) {
		free(queue[i]);
		queue[i] = NULL;
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

	for (auto ambient : ambients) {
		delete ambient;
	}

	if (reverb) {
		delete reverb;
	}

	//malloc-d in AREImp
	free( ExploredBitmap );
	free( VisibleBitmap );
	if (Walls) {
		for (unsigned int i = 0; i < WallCount; i++) {
			delete Walls[i];
		}
		free( Walls );
	}
	WallCount=0;
}

void Map::ChangeTileMap(Image* lm, Sprite2D* sm)
{
	delete LightMap;
	Sprite2D::FreeSprite(SmallMap);

	LightMap = lm;
	SmallMap = sm;

	TMap->UpdateDoors();
}

void Map::AddTileMap(TileMap* tm, Image* lm, Bitmap* sr, Sprite2D* sm, Bitmap* hm)
{
	// CHECKME: leaks? Should the old TMap, LightMap, etc... be freed?
	TMap = tm;
	LightMap = lm;
	HeightMap = hm;
	SmallMap = sm;
	Width = (unsigned int) (TMap->XCellCount * 4);
	Height = (unsigned int) (( TMap->YCellCount * 64 + 63) / 12);
	//Internal Searchmap
	int y = sr->GetHeight();
	SrchMap = (unsigned short *) calloc(Width * Height, sizeof(unsigned short));
	MaterialMap = (unsigned short *) calloc(Width * Height, sizeof(unsigned short));
	while(y--) {
		int x=sr->GetWidth();
		while(x--) {
			unsigned short value = sr->GetAt(x,y) & PATH_MAP_AREAMASK;
			size_t index = y * Width + x;
			SrchMap[index] = Passable[value];
			MaterialMap[index] = value;
		}
	}

	//delete the original searchmap
	delete sr;
}
void Map::AutoLockDoors() const
{
	GetTileMap()->AutoLockDoors();
}

void Map::MoveToNewArea(const char *area, const char *entrance, unsigned int direction, int EveryOne, Actor *actor)
{
	char command[256];

	//change loader MOS image here
	//check worldmap entry, if that doesn't contain anything,
	//make a random pick

	Game* game = core->GetGame();
	if (EveryOne==CT_WHOLE) {
		//copy the area name if it exists on the worldmap
		unsigned int index;

		WMPAreaEntry* entry = core->GetWorldMap()->FindNearestEntry(area, index);
		if (entry) {
			memcpy (game->PreviousArea, entry->AreaName, 8);
		}

		//perform autosave
		core->GetSaveGameIterator()->CreateSaveGame(0, false);
	}
	const Map *map = game->GetMap(area, false);
	if (!map) {
		Log(ERROR, "Map", "Invalid map: %s", area);
		command[0]=0;
		return;
	}
	const Entrance *ent = nullptr;
	if (entrance[0]) {
		ent = map->GetEntrance( entrance );
		if (!ent) {
			Log(ERROR, "Map", "Invalid entrance '%s' for area %s", entrance, area);
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
	snprintf(command, sizeof(command), "LeaveArea(\"%s\",[%d.%d],%d)", area, X, Y, face);

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
	Game *game=core->GetGame();

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
void Map::DrawPortal(InfoPoint *ip, int enable)
{
	ieDword gotportal = HasVVCCell(PortalResRef, ip->Pos);

	if (enable) {
		if (gotportal>PortalTime) return;
		ScriptedAnimation *sca = gamedata->GetScriptedAnimation(PortalResRef, false);
		if (sca) {
			sca->SetBlend();
			sca->PlayOnce();
			//exact position, because HasVVCCell depends on the coordinates, PST had no coordinate offset anyway
			sca->XPos = ip->Pos.x;
			sca->YPos = ip->Pos.y;
			//this is actually ordered by time, not by height
			sca->ZPos = gotportal;
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
	if (!has_pcs && !(MasterArea && actors.size()) /*&& !CanFree()*/) {
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

	//Run actor scripts (only for 0 priority)
	int q=Qcount[PR_SCRIPT];

	Game *game = core->GetGame();
	bool timestop = game->IsTimestopActive();
	if (!timestop) {
		game->SetTimestopOwner(NULL);
	}

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

		actor->UpdateActorState(game->GameTime);
		actor->SetSpeed(false);
	}

	//clean up effects on dead actors too
	q=Qcount[PR_DISPLAY];
	while(q--) {
		Actor* actor = queue[PR_DISPLAY][q];
		actor->fxqueue.Cleanup();
	}

	ieDword time = game->Ticks; // make sure everything moves at the same time
	// Make actors pathfind if there are others nearby
	// in order to avoid bumping when possible
	q = Qcount[PR_SCRIPT];
	while (q--) {
		Actor* actor = queue[PR_SCRIPT][q];
		if (actor->GetRandomBackoff() || !actor->GetStep() || actor->GetSpeed() == 0) {
			continue;
		}
		Actor* nearActor = GetActorInRadius(actor->Pos, GA_NO_DEAD|GA_NO_UNSCHEDULED, actor->GetAnims()->GetCircleSize());
		if (nearActor && nearActor != actor) {
			actor->NewPath();
		}
	}

	q = Qcount[PR_SCRIPT];
	while (q--) {
		Actor* actor = queue[PR_SCRIPT][q];
		if (actor->GetRandomBackoff()) {
			actor->DecreaseBackoff();
			if (!actor->GetRandomBackoff() && actor->GetSpeed() > 0) {
				actor->NewPath();
			}
			continue;
		}
		DoStepForActor(actor, time);
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
				ip->TrapLaunch.x, ip->TrapLaunch.y);
		}
		ip->Update();
	}

	UpdateSpawns();
	GenerateQueues();
	SortQueues();
}

void Map::ResolveTerrainSound(ieResRef &sound, Point &Pos) const
{
	for(int i=0;i<tsndcount;i++) {
		if (!memcmp(sound, terrainsounds[i].Group, sizeof(ieResRef) ) ) {
			int type = MaterialMap[Pos.x/16 + Pos.y/12 * Width];
			memcpy(sound, terrainsounds[i].Sounds[type], sizeof(ieResRef) );
			return;
		}
	}
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
	BlockSearchMap(actor->Pos, actor->size, PATH_MAP_UNMARKED);

	// Restore the searchmap areas of any nearby actors that could
	// have been cleared by this BlockSearchMap(..., PATH_MAP_UNMARKED).
	// (Necessary since blocked areas of actors may overlap.)
	for (const Actor *neighbour : nearActors) {
		if (neighbour->BlocksSearchMap()) {
			BlockSearchMap(neighbour->Pos, neighbour->size, neighbour->IsPartyMember() ? PATH_MAP_PC : PATH_MAP_NPC);
		}
	}
}

void Map::DrawHighlightables() const
{
	// NOTE: piles are drawn in the main queue
	unsigned int i = 0;
	const Container *c;

	while ( (c = TMap->GetContainer(i++))!=NULL ) {
		if (c->Highlight) {
			if (c->Type != IE_CONTAINER_PILE) {
				c->DrawOutline();
			}
		}
	}

	const Door *d;
	i = 0;
	while ( (d = TMap->GetDoor(i++))!=NULL ) {
		if (d->Highlight) d->DrawOutline();
	}

	const InfoPoint *p;
	i = 0;
	while ( (p = TMap->GetInfoPoint(i++))!=NULL ) {
		if (p->Highlight) p->DrawOutline();
	}
}

void Map::DrawPile(Region screen, int pileidx) const
{
	const Region vp = core->GetVideoDriver()->GetViewport();
	Container *c = TMap->GetContainer(pileidx);
	assert(c != NULL);

	Color tint = LightMap->GetPixel(c->Pos.x / 16, c->Pos.y / 12);
	tint.a = 255;

	if (c->Highlight) {
		c->DrawPile(true, screen, tint);
	} else {
		if (c->outline->BBox.IntersectsRegion(vp)) {
			c->DrawPile(false, screen, tint);
		}
	}
}

Container *Map::GetNextPile(int &index) const
{
	Container *c = TMap->GetContainer(index++);

	while (c) {
		if (c->Type == IE_CONTAINER_PILE) {
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

AreaAnimation *Map::GetNextAreaAnimation(aniIterator &iter, ieDword gametime) const
{
retry:
	if (iter==animations.end()) {
		return NULL;
	}
	AreaAnimation *a = *(iter++);
	if (!a->Schedule(gametime) ) {
		goto retry;
	}
	if (!IsVisible( a->Pos, !(a->Flags & A_ANI_NOT_IN_FOG)) ) {
		goto retry;
	}
	return a;
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

Projectile *Map::GetNextTrap(proIterator &iter) const
{
	Projectile *pro;

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

static ieDword oldgametime = 0;

//Draw the game area (including overlays, actors, animations, weather)
void Map::DrawMap(Region screen)
{
	if (!TMap) {
		return;
	}
	Game *game = core->GetGame();
	ieDword gametime = game->GameTime;

	//area specific spawn.ini files (a PST feature)
	if (INISpawn) {
		INISpawn->CheckSpawn();
	}

	//Blit the Background Map Animations (before actors)
	Video* video = core->GetVideoDriver();
	int bgoverride = false;

	if (Background) {
		if (BgDuration<gametime) {
			Sprite2D::FreeSprite(Background);
		} else {
			video->BlitSprite(Background,0,0,true);
			bgoverride = true;
		}
	}

	if (!bgoverride) {
		int rain, flags;

		if (game->IsTimestopActive()) {
			flags = TILE_GREY;
		}
		else if (AreaFlags&AF_DREAM) {
			flags = TILE_SEPIA;
		} else flags = 0;

		if (HasWeather()) {
			//zero when the weather particles are all gone
			rain = game->weather->GetPhase()-P_EMPTY;
		} else {
			rain = 0;
		}

		TMap->DrawOverlays( screen, rain, flags );
	}

	//drawing queues 1 and 0
	//starting with lower priority
	//so displayed, but inactive actors (dead) will be drawn over
	int q = PR_DISPLAY;
	int index = Qcount[q];
	Actor* actor = GetNextActor(q, index);
	aniIterator aniidx = animations.begin();
	scaIterator scaidx = vvcCells.begin();
	proIterator proidx = projectiles.begin();
	spaIterator spaidx = particles.begin();
	int pileidx = 0;
	Container *pile = GetNextPile(pileidx);

	AreaAnimation *a = GetNextAreaAnimation(aniidx, gametime);
	VEFObject *sca = GetNextScriptedAnimation(scaidx);
	Projectile *pro = GetNextProjectile(proidx);
	Particles *spark = GetNextSpark(spaidx);

	//draw all background animations first
	while (a && a->GetHeight() == ANI_PRI_BACKGROUND) {
		a->Draw(screen, this);
		a = GetNextAreaAnimation(aniidx, gametime);
	}

	if (!bgoverride) {
		//Draw Outlines
		DrawHighlightables();
	}

	// TODO: In at least HOW/IWD2 actor ground circles will be hidden by
	// an area animation with height > 0 even if the actors themselves are not
	// hidden by it.

	while (actor || a || sca || spark || pro || pile) {
		switch(SelectObject(actor,q,a,sca,spark,pro,pile)) {
		case AOT_ACTOR:
			assert(actor != NULL);
			actor->Draw( screen );
			actor->UpdateAnimations();
			actor = GetNextActor(q, index);
			break;
		case AOT_PILE:
			// draw piles
			if (!bgoverride) {
				DrawPile(screen, pileidx-1);
				pile = GetNextPile(pileidx);
			}
			break;
		case AOT_AREA:
			//draw animation
			a->Draw( screen, this );
			a = GetNextAreaAnimation(aniidx,gametime);
			break;
		case AOT_SCRIPTED:
			{
				Point Pos(0,0);

				Color tint = LightMap->GetPixel( sca->XPos / 16, sca->YPos / 12);
				tint.a = 255;
				bool endReached = sca->Draw(screen, Pos, tint, this, 0, -1, 0);
				if (endReached) {
					delete( sca );
					scaidx=vvcCells.erase(scaidx);
				} else {
					scaidx++;
				}
			}
			sca = GetNextScriptedAnimation(scaidx);
			break;
		case AOT_PROJECTILE:
			{
				int drawn;
				if (gametime>oldgametime) {
					drawn = pro->Update();
				} else {
					drawn = 1;
				}
				if (drawn) {
					pro->Draw( screen );
					proidx++;
				} else {
					delete( pro );
					proidx = projectiles.erase(proidx);
				}
			}
			pro = GetNextProjectile(proidx);
			break;
		case AOT_SPARK:
			{
				int drawn;
				if (gametime>oldgametime) {
					drawn = spark->Update();
				} else {
					drawn = 1;
				}
				if (drawn) {
					spark->Draw( screen );
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

	if ((core->FogOfWar&FOG_DRAWSEARCHMAP) && SrchMap) {
		DrawSearchMap(screen);
	} else {
		if ((core->FogOfWar&FOG_DRAWFOG) && TMap) {
			TMap->DrawFogOfWar( ExploredBitmap, VisibleBitmap, screen );
		}
	}

	int ipCount = 0;
	while (true) {
		//For each InfoPoint in the map
		assert(TMap);
		InfoPoint* ip = TMap->GetInfoPoint( ipCount++ );
		if (!ip)
			break;
		ip->DrawOverheadText(screen);
	}

	int cnCount = 0;
	while (true) {
		//For each Container in the map
		Container* cn = TMap->GetContainer( cnCount++ );
		if (!cn)
			break;
		cn->DrawOverheadText(screen);
	}

	int drCount = 0;
	while (true) {
		//For each Door in the map
		Door* dr = TMap->GetDoor( drCount++ );
		if (!dr)
			break;
		dr->DrawOverheadText(screen);
	}

	size_t i = actors.size();
	while (i--) {
		//For each Actor present
		//This must go AFTER the fog!
		//(maybe we should be using the queue?)
		Actor* actor = actors[i];
		actor->DrawOverheadText(screen);
	}

	oldgametime=gametime;
}

void Map::DrawSearchMap(const Region &screen) const
{
	Color inaccessible = { 128, 128, 128, 128 };
	Color impassible = { 128, 64, 64, 128 }; // red-ish
	Color sidewall = { 64, 64, 128, 128 }; // blue-ish
	Color actor = { 128, 64, 128, 128 }; // purple-ish
	Video *vid=core->GetVideoDriver();
	Region rgn=vid->GetViewport();
	Region block;

	block.w=16;
	block.h=12;
	int w = screen.w/16+2;
	int h = screen.h/12+2;

	for(int x=0;x<w;x++) {
		for(int y=0;y<h;y++) {
			unsigned char blockvalue = GetBlocked(x + rgn.x / 16, y + rgn.y / 12);
			block.x = screen.x + x * 16 - (rgn.x % 16);
			block.y = screen.y + y * 12 - (rgn.y % 12);
			if (!(blockvalue & PATH_MAP_PASSABLE)) {
				if (blockvalue == PATH_MAP_IMPASSABLE) { // 0
					vid->DrawRect(block,impassible);
				} else if (blockvalue & PATH_MAP_SIDEWALL) {
					vid->DrawRect(block,sidewall);
				} else if (!(blockvalue & PATH_MAP_ACTOR)){
					vid->DrawRect(block,inaccessible);
				}
			}
			if (blockvalue & PATH_MAP_ACTOR) {
				vid->DrawRect(block, actor);
			}
		}
	}

	// draw also pathfinding waypoints
	const Actor *act = core->GetFirstSelectedActor();
	if (!act) return;
	const PathNode *path = act->GetPath();
	if (!path) return;
	const PathNode *step = path->Next;
	Color waypoint = {0, 64, 128, 128}; // darker blue-ish
	int i = 0;
	block.w = 8;
	block.h = 6;
	while (step) {
		block.x = (step->x+64) - rgn.x;
		block.y = (step->y+6) - rgn.y;
		print("Waypoint %d at (%d, %d)", i, step->x, step->y);
		vid->DrawRect(block, waypoint);
		step = step->Next;
		i++;
	}
}

//adding animation in order, based on its height parameter
void Map::AddAnimation(AreaAnimation* panim)
{
	//copy external memory to core memory for msvc's sake
	AreaAnimation *anim = new AreaAnimation(panim);

	aniIterator iter;

	int Height = anim->GetHeight();
	for (iter = animations.begin(); (iter != animations.end()) && ((*iter)->GetHeight() < Height); ++iter) ;
	animations.insert(iter, anim);
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

void Map::Shout(Actor* actor, int shoutID, bool global)
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

void Map::ActorSpottedByPlayer(Actor *actor) const
{
	unsigned int animid;

	if(core->HasFeature(GF_HAS_BEASTS_INI)) {
		animid=actor->BaseStats[IE_ANIMATION_ID];
		if(core->HasFeature(GF_ONE_BYTE_ANIMID)) {
			animid&=0xff;
		}
		if (animid < (ieDword)CharAnimations::GetAvatarsCount()) {
			const AvatarStruct *avatar = CharAnimations::GetAvatarStruct(animid);
			core->GetGame()->SetBeastKnown(avatar->Bestiary);
		}
	}

	if (!(actor->GetInternalFlag()&IF_STOPATTACK) && !core->GetGame()->AnyPCInCombat()) {
		if (actor->Modified[IE_EA] > EA_EVILCUTOFF && !(actor->GetInternalFlag() & IF_TRIGGER_AP)) {
			actor->SetInternalFlag(IF_TRIGGER_AP, OP_OR);
			core->Autopause(AP_ENEMY, actor);
		}
	}
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
		InitActor(actor);
	}
}

void Map::InitActor(Actor *actor)
{
	//if a visible aggressive actor was put on the map, it is an autopause reason
	//guess game is always loaded? if not, then we'll crash
	ieDword gametime = core->GetGame()->GameTime;

	if (IsVisible(actor->Pos, false) && actor->Schedule(gametime, true) ) {
		ActorSpottedByPlayer(actor);
	}
	if (actor->InParty && core->HasFeature(GF_AREA_VISITED_VAR)) {
		char key[32];
		const size_t len = snprintf(key, sizeof(key),"%s_visited", scriptName);
		if (len > sizeof(key)) {
			Log(ERROR, "Map", "Area %s has a too long script name for generating _visited globals!", scriptName);
		}
		core->GetGame()->locals->SetAt(key, 1);
	}
}

void Map::AddActor(Actor* actor, bool init)
{
	//setting the current area for the actor as this one
	strnlwrcpy(actor->Area, scriptName, 8);
	if (!HasActor(actor)) {
		actors.push_back( actor );
	}
	if (init) {
		actor->SetMap(this);
		InitActor(actor);
	}
}

bool Map::AnyPCSeesEnemy() const
{
	ieDword gametime = core->GetGame()->GameTime;
	for (const Actor *actor : actors) {
		if (actor->Modified[IE_EA]>=EA_EVILCUTOFF) {
			if (IsVisible(actor->Pos, false) && actor->Schedule(gametime, true) ) {
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
		CopyResRef(actor->Area, "");
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
		return NULL;
	}
	for (auto actor : actors) {
		if (actor->GetGlobalID()==objectID) {
			return actor;
		}
	}
	return NULL;
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
				CREItem *itemslot = c->inventory.GetSlotItem(j);
				if (itemslot->Flags&IE_INV_ITEM_CRITICAL) {
					continue;
				}
			}
			TMap->CleanupContainer(c);
		}
	}
	// 3. reset living neutral actors to their HomeLocation,
	// in case they RandomWalked/flew themselves into a "corner" (mirroring original behaviour)
	for (Actor *actor : actors) {
		if (!actor->GetRandomWalkCounter()) continue;
		if (actor->GetStat(IE_MC_FLAGS) & MC_IGNORE_RETURN) continue;
		if (!actor->ValidTarget(GA_NO_DEAD|GA_NO_UNSCHEDULED|GA_NO_ALLY|GA_NO_ENEMY)) continue;
		if (!actor->HomeLocation.isnull() && !actor->HomeLocation.isempty() && actor->Pos != actor->HomeLocation) {
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

Scriptable *Map::GetActorByDialog(const char *resref) const
{
	for (auto actor : actors) {
		//if a busy or hostile actor shouldn't be found
		//set this to GD_CHECK
		if (strnicmp( actor->GetDialog(GD_NORMAL), resref, 8 ) == 0) {
			return actor;
		}
	}

	if (!core->HasFeature(GF_INFOPOINT_DIALOGS)) {
		return NULL;
	}

	// pst has plenty of talking infopoints, eg. in ar0508 (Lothar's cabinet)
	unsigned int i = TMap->GetInfoPointCount();
	while (i--) {
		InfoPoint* ip = TMap->GetInfoPoint(i);
		if (strnicmp(ip->GetDialog(), resref, 8) == 0) {
			return ip;
		}
	}

	// move higher if someone needs talking doors
	i = TMap->GetDoorCount();
	while (i--) {
		Door* door = TMap->GetDoor(i);
		if (strnicmp(door->GetDialog(), resref, 8) == 0) {
			return door;
		}
	}
	return NULL;
}

// NOTE: this function is not as general as it sounds
// currently only looks at the party, since it is enough for the only known user
// relies on an override item we create, with the resref matching the dialog one!
// currently only handles dmhead, since no other users have been found yet (to avoid checking whole inventory)
Scriptable *Map::GetItemByDialog(ieResRef resref) const
{
	const Game *game = core->GetGame();
	ieResRef itemref;
	// choose the owner of the dialog via passed dialog ref
	if (strnicmp(resref, "dmhead", 8)) {
		Log(WARNING, "Map", "Encountered new candidate item for GetItemByDialog? %s", resref);
		return NULL;
	}
	CopyResRef(itemref, "mertwyn");

	int i = game->GetPartySize(true);
	while (i--) {
		const Actor *pc = game->GetPC(i, true);
		int slot = pc->inventory.FindItem(itemref, 0);
		if (slot == -1) continue;
		const CREItem *citem = pc->inventory.GetSlotItem(slot);
		if (!citem) continue;
		const Item *item = gamedata->GetItem(citem->ItemResRef);
		if (!item) continue;
		if (strnicmp(item->Dialog, resref, 8)) continue;

		// finally, spawn (dmhead.cre) from our override as a substitute talker
		// the cre file is set up to be invisible, invincible and immune to several things
		Actor *surrogate = gamedata->GetCreature(resref);
		if (!surrogate) {
			error("Map", "GetItemByDialog found the right item, but creature is missing: %s!", resref);
			// error is fatal
		}
		Map *map = pc->GetCurrentArea();
		map->AddActor(surrogate, true);
		surrogate->SetPosition(pc->Pos, 0);

		return surrogate;
	}
	return NULL;
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

int Map::GetActorInRect(Actor**& actorlist, const Region& rgn, bool onlyparty) const
{
	actorlist = ( Actor * * ) malloc( actors.size() * sizeof( Actor * ) );
	int count = 0;
	for (auto actor : actors) {
//use this function only for party?
		if (onlyparty && actor->GetStat(IE_EA)>EA_CHARMED) {
			continue;
		}
		// this is called by non-selection code..
		if (onlyparty && !actor->ValidTarget(GA_SELECT))
			continue;
		if (!actor->ValidTarget(GA_NO_DEAD|GA_NO_UNSCHEDULED))
			continue;
		if ((actor->Pos.x<rgn.x) || (actor->Pos.y<rgn.y))
			continue;
		if ((actor->Pos.x>rgn.x+rgn.w) || (actor->Pos.y>rgn.y+rgn.h) )
			continue;
		actorlist[count++] = actor;
	}
	actorlist = ( Actor * * ) realloc( actorlist, count * sizeof( Actor * ) );
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

unsigned int Map::GetBlockedNavmap(unsigned int x, unsigned int y) const
{
	return GetBlocked(x / 16, y / 12);
}

// Args are in searchmap coordinates
// The default behavior is for actors to be blocking
// If they shouldn't be, the caller should check for PATH_MAP_PASSABLE | PATH_MAP_ACTOR
unsigned int Map::GetBlocked(unsigned int x, unsigned int y) const
{
	if (y>=Height || x>=Width) {
		return PATH_MAP_IMPASSABLE;
	}
	unsigned int ret = SrchMap[y*Width+x];
	if (ret & (PATH_MAP_DOOR_IMPASSABLE|PATH_MAP_ACTOR)) {
		ret &= ~PATH_MAP_PASSABLE;
	}
	if (ret & PATH_MAP_DOOR_OPAQUE) {
		ret = PATH_MAP_SIDEWALL;
	}
	return ret;
}

// Args are in navmap coordinates
unsigned int Map::GetBlockedInRadius(unsigned int px, unsigned int py, unsigned int size, bool stopOnImpassable) const
{
	// We check a circle of radius size-2 around (px,py)
	// Note that this does not exactly match BG2. BG2's approximations of
	// these circles are slightly different for sizes 7 and up.

	if (size > MAX_CIRCLESIZE) size = MAX_CIRCLESIZE;
	if (size < 2) size = 2;
	unsigned int ret = 0;

	unsigned int r = (size - 2) * (size - 2) + 1;
	if (size == 2) r = 0;
	for (unsigned int i = 0; i < size - 1; i++) {
		for (unsigned int j = 0; j < size - 1; j++) {
			if (i * i + j * j <= r) {
				unsigned int retBotRight = GetBlockedNavmap(px + i * 16, py + j * 12);
				unsigned int retTopRight = GetBlockedNavmap(px + i * 16, py - j * 12);
				unsigned int retBotLeft = GetBlockedNavmap(px - i * 16, py + j * 12);
				unsigned int retTopLeft = GetBlockedNavmap(px - i * 16, py - j * 12);
				if (stopOnImpassable) {
					if (retBotRight == PATH_MAP_IMPASSABLE || retBotLeft == PATH_MAP_IMPASSABLE || retTopRight == PATH_MAP_IMPASSABLE || retTopLeft == PATH_MAP_IMPASSABLE) {
						return PATH_MAP_IMPASSABLE;
					}
				}
				ret |= (retBotRight | retTopRight | retBotLeft | retTopLeft);
			}
		}
	}
	if (ret & (PATH_MAP_DOOR_IMPASSABLE|PATH_MAP_ACTOR|PATH_MAP_SIDEWALL)) {
		ret &= ~PATH_MAP_PASSABLE;
	}
	if (ret & PATH_MAP_DOOR_OPAQUE) {
		ret = PATH_MAP_SIDEWALL;
	}

	return ret;
}

unsigned int Map::GetBlockedInLine(const Point &s, const Point &d, bool stopOnImpassable, const Actor *caller) const
{
	unsigned int ret = 0;
	Point p = s;
	while (p != d) {
		double dx = d.x - p.x;
		double dy = d.y - p.y;
		double factor = caller && caller->GetSpeed() ? double(gamedata->GetStepTime()) / double(caller->GetSpeed()) : 1;
		NormalizeDeltas(dx, dy, factor);
		p.x += dx;
		p.y += dy;
		int blockStatus = GetBlockedNavmap(p.x, p.y);
		if (stopOnImpassable && blockStatus == PATH_MAP_IMPASSABLE) {
			return PATH_MAP_IMPASSABLE;
		}
		ret |= blockStatus;
	}
	if (ret & (PATH_MAP_DOOR_IMPASSABLE|PATH_MAP_ACTOR|PATH_MAP_SIDEWALL)) {
		ret &= ~PATH_MAP_PASSABLE;
	}
	if (ret & PATH_MAP_DOOR_OPAQUE) {
		ret = PATH_MAP_SIDEWALL;
	}

	return ret;
}

// PATH_MAP_SIDEWALL obstructs LOS, while PATH_MAP_IMPASSABLE doesn't
bool Map::IsVisibleLOS(const Point &s, const Point &d, const Actor *caller) const
{
	unsigned ret = GetBlockedInLine(s, d, false, caller);
	return !(ret & PATH_MAP_SIDEWALL);
}

// Used by the pathfinder, so PATH_MAP_IMPASSABLE obstructs walkability
bool Map::IsWalkableTo(const Point &s, const Point &d, bool actorsAreBlocking, const Actor *caller) const
{
	unsigned ret = GetBlockedInLine(s, d, true, caller);
	return ret & (PATH_MAP_PASSABLE | PATH_MAP_TRAVEL | (actorsAreBlocking ? 0 : PATH_MAP_ACTOR));
}

//flags:0 - never dither (full cover)
//	1 - dither if polygon wants it
//	2 - always dither

SpriteCover* Map::BuildSpriteCover(int x, int y, int xpos, int ypos,
	unsigned int width, unsigned int height, int flags, bool areaanim)
{
	SpriteCover* sc = new SpriteCover;
	sc->worldx = x;
	sc->worldy = y;
	sc->XPos = xpos;
	sc->YPos = ypos;
	sc->Width = width;
	sc->Height = height;

	Video* video = core->GetVideoDriver();
	video->InitSpriteCover(sc, flags);

	unsigned int wpcount = GetWallCount();
	for (unsigned int i = 0; i < wpcount; ++i)
	{
		Wall_Polygon* wp = GetWallGroup(i);
		if (!wp) continue;
		if (!wp->PointCovered(x, y)) continue;
		if (areaanim && !(wp->GetPolygonFlag() & WF_COVERANIMS)) continue;

		video->AddPolygonToSpriteCover(sc, wp);
	}

	return sc;
}

void Map::ActivateWallgroups(unsigned int baseindex, unsigned int count, int flg) const
{
	if (!Walls) {
		return;
	}
	for (unsigned int i = baseindex; i < baseindex + count; ++i) {
		Wall_Polygon* wp = GetWallGroup(i);
		if (!wp)
			continue;
		ieDword value=wp->GetPolygonFlag();
		if (flg)
			value&=~WF_DISABLED;
		else
			value|=WF_DISABLED;
		wp->SetPolygonFlag(value);
	}
	//all actors will have to generate a new spritecover
	for (auto actor : actors) {
		actor->SetSpriteCover(NULL);
	}
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
		} else {
			//dead actors are always visible on the map, but run no scripts
			if ((stance == IE_ANI_TWITCH) || (stance == IE_ANI_DIE) ) {
				priority = PR_DISPLAY;
			} else {
				//isvisible flag is false (visibilitymap) here,
				//coz we want to reactivate creatures that
				//just became visible
				if (IsVisible(actor->Pos, false) && actor->Schedule(gametime, false) ) {
					priority = PR_SCRIPT; //run scripts and display, activated now
					//more like activate!
					actor->Activate();
					ActorSpottedByPlayer(actor);
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
}

//the original qsort implementation was flawed
void Map::SortQueues()
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
ieDword Map::HasVVCCell(const ieResRef resource, const Point &p) const
{
	ieDword ret = 0;

	for (const VEFObject *vvc: vvcCells) {
		if (!p.isempty()) {
			if (vvc->XPos!=p.x) continue;
			if (vvc->YPos!=p.y) continue;
		}
		if (strnicmp(resource, vvc->ResName, sizeof(ieResRef))) continue;
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

	for(iter=vvcCells.begin();iter!=vvcCells.end() && (*iter)->ZPos<vvc->ZPos; iter++) ;
	vvcCells.insert(iter, vvc);
}

AreaAnimation *Map::GetAnimation(const char *Name) const
{
	for (auto anim : animations) {
		if (anim->Name[0] && (strnicmp(anim->Name, Name, 32) == 0)) {
			return anim;
		}
	}
	return NULL;
}

Spawn *Map::AddSpawn(char* Name, int XPos, int YPos, ieResRef *creatures, unsigned int count)
{
	Spawn* sp = new Spawn();
	strnspccpy(sp->Name, Name, 32);
	if (count>MAX_RESCOUNT) {
		count=MAX_RESCOUNT;
	}
	sp->Pos.x = (ieWord) XPos;
	sp->Pos.y = (ieWord) YPos;
	sp->Count = count;
	sp->Creatures = (ieResRef *) calloc( count, sizeof(ieResRef) );
	for( unsigned int i=0;i<count;i++) {
		strnlwrcpy(sp->Creatures[i],creatures[i],8);
	}
	spawns.push_back( sp );
	return sp;
}

void Map::AddEntrance(char* Name, int XPos, int YPos, short Face)
{
	Entrance* ent = new Entrance();
	strlcpy( ent->Name, Name, sizeof(ent->Name) );
	ent->Pos.x = (ieWord) XPos;
	ent->Pos.y = (ieWord) YPos;
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
	for (Actor *act : actors) {
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
			CopyResRef(actor->Area, "");
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
	}
	//we expect the area to be swapped out, so we simply remove the corpses now
	PurgeArea(false);
	return true;
}

void Map::dump(bool show_actors) const
{
	StringBuffer buffer;
	buffer.appendFormatted( "Debugdump of Area %s:\n", scriptName );
	buffer.append("Scripts:");

	for (size_t i = 0; i < MAX_SCRIPTS; i++) {
		const char* poi = "<none>";
		if (Scripts[i]) {
			poi = Scripts[i]->GetName();
		}
		buffer.appendFormatted( " %.8s", poi );
	}
	buffer.append("\n");
	buffer.appendFormatted( "Area Global ID:  %d\n", GetGlobalID());
	buffer.appendFormatted( "OutDoor: %s\n", YESNO(AreaType & AT_OUTDOOR ) );
	buffer.appendFormatted( "Day/Night: %s\n", YESNO(AreaType & AT_DAYNIGHT ) );
	buffer.appendFormatted( "Extended night: %s\n", YESNO(AreaType & AT_EXTENDED_NIGHT ) );
	buffer.appendFormatted( "Weather: %s\n", YESNO(AreaType & AT_WEATHER ) );
	buffer.appendFormatted( "Area Type: %d\n", AreaType & (AT_CITY|AT_FOREST|AT_DUNGEON) );
	buffer.appendFormatted( "Can rest: %s\n", YESNO(AreaType & AT_CAN_REST_INDOORS) );

	if (show_actors) {
		buffer.append("\n");
		for (auto actor : actors) {
			if (actor->ValidTarget(GA_NO_DEAD|GA_NO_UNSCHEDULED)) {
				buffer.appendFormatted("Actor: %s (%d %s) at %d.%d\n", actor->GetName(1), actor->GetGlobalID(), actor->GetScriptName(), actor->Pos.x, actor->Pos.y);
			}
		}
	}
	Log(DEBUG, "Map", buffer);
}

bool Map::AdjustPositionX(Point &goal, unsigned int radiusx, unsigned int radiusy) const
{
	unsigned int minx = 0;
	if ((unsigned int) goal.x > radiusx)
		minx = goal.x - radiusx;
	unsigned int maxx = goal.x + radiusx + 1;
	if (maxx > Width)
		maxx = Width;

	for (unsigned int scanx = minx; scanx < maxx; scanx++) {
		if ((unsigned int) goal.y >= radiusy) {
			if (GetBlocked(scanx, goal.y - radiusy) & PATH_MAP_PASSABLE) {
				goal.x = (ieWord) scanx;
				goal.y = (ieWord) (goal.y - radiusy);
				return true;
			}
		}
		if (goal.y + radiusy < Height) {
			if (GetBlocked(scanx, goal.y + radiusy) & PATH_MAP_PASSABLE) {
				goal.x = (ieWord) scanx;
				goal.y = (ieWord) (goal.y + radiusy);
				return true;
			}
		}
	}
	return false;
}

bool Map::AdjustPositionY(Point &goal, unsigned int radiusx,  unsigned int radiusy) const
{
	unsigned int miny = 0;
	if ((unsigned int) goal.y > radiusy)
		miny = goal.y - radiusy;
	unsigned int maxy = goal.y + radiusy + 1;
	if (maxy > Height)
		maxy = Height;
	for (unsigned int scany = miny; scany < maxy; scany++) {
		if ((unsigned int) goal.x >= radiusx) {
			if (GetBlocked(goal.x - radiusx, scany) & PATH_MAP_PASSABLE) {
				goal.x = (ieWord) (goal.x - radiusx);
				goal.y = (ieWord) scany;
				return true;
			}
		}
		if (goal.x + radiusx < Width) {
			if (GetBlocked(goal.x + radiusx, scany) & PATH_MAP_PASSABLE) {
				goal.x = (ieWord) (goal.x + radiusx);
				goal.y = (ieWord) scany;
				return true;
			}
		}
	}
	return false;
}

void Map::AdjustPositionNavmap(NavmapPoint &goal, unsigned int radiusx, unsigned int radiusy) const
{
	NavmapPoint smptGoal(goal.x / 16, goal.y / 12);
	AdjustPosition(smptGoal, radiusx, radiusy);
	goal.x = smptGoal.x * 16 + 8;
	goal.y = smptGoal.y * 12 + 6;
}

void Map::AdjustPosition(SearchmapPoint &goal, unsigned int radiusx, unsigned int radiusy) const
{
	if ((unsigned int) goal.x > Width) {
		goal.x = (ieWord) Width;
	}
	if ((unsigned int) goal.y > Height) {
		goal.y = (ieWord) Height;
	}

	while(radiusx<Width || radiusy<Height) {
		//lets make it slightly random where the actor will appear
		if (RAND(0,1)) {
			if (AdjustPositionX(goal, radiusx, radiusy)) {
				return;
			}
			if (AdjustPositionY(goal, radiusx, radiusy)) {
				return;
			}
		} else {
			if (AdjustPositionY(goal, radiusx, radiusy)) {
				return;
			}
			if (AdjustPositionX(goal, radiusx, radiusy)) {
				return;
			}
		}
		if (radiusx<Width) {
			radiusx++;
		}
		if (radiusy<Height) {
			radiusy++;
		}
	}
}

//single point visible or not (visible/exploredbitmap)
//if explored = true then explored otherwise currently visible
bool Map::IsVisible(const Point &pos, int explored) const
{
	if (!VisibleBitmap)
		return false;
	int sX=pos.x/32;
	int sY=pos.y/32;
	if (sX<0) return false;
	if (sY<0) return false;
	int w = TMap->XCellCount * 2 + LargeFog;
	int h = TMap->YCellCount * 2 + LargeFog;
	if (sX>=w) return false;
	if (sY>=h) return false;
	int b0 = (sY * w) + sX;
	int by = b0/8;
	int bi = 1<<(b0%8);

	if (explored) return (ExploredBitmap[by] & bi)!=0;
	return (VisibleBitmap[by] & bi)!=0;
}

//returns direction of area boundary, returns -1 if it isn't a boundary
int Map::WhichEdge(const Point &s) const
{
	unsigned int sX=s.x/16;
	unsigned int sY=s.y/12;
	if (!(GetBlocked(sX, sY) & PATH_MAP_TRAVEL)) {
		Log(DEBUG, "Map", "This isn't a travel region [%d.%d]?",
			sX, sY);
		return -1;
	}
	sX*=Height;
	sY*=Width;
	if (sX>sY) { //north or east
		if (Width*Height>sX+sY) { //
			return WMP_NORTH;
		}
		return WMP_EAST;
	}
	//south or west
	if (Width*Height<sX+sY) { //
		return WMP_SOUTH;
	}
	return WMP_WEST;
}

//--------ambients----------------
void Map::SetupAmbients()
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

//--------mapnotes----------------
//text must be a pointer we can claim ownership of
void Map::AddMapNote(const Point &point, int color, String* text)
{
	AddMapNote(point, MapNote(text, color));
}

void Map::AddMapNote(const Point &point, int color, ieStrRef strref)
{
	AddMapNote(point, MapNote(strref, color));
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
		if (it->Pos == point) {
			mapnotes.erase(it);
			break;
		}
	}
}

const MapNote *Map::MapNoteAtPoint(const Point &point) const
{
	size_t i = mapnotes.size();
	while (i--) {
		if (Distance(point, mapnotes[i].Pos) < 10 ) {
			return &mapnotes[i];
		}
	}
	return NULL;
}

//--------spawning------------------
void Map::LoadIniSpawn()
{
	INISpawn = new IniSpawn(this);
	if (core->HasFeature(GF_RESDATA_INI)) {
		// 85 cases where we'd miss the ini and 1 where we'd use the wrong one
		INISpawn->InitSpawn(scriptName);
	} else {
		INISpawn->InitSpawn(WEDResRef);
	}
}

bool Map::SpawnCreature(const Point &pos, const char *creResRef, int radiusx, int radiusy, ieWord rwdist, int *difficulty, unsigned int *creCount)
{
	bool spawned = false;
	SpawnGroup *sg = NULL;
	void *lookup;
	bool first = (creCount ? *creCount == 0 : true);
	int level = (difficulty ? *difficulty : core->GetGame()->GetTotalPartyLevel(true));
	int count = 1;

	if (Spawns.Lookup(creResRef, lookup)) {
		sg = (SpawnGroup *) lookup;
		if (first || (level >= (int) sg->Level)) {
			count = sg->Count;
		} else {
			count = 0;
		}
	}

	while (count--) {
		Actor *creature = gamedata->GetCreature(sg ? sg->ResRefs[count] : creResRef);
		if (creature) {
			// ensure a minimum power level, since many creatures have this as 0
			int cpl = creature->Modified[IE_XP] ? creature->Modified[IE_XP] : 1;

			//SpawnGroups are all or nothing but make sure we spawn
			//at least one creature if this is the first
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
	}

	if (spawned && sg && difficulty) {
		*difficulty -= sg->Level;
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
	unsigned int spawncount = 0, i = RAND(0, spawn->Count-1);
	while (difficulty >= 0 && spawncount < spawn->Maximum) {
		if (!SpawnCreature(spawn->Pos, spawn->Creatures[i], 0, 0, spawn->rwdist, &difficulty, &spawncount)) {
			break;
		}
		if (++i >= spawn->Count) {
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
			if (spawn->NextSpawn < time && !IsVisible(spawn->Pos, false) &&
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

	//based on ingame timer
	int chance=day?RestHeader.DayChance:RestHeader.NightChance;
	bool interrupt = RAND(0, 99) < chance;
	unsigned int spawncount = 0;
	int spawnamount = core->GetGame()->GetTotalPartyLevel(true) * RestHeader.Difficulty;
	if (spawnamount < 1) spawnamount = 1;
	for (int i=0;i<hours;i++) {
		if (interrupt) {
			int idx = RAND(0, RestHeader.CreatureNum-1);
			Actor *creature = gamedata->GetCreature(RestHeader.CreResRef[idx]);
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

//--------explored bitmap-----------
int Map::GetExploredMapSize() const
{
	int x = TMap->XCellCount*2;
	int y = TMap->YCellCount*2;
	if (LargeFog) {
		x++;
		y++;
	}
	return (x*y+7)/8;
}

void Map::Explore(int setreset)
{
	memset (ExploredBitmap, setreset, GetExploredMapSize() );
}

void Map::SetMapVisibility(int setreset)
{
	memset( VisibleBitmap, setreset, GetExploredMapSize() );
}

// x, y are not in tile coordinates
void Map::ExploreTile(const Point &pos)
{
	int h = TMap->YCellCount * 2 + LargeFog;
	int y = pos.y/32;
	if (y < 0 || y >= h)
		return;

	int w = TMap->XCellCount * 2 + LargeFog;
	int x = pos.x/32;
	if (x < 0 || x >= w)
		return;

	int b0 = (y * w) + x;
	int by = b0/8;
	int bi = 1<<(b0%8);

	ExploredBitmap[by] |= bi;
	VisibleBitmap[by] |= bi;
}

void Map::ExploreMapChunk(const Point &Pos, int range, int los)
{
	Point Tile;

	if (range>MaxVisibility) {
		range=MaxVisibility;
	}
	int p=VisibilityPerimeter;
	while (p--) {
		int Pass = 2;
		bool block = false;
		bool sidewall = false ;
		for (int i=0;i<range;i++) {
			Tile.x = Pos.x+VisibilityMasks[i][p].x;
			Tile.y = Pos.y+VisibilityMasks[i][p].y;

			if (los) {
				if (!block) {
					int type = GetBlocked(Tile.x / 16, Tile.y / 12);
					if (type & PATH_MAP_NO_SEE) {
						block=true;
					} else if (type & PATH_MAP_SIDEWALL) {
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
	if (!(core->FogOfWar&FOG_DRAWFOG) ) {
		SetMapVisibility( -1 );
		Explore(-1);
	} else {
		SetMapVisibility( 0 );
	}

	for (size_t i = 0; i < actors.size(); i++) {
		const Actor *actor = actors[i];
		if (!actor->Modified[ IE_EXPLORE ] ) continue;
		if (core->FogOfWar&FOG_DRAWFOG) {
			int state = actor->Modified[IE_STATE_ID];
			if (state & STATE_CANTSEE) continue;
			int vis2 = actor->Modified[IE_VISUALRANGE];
			if ((state&STATE_BLIND) || (vis2<2)) vis2=2; //can see only themselves
			ExploreMapChunk (actor->Pos, vis2+actor->GetAnims()->GetCircleSize(), 1);
		}
		Spawn *sp = GetSpawnRadius(actor->Pos, SPAWN_RANGE); //30 * 12
		if (sp) {
			TriggerSpawn(sp);
		}
	}
}

// Valid values are - PATH_MAP_UNMARKED, PATH_MAP_PC, PATH_MAP_NPC
void Map::BlockSearchMap(const Point &Pos, unsigned int size, unsigned int value)
{
	// We block a circle of radius size-1 around (px,py)
	// Note that this does not exactly match BG2. BG2's approximations of
	// these circles are slightly different for sizes 6 and up.

	// Note: this is a larger circle than the one tested in GetBlocked.
	// This means that an actor can get closer to a wall than to another
	// actor. This matches the behaviour of the original BG2.

	if (size > MAX_CIRCLESIZE) size = MAX_CIRCLESIZE;
	if (size < 1) size = 1;
	unsigned int ppx = Pos.x/16;
	unsigned int ppy = Pos.y/12;
	unsigned int r=(size-1)*(size-1)+1;
	for (unsigned int i=0; i<size; i++) {
		for (unsigned int j=0; j<size; j++) {
			if (i*i+j*j <= r) {
				unsigned int ppxpi = ppx+i;
				unsigned int ppypj = ppy+j;
				unsigned int ppxmi = ppx-i;
				unsigned int ppymj = ppy-j;
				unsigned int pos = ppypj * Width + ppxpi;
				if (ppxpi < Width && ppypj < Height && SrchMap[pos] != PATH_MAP_IMPASSABLE) {
					SrchMap[pos] = (SrchMap[pos] & PATH_MAP_NOTACTOR) | value;
				}
				pos = ppymj * Width + ppxpi;
				if (ppxpi < Width && ppymj < Height && SrchMap[pos] != PATH_MAP_IMPASSABLE) {
					SrchMap[pos] = (SrchMap[pos] & PATH_MAP_NOTACTOR) | value;
				}
				pos = ppypj * Width + ppxmi;
				if (ppxmi < Width && ppypj < Height && SrchMap[pos] != PATH_MAP_IMPASSABLE) {
					SrchMap[pos] = (SrchMap[pos] & PATH_MAP_NOTACTOR) | value;
				}
				pos = ppymj * Width + ppxmi;
				if (ppxmi < Width && ppymj < Height && SrchMap[pos] != PATH_MAP_IMPASSABLE) {
					SrchMap[pos] = (SrchMap[pos] & PATH_MAP_NOTACTOR) | value;
				}
			}
		}
	}
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
			continue;
		}
		itemcount += c->inventory.GetSlotCount();
	}
	return itemcount;
}

//Pos could be [-1,-1] in which case it copies the ground piles to their
//original position in the second area
void Map::CopyGroundPiles(Map *othermap, const Point &Pos)
{
	int containercount = (int) TMap->GetContainerCount();
	while (containercount--) {
		Container * c = TMap->GetContainer( containercount);
		if (c->Type==IE_CONTAINER_PILE) {
			//creating (or grabbing) the container in the other map at the given position
			Container *othercontainer;
			if (Pos.isempty()) {
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
		int count = pile->inventory.CountItems(item->ItemResRef, 0);
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
				Log(DEBUG, "Map", "MoveVisibleGroundPiles found unaccessible pile item: %s", item->ItemResRef);
				skipped--;
				continue;
			}
			CREItem *otheritem = pile->inventory.GetSlotItem(slot);
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
		if (c->Type==IE_CONTAINER_PILE && IsVisible(c->Pos, true)) {
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
		CREItem *item = othercontainer->inventory.GetSlotItem(i);
		int count = othercontainer->inventory.CountItems(item->ItemResRef, 0);
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
	Point tmp[4];
	char heapname[32];

	//converting to search square
	position.x=position.x/16;
	position.y=position.y/12;
	snprintf(heapname, sizeof(heapname), "heap_%hd.%hd", position.x, position.y);
	//pixel position is centered on search square
	position.x=position.x*16+8;
	position.y=position.y*12+6;
	Container *container = TMap->GetContainer(position,IE_CONTAINER_PILE);
	if (!container) {
		//bounding box covers the search square
		tmp[0].x=position.x-8;
		tmp[0].y=position.y-6;
		tmp[1].x=position.x+8;
		tmp[1].y=position.y-6;
		tmp[2].x=position.x+8;
		tmp[2].y=position.y+6;
		tmp[3].x=position.x-8;
		tmp[3].y=position.y+6;
		Gem_Polygon* outline = new Gem_Polygon( tmp, 4 );
		container = AddContainer(heapname, IE_CONTAINER_PILE, outline);
		container->Pos=position;
	}
	return container;
}

void Map::AddItemToLocation(const Point &position, CREItem *item)
{
	Container *container = GetPile(position);
	container->AddItem(item);
}

Container* Map::AddContainer(const char* Name, unsigned short Type,
	Gem_Polygon* outline)
{
	Container* c = new Container();
	c->SetScriptName( Name );
	c->Type = Type;
	c->outline = outline;
	c->SetMap(this);
	TMap->AddContainer( c );
	return c;
}

int Map::GetCursor(const Point &p) const
{
	if (!IsVisible( p, true ) ) {
		return IE_CURSOR_INVALID;
	}
	switch (GetBlocked(p.x / 16, p.y / 12) & (PATH_MAP_PASSABLE | PATH_MAP_TRAVEL)) {
		case 0:
			return IE_CURSOR_BLOCKED;
		case PATH_MAP_PASSABLE:
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

void Map::FadeSparkle(const Point &pos, bool forced)
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
void Map::ClearTrap(Actor *actor, ieDword InTrap)
{
	InfoPoint *trap = TMap->GetInfoPoint(InTrap);
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
unsigned int Map::GetLightLevel(const Point &Pos) const
{
	Color c = LightMap->GetPixel(Pos.x/16, Pos.y/12);
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
	animation=NULL;
	animcount=0;
	palette=NULL;
	covers=NULL;
	appearance = sequence = frame = transparency = height = 0;
	Flags = originalFlags = startFrameRange = skipcycle = startchance = 0;
	unknown48 = 0;
	Name[0] = 0;
	BAM[0] = 0;
	PaletteRef[0] = 0;
}

AreaAnimation::AreaAnimation(AreaAnimation *src)
{
	animcount = src->animcount;
	sequence = src->sequence;
	animation = NULL;
	Flags = src->Flags;
	originalFlags = src->originalFlags;
	Pos.x = src->Pos.x;
	Pos.y = src->Pos.y;
	appearance = src->appearance;
	frame = src->frame;
	transparency = src->transparency;
	height = src->height;
	startFrameRange = src->startFrameRange;
	skipcycle = src->skipcycle;
	startchance = src->startchance;
	unknown48 = 0;

	memcpy(PaletteRef, src->PaletteRef, sizeof(PaletteRef));
	memcpy(Name, src->Name, sizeof(ieVariable));
	memcpy(BAM, src->BAM, sizeof(ieResRef));

	palette = src->palette ? new Palette(src->palette->col, src->palette->alpha) : NULL;
	// covers will get built once we try to draw it
	covers = NULL;

	// handles the rest: animation, resets animcount
	InitAnimation();
}

AreaAnimation::~AreaAnimation()
{
	for(int i=0;i<animcount;i++) {
		if (animation[i]) {
			delete (animation[i]);
		}
	}
	free(animation);
	gamedata->FreePalette(palette, PaletteRef);
	if (covers) {
		for(int i=0;i<animcount;i++) {
			delete covers[i];
		}
		free (covers);
	}
}

Animation *AreaAnimation::GetAnimationPiece(AnimationFactory *af, int animCycle)
{
	Animation *anim = af->GetCycle( ( unsigned char ) animCycle );
	if (!anim)
		anim = af->GetCycle( 0 );
	if (!anim) {
		print("Cannot load animation: %s", BAM);
		return NULL;
	}
	//this will make the animation stop when the game is stopped
	//a possible gemrb feature to have this flag settable in .are
	anim->gameAnimation = true;
	anim->SetPos(frame); // sanity check it first
	anim->Flags = Flags;
	anim->x = Pos.x;
	anim->y = Pos.y;
	if (anim->Flags&A_ANI_MIRROR) {
		anim->MirrorAnimation();
	}

	return anim;
}

void AreaAnimation::InitAnimation()
{
	AnimationFactory* af = ( AnimationFactory* )
		gamedata->GetFactoryResource( BAM, IE_BAM_CLASS_ID );
	if (!af) {
		print("Cannot load animation: %s", BAM);
		return;
	}

	//freeing up the previous animation
	for (int i=0; i<animcount && animation; i++) {
		delete animation[i];
	}
	free(animation);

	animcount = (int) af->GetCycleCount();
	if (Flags & A_ANI_ALLCYCLES && animcount > 0) {
		animation = (Animation **) malloc(animcount * sizeof(Animation *) );
		for(int j=0;j<animcount;j++) {
			animation[j]=GetAnimationPiece(af, j);
		}
	} else {
		animcount = 1;
		animation = (Animation **) malloc( sizeof(Animation *) );
		animation[0]=GetAnimationPiece(af, sequence);
	}
	if (Flags & A_ANI_PALETTE) {
		SetPalette(PaletteRef);
	}
	if (Flags&A_ANI_BLEND) {
		BlendAnimation();
	}
}

void AreaAnimation::SetPalette(ieResRef Pal)
{
	Flags |= A_ANI_PALETTE;
	gamedata->FreePalette(palette, PaletteRef);
	strnlwrcpy(PaletteRef, Pal, 8);
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

		if (animcount == 0 || !animation[0]) return;
		Sprite2D* spr = animation[0]->GetFrame(0);
		if (!spr) return;
		palette = spr->GetPalette()->Copy();
		PaletteRef[0] = 0;
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
	if (Flags&A_ANI_BACKGROUND) return ANI_PRI_BACKGROUND;
	if (core->HasFeature(GF_IMPLICIT_AREAANIM_BACKGROUND) && height <= 0)
		return ANI_PRI_BACKGROUND;
	return Pos.y+height;
}


void AreaAnimation::Draw(const Region &screen, Map *area)
{
	Video* video = core->GetVideoDriver();
	Game *game = core->GetGame();

	//always draw the animation tinted because tint is also used for
	//transparency
	ieByte inverseTransparency = 255-transparency;
	Color tint = {255,255,255,inverseTransparency};
	if (Flags & A_ANI_NO_SHADOW) {
		tint = area->LightMap->GetPixel( Pos.x / 16, Pos.y / 12);
		tint.a = inverseTransparency;
	}
	ieDword flags = BLIT_TINTED;
	if (game) game->ApplyGlobalTint(tint, flags);
	bool covered = true;

	// TODO: This needs more testing. The HOW ar9101 roast seems to need it.
	// The conditional on height<=0 is unverified.
	if (core->HasFeature(GF_IMPLICIT_AREAANIM_BACKGROUND) && height <= 0)
		covered = false;

	if (Flags&A_ANI_NO_WALL)
		covered = false;

	if (covered && !covers) {
		covers=(SpriteCover **) calloc( animcount, sizeof(SpriteCover *) );
	}

	int ac = animcount;
	while (ac--) {
		Animation *anim = animation[ac];
		Sprite2D *frame = anim->NextFrame();
		if(covers) {
			if(!covers[ac] || !covers[ac]->Covers(Pos.x, Pos.y + height, frame->XPos, frame->YPos, frame->Width, frame->Height)) {
				delete covers[ac];
				covers[ac] = area->BuildSpriteCover(Pos.x, Pos.y + height, -anim->animArea.x,
					-anim->animArea.y, anim->animArea.w, anim->animArea.h, 0, true);
			}
		}
		video->BlitGameSprite( frame, Pos.x + screen.x, Pos.y + screen.y,
			flags, tint, covers?covers[ac]:0, palette, &screen );
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

	PluginHolder<MapMgr> mM(IE_ARE_CLASS_ID);
	//no need to open and read the .are file again
	//using the ARE class for this because ChangeMap is similar to LoadMap
	//it loads the lightmap and the minimap too, besides swapping the tileset
	if (!mM->ChangeMap(this, day_or_night) && !day_or_night) {
		Log(WARNING, "Map", "Invalid night lightmap, falling back to day lightmap.");
		mM->ChangeMap(this, 1);
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

short unsigned int Map::GetInternalSearchMap(int x, int y) const
{
	if ((unsigned)x >= Width || (unsigned)y >= Height) {
		return 0;
	}
	return SrchMap[x+y*Width];
}

void Map::SetInternalSearchMap(int x, int y, int value)
{
	if ((unsigned)x >= Width || (unsigned)y >= Height) {
		return;
	}
	SrchMap[x+y*Width] = value;
}

void Map::SetBackground(const ieResRef &bgResRef, ieDword duration)
{
	ResourceHolder<ImageMgr> bmp = GetResourceHolder<ImageMgr>(bgResRef);

	if (Background) {
		Sprite2D::FreeSprite(Background);
	}
	Background = bmp->GetSprite2D();
	BgDuration = duration;
}

void Map::SetupReverbInfo() {
	if (!reverb) {
		reverb = new MapReverb(*this);
	}
}

}
