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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Map.cpp,v 1.193 2005/11/13 20:26:22 avenger_teambg Exp $
 *
 */

#include "../../includes/win32def.h"
#include "Map.h"
#include "Interface.h"
#include "PathFinder.h"
#include "Ambient.h"
#include "../../includes/strrefs.h"
#include "AmbientMgr.h"

#ifndef WIN32
#include <sys/time.h>
#else
extern HANDLE hConsole;
#endif

extern Interface* core;
#ifdef WIN32
extern HANDLE hConsole;
#endif

static int MaxVisibility = 30;
static int Perimeter; //calculated from MaxVisibility
static int NormalCost = 10;
static int AdditionalCost = 4;
static int Passable[16] = {
	4, 1, 1, 1, 1, 1, 1, 1, 0, 1, 8, 0, 0, 0, 3, 1
};
static Point **VisibilityMasks=NULL;

static bool PathFinderInited = false;
static Variables Spawns;
int LargeFog;
static ieWord globalActorCounter;

#define STEP_TIME 150

void Map::ReleaseMemory()
{
	if (VisibilityMasks) {
		for (int i=0;i<MaxVisibility;i++) {
			free(VisibilityMasks[i]);
		}
	}
}

//returns true if creature must be embedded in the area
static bool MustSave(Actor *actor)
{
	if (actor->InParty) {
		return false;
	}
	//check for familiars, summons?
	return true;
}

void InitSpawnGroups()
{
	ieResRef GroupName;
	int i;
	TableMgr * tab;

	int table=core->LoadTable( "spawngrp" );

	Spawns.RemoveAll();
	Spawns.SetType( GEM_VARIABLES_STRING );
	
	if (table<0) {
		return;
	}
	tab = core->GetTable( table );
	if (!tab) {
		goto end;
	}
	i=tab->GetColNamesCount();
	while (i--) {
		int j=tab->GetRowCount();
		while (j--) {
			char *crename = tab->QueryField( j,i );
			if (crename[0] != '*') break;
		}
		if (j>0) {
			ieResRef *creatures = (ieResRef *) malloc( sizeof(ieResRef)*(j+1) );
			//count of creatures
			*(ieDword *) creatures = (ieDword) j;
			//difficulty
			*(((ieDword *) creatures)+1) = (ieDword) atoi( tab->QueryField(i,0) );
			for (;j;j--) {
				strnuprcpy( creatures[j], tab->QueryField(j,i), sizeof( ieResRef ) );
			}
			strnuprcpy( GroupName, tab->GetColumnName( i ), 8);
			Spawns.SetAt( GroupName, (const char *) creatures );
		}
	}
end:
	core->DelTable( table );
}

void InitPathFinder()
{
	PathFinderInited = true;
	int passabletable = core->LoadTable( "pathfind" );
	if (passabletable >= 0) {
		TableMgr* tm = core->GetTable( passabletable );
		if (tm) {
			char* poi;

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
			core->DelTable( passabletable );
		}
	}
}

void AddLOS(int destx, int desty, int slot)
{
	for (int i=0;i<MaxVisibility;i++) {
		int x=(destx*i+MaxVisibility/2)/MaxVisibility*16;
		int y=(desty*i+MaxVisibility/2)/MaxVisibility*16;
		if (LargeFog) {
			x += 16;
			y += 16;
		}
		VisibilityMasks[i][slot].x=x;
		VisibilityMasks[i][slot].y=y;
	}
}

void InitExplore()
{
	LargeFog = !core->HasFeature(GF_SMALL_FOG);

	//circle perimeter size for MaxVisibility
	int x = MaxVisibility;
	int y = 0;
	int xc = 1 - ( 2 * MaxVisibility );
	int yc = 1;
	int re = 0;
	Perimeter = 0;
	while (x>=y) {
		Perimeter+=8;
		y++;
		re += yc;
		yc += 2;
		if (( ( 2 * re ) + xc ) > 0) {
			x--;
			re += xc;
			xc += 2;
		}
	}

	int i;
	VisibilityMasks = (Point **) malloc(MaxVisibility * sizeof(Point *) );
	for (i=0;i<MaxVisibility;i++) {
		VisibilityMasks[i] = (Point *) malloc(Perimeter*sizeof(Point) );
	}

	x = MaxVisibility;
	y = 0;
	xc = 1 - ( 2 * MaxVisibility );
	yc = 1;
	re = 0;
	Perimeter = 0;
	while (x>=y) {
		AddLOS (x, y, Perimeter++);
		AddLOS (-x, y, Perimeter++);
		AddLOS (-x, -y, Perimeter++);
		AddLOS (x, -y, Perimeter++);
		AddLOS (y, x, Perimeter++);
		AddLOS (-y, x, Perimeter++);
		AddLOS (-y, -x, Perimeter++);
		AddLOS (y, -x, Perimeter++);
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
	//vars = NULL;
	TMap = NULL;
	LightMap = NULL;
	SearchMap = NULL;
	SmallMap = NULL;
	MapSet = NULL;
	Walls = NULL;
	WallCount = 0;
	queue[0] = NULL;
	queue[1] = NULL;
	queue[2] = NULL;
	Qcount[0] = 0;
	Qcount[1] = 0;
	Qcount[2] = 0;
	lastActorCount[0] = 0;
	lastActorCount[1] = 0;
	lastActorCount[2] = 0;
	if (!PathFinderInited) {
		InitPathFinder();
		InitSpawnGroups();
		InitExplore();
		globalActorCounter = 0;
	}
	ExploredBitmap = NULL;
	VisibleBitmap = NULL;
	version = 0;
	localActorCounter = 0;
}

Map::~Map(void)
{
	unsigned int i;

	if (MapSet) {
		free( MapSet );
	}
	if (TMap) {
		delete TMap;
	}
	for (i = 0; i < animations.size(); i++) {
		delete animations[i];
	}

	for (i = 0; i < actors.size(); i++) {
		Actor* a = actors[i];
		//don't delete NPC/PC 
		if (a && !a->Persistent() ) {
			delete a;
		}
	}

	for (i = 0; i < entrances.size(); i++) {
		delete( entrances[i] );
	}
	if (LightMap)
		core->FreeInterface( LightMap );
	if (SearchMap)
		core->FreeInterface( SearchMap );
	if (SmallMap)
		core->FreeInterface( SmallMap );
	for (i = 0; i < 3; i++) {
		if (queue[i]) {
			free(queue[i]);
			queue[i] = NULL;
		}
	}
	for (i = 0; i < vvcCells.size(); i++) {
		if (vvcCells[i]) {
			delete vvcCells[i];
			vvcCells[i] = NULL;
		}
	}
	for (i = 0; i < ambients.size(); i++) {
		delete ambients[i];
	}
	for (i = 0; i < mapnotes.size(); i++) {
		delete mapnotes[i];
	}

	//malloc-d in AREImp
	if (ExploredBitmap) {
		free( ExploredBitmap );
	}
	if (VisibleBitmap) {
		free( VisibleBitmap );
	}
	if (Walls) {
		for(i=0;i<WallCount;i++) {
			delete(Walls[i]);
		}
		free( Walls );
	}
	WallCount=0;
}

void Map::AddTileMap(TileMap* tm, ImageMgr* lm, ImageMgr* sr, ImageMgr* sm)
{
	TMap = tm;
	LightMap = lm;
	SearchMap = sr;
	SmallMap = sm;
	Width=TMap->XCellCount * 4;
	Height=( TMap->YCellCount * 64 ) / 12;
	//Filling Matrices
	MapSet = (unsigned short *) malloc(sizeof(unsigned short) * Width * Height);
}

/* this command will load the target area and set the coordinates according to the entrance string*/
void Map::CreateMovement(char *command, const char *area, const char *entrance)
{
//change loader MOS image here
//check worldmap entry, if that doesn't contain anything,
//make a random pick
	Game* game = core->GetGame();
	Map* map = game->GetMap(area, false);
	if (!map) {
		printf("[Map] Invalid map: %s\n",area);
		command[0]=0;
		return;
	}
	Entrance* ent = map->GetEntrance( entrance );
	int X,Y, face;
	if (!ent) {
		textcolor( YELLOW );
		printf( "[Map] WARNING!!! %s EntryPoint does not Exists\n", entrance );
		textcolor( WHITE );
		X = map->TMap->XCellCount * 64;
		Y = map->TMap->YCellCount * 64;
		face = -1;
	} else {
		X = ent->Pos.x;
		Y = ent->Pos.y;
		face = ent->Face;
	}
	//LeaveArea is the same in ALL engine versions
	sprintf(command, "LeaveArea(\"%s\",[%d.%d],%d)", area, X, Y, face);
}

void Map::UseExit(Actor *actor, InfoPoint *ip)
{
	char Tmp[256];
	Game *game=core->GetGame();

	int EveryOne = ip->CheckTravel(actor);
	switch(EveryOne) {
	case CT_GO_CLOSER:
		core->DisplayConstantString(STR_WHOLEPARTY,0xffffff); //white
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

	ip->Flags&=~TRAP_RESET; //exit triggered
	if (ip->Destination[0] != 0) {
		CreateMovement(Tmp, ip->Destination, ip->EntranceName);
		if (EveryOne&CT_GO_CLOSER) {
			int i=game->GetPartySize(false);
			while (i--) {
				Actor *pc = game->GetPC(i,false);

				pc->ClearPath();
				pc->ClearActions();
				pc->AddAction( GenerateAction( Tmp ) );
			}
			return;
		}
		if (EveryOne&CT_SELECTED) {
			int i=game->GetPartySize(false);
			while (i--) {
				Actor *pc = game->GetPC(i,false);
				
				if (!pc->IsSelected()) continue;
				pc->ClearPath();
				pc->ClearActions();
				pc->AddAction( GenerateAction( Tmp ) );
			}
		}

		actor->ClearPath();
		actor->ClearActions();
		actor->AddAction( GenerateAction( Tmp ) );
	} else {
		if (ip->Scripts[0]) {
			ip->LastTrigger = ip->LastEntered = actor->GetID();
			ip->ExecuteScript( ip->Scripts[0] );
			ip->ProcessActions();
		}
	}
}

void Map::UpdateScripts()
{
	//Run the Global Script
	Game* game = core->GetGame();
	game->ExecuteScript( game->Scripts[0] );
	game->ProcessActions();
	//Run the Map Script
	if (Scripts[0]) {
		ExecuteScript( Scripts[0] );
	}
	//Execute Pending Actions
	ProcessActions();

	//Run actor scripts (only for 0 priority)
	int q=Qcount[0];

	while (q--) {
		Actor* actor = queue[0][q];
		for (unsigned int i = 0; i < 8; i++) {
			if (actor->Scripts[i]) {
				if (actor->GetNextAction())
					break;
				actor->ExecuteScript( actor->Scripts[i] );
			}
		}
		actor->ProcessActions();

		//returns true if actor should be completely removed
		actor->inventory.CalculateWeight();
		actor->SetStat( IE_ENCUMBRANCE, actor->inventory.GetWeight() );
		actor->DoStep( );
	}
	//Check if we need to start some trigger scripts
	int ipCount = 0;
	while (true) {
		//For each InfoPoint in the map
		InfoPoint* ip = TMap->GetInfoPoint( ipCount++ );
		if (!ip)
			break;
		if (!(ip->Active&SCR_ACTIVE) )
			continue;
		//If this InfoPoint has no script and it is not a Travel Trigger, skip it
		if (!ip->Scripts[0] && ( ip->Type != ST_TRAVEL ))
			continue;
		//If this InfoPoint is a Switch Trigger
		if (ip->Type == ST_TRIGGER) {
			//Check if this InfoPoint was activated
			if (ip->LastTrigger) {
				//Run the InfoPoint script
				ip->ExecuteScript( ip->Scripts[0] );
				//Execute Pending Actions
				ip->ProcessActions();
			}
			continue;
		}
		
		q=Qcount[0];
		while (q--) {
			Actor* actor = queue[0][q];
			if ((ip->Type == ST_PROXIMITY) && !(ip->Flags&TRAP_DEACTIVATED) ) {
				if (ip->outline->PointIn( actor->Pos )) {
					ip->Entered(actor);
				}
				ip->ExecuteScript( ip->Scripts[0] );
				//Execute Pending Actions
				ip->ProcessActions();
			} else {
				//ST_TRAVEL
				//don't move if doing something else
				if (actor->GetNextAction())
					continue;
				if (ip->outline->PointIn( actor->Pos )) {
					UseExit(actor, ip);
				}
			}
		}
	}
}

/* Handling automatic stance changes */
bool Map::HandleActorStance(Actor *actor, CharAnimations *ca, int StanceID)
{
	int x = rand()%1000;
	if (ca->autoSwitchOnEnd) {
		actor->SetStance( ca->nextStanceID );
		return true;
	}
	if ((StanceID==IE_ANI_AWAKE) && !x ) {
		actor->SetStance( IE_ANI_HEAD_TURN );
		return true;
	}
	if ((StanceID==IE_ANI_READY) && !actor->GetNextAction()) {
		if (!actor->GetNextAction()) {
			actor->SetStance( IE_ANI_AWAKE );
			return true;
		}
	}
	return false;
}

void Map::DrawContainers( Region screen, Container *overContainer)
{
	Region vp = core->GetVideoDriver()->GetViewport();
	unsigned int i = 0;
	Container *c;

	while ( (c = TMap->GetContainer(i++))!=NULL ) {
		Color tint = LightMap->GetPixel( c->Pos.x / 16, c->Pos.y / 12);
		tint.a = 255;

		if ((c != overContainer) && (c->Type==IE_CONTAINER_PILE) ) {
			if (c->outline->BBox.InsideRegion( vp )) {
				c->DrawPile(false, screen, tint);
			}
		}
	}
	//draw overcontainer with highlight
	if (overContainer) {
		if (overContainer->Type==IE_CONTAINER_PILE) {
			Color tint = LightMap->GetPixel( overContainer->Pos.x / 16, overContainer->Pos.y / 12);
			tint.a = 255;
			overContainer->DrawPile(true, screen, tint);
		} else {
			overContainer->DrawOutline();
		}
	}
}

void Map::DrawMap(Region screen, GameControl* gc)
{
	unsigned int i;
	//Draw the Map

	if (!TMap) {
		return;
	}
	ieDword gametime = core->GetGame()->GameTime;

	TMap->DrawOverlay( 0, screen );
	//Blit the Background Map Animations (before actors)
	Video* video = core->GetVideoDriver();
	for (i = 0; i < animations.size(); i++) {
		AreaAnimation *a = animations[i];
		int animcount=a->animcount;
		
		if (!(a->Flags&A_ANI_BACKGROUND)) continue; //these are drawn after actors
		if (!a->Schedule(gametime)) continue;
		
		if (!IsVisible( a->Pos, !(a->Flags & A_ANI_NOT_IN_FOG)) )
			continue;
		//maybe we should divide only by 128, so brightening is possible too? In that case use 128,128,128 here
		Color tint = {255,255,255,(ieByte) a->transparency};
		if ((a->Flags&A_ANI_NO_SHADOW)) {
			tint = LightMap->GetPixel( a->Pos.x / 16, a->Pos.y / 12);
		}
		if (a->Flags&A_ANI_BLEND) {
			tint.a = 0xa0;
		} else {
			tint.a = 255;
		}
		while (animcount--) {
			Animation *anim = a->animation[animcount];
			video->BlitSpriteTinted( anim->NextFrame(),
				a->Pos.x + screen.x, a->Pos.y + screen.y,
				tint, anim->Palette, &screen );		
		}
	}
	//Draw Selected Door Outline
	if (gc->overDoor) {
		gc->overDoor->DrawOutline();
	}
	DrawContainers( screen, gc->overContainer );
	Region vp = video->GetViewport();
	// starting with lowest priority (so they are drawn over)
	GenerateQueues();
	int q = 2; //skip inactive actors, don't even sort them
	while (q--) {
		int index = Qcount[q];
		while (true) {
			Actor* actor = GetRoot( q, index );
			if (!actor)
				break;
			int cx = actor->Pos.x;
			int cy = actor->Pos.y;
			int explored = actor->Modified[IE_DONOTJUMP]&2;
			//check the deactivation condition only if needed
			//this fixes dead actors disappearing from fog of war (they should be permanently visible)
			if (!IsVisible( actor->Pos, explored) && (actor->Active&SCR_ACTIVE) ) {
				//finding an excuse why we don't hybernate the actor
				if (actor->Modified[IE_ENABLEOFFSCREENAI])
					continue;
				if (actor->CurrentAction)
					continue;
				if (actor->path)
					continue;
				if (actor->GetNextAction())
					continue;
				if (actor->GetWait()) //would never stop waiting
					continue;
				//turning actor inactive
				actor->Active&=~SCR_ACTIVE;
			}
			//visual feedback
			CharAnimations* ca = actor->GetAnims();
			if (!ca) {
				continue;
			}
			//explored or visibilitymap (bird animations are visible in fog)
			//0 means opaque
			int Trans = actor->Modified[IE_TRANSLUCENT];
			//int Trans = actor->Modified[IE_TRANSLUCENT] * 255 / 100;
			if (Trans>255) {
				 Trans=255;
			}
			int State = actor->Modified[IE_STATE_ID];
			if (State&STATE_INVISIBLE) {
				//enemies/neutrals are fully invisible if invis flag 2 set
				if (actor->Modified[IE_EA]>EA_GOODCUTOFF) {
					if (State&STATE_INVIS2)
						Trans=256;
					else
						Trans=128;
				} else {
					Trans=256;
				}
			}
			//friendlies are half transparent at best
			if (Trans>128) {
				if (actor->Modified[IE_EA]<=EA_GOODCUTOFF) {
					Trans=128;
				}
			}
			//no visual feedback
			if (Trans>255) {
				continue;
			}
			if (( !actor->Modified[IE_NOCIRCLE] ) &&
					( !( State & STATE_DEAD ) )) {
				actor->DrawCircle(vp);
				actor->DrawTargetPoint(vp);
			}

			unsigned char StanceID = actor->GetStance();
			Animation* anim = ca->GetAnimation( StanceID, actor->GetNextFace() );
			if (anim) {
				Sprite2D* nextFrame = anim->NextFrame();
				if (nextFrame) {
					if (actor->lastFrame != nextFrame) {
						Region newBBox;
						newBBox.x = cx - nextFrame->XPos;
						newBBox.w = nextFrame->Width;
						newBBox.y = cy - nextFrame->YPos;
						newBBox.h = nextFrame->Height;
						actor->lastFrame = nextFrame;
						actor->SetBBox( newBBox );
					}
					if (actor->BBox.InsideRegion( vp )) {
						Color tint = LightMap->GetPixel( cx / 16, cy / 12);
						tint.a = 255-Trans;
						video->BlitSpriteTinted( nextFrame, cx + screen.x, cy + screen.y, tint, anim->Palette, &screen );
					}
					if (anim->endReached) {
						if (HandleActorStance(actor, ca, StanceID) ) {
							anim->endReached = false;
						}
					}
				}
			}

			//text feedback
			actor->DrawOverheadText(screen);
		}
	}

	//draw normal animations after actors
	for (i = 0; i < animations.size(); i++) {
		AreaAnimation *a = animations[i];
		int animcount=a->animcount;
		
		if (a->Flags&A_ANI_BACKGROUND) continue; //these are drawn before actors
		if (!a->Schedule(gametime)) continue;

		if (!IsVisible( a->Pos, !(a->Flags & A_ANI_NOT_IN_FOG)) )
			continue;
		Color tint = {255,255,255,(ieByte) a->transparency};
		if ((a->Flags&A_ANI_NO_SHADOW)) {
			tint = LightMap->GetPixel( a->Pos.x / 16, a->Pos.y / 12);
		}
		if (a->Flags&A_ANI_BLEND) {
			tint.a = 0xa0;
		} else {
			tint.a = 255;
		}
		while (animcount--) {
			Animation *anim = a->animation[animcount];
			video->BlitSpriteTinted( anim->NextFrame(),
				a->Pos.x + screen.x, a->Pos.y + screen.y,
				tint, anim->Palette, &screen );
		}
	}

	for (i = 0; i < vvcCells.size(); i++) {
		ScriptedAnimation* vvc = vvcCells.at( i );
		if (!vvc)
			continue;
		if (!vvc->anims[0])
			continue;
		if (vvc->anims[0]->endReached) {
			vvcCells[i] = NULL;
			delete( vvc );
			continue;
		}
		if (vvc->justCreated) {
			vvc->justCreated = false;
			if (vvc->Sounds[0][0] != 0) {
				core->GetSoundMgr()->Play( vvc->Sounds[0] );
			}
		}
		Sprite2D* frame = vvc->anims[0]->NextFrame();
		if (!frame)
			continue;
		if (vvc->Transparency & IE_VVC_BRIGHTEST) {
			video->BlitSprite( frame, vvc->XPos + screen.x,
					vvc->YPos + screen.y, false, &screen );
		} else {
			video->BlitSprite( frame, vvc->XPos + screen.x,
					vvc->YPos + screen.y, false, &screen );
		}
	}

	if (core->FogOfWar && TMap) {
		TMap->DrawFogOfWar( ExploredBitmap, VisibleBitmap, screen );
	}
}

void Map::AddAnimation(AreaAnimation* anim)
{
	animations.push_back( anim );
}

//reapplying all of the effects on the actors of this map
//this might be unnecessary later
void Map::UpdateEffects()
{
	unsigned int i = actors.size();
	while (i--) {
		actors[i]->Init();
	}
}

void Map::Shout(Actor* actor, int shoutID, unsigned int radius)
{
	int i=actors.size();
	while (i--) {
		if (radius) {
			if (Distance(actor->Pos, actors[i]->Pos)>radius) {
				continue;
			}
		}
		if (shoutID) {
			actors[i]->LastHeard = actor->GetID();
			actors[i]->LastShout = shoutID;
		} else {
			actors[i]->LastHelp = actor->GetID();
		}
	}
}

void Map::AddActor(Actor* actor)
{
	//setting the current area for the actor as this one
	strnuprcpy(actor->Area, scriptName, 8);
	//0 is reserved for 'no actor'
	actor->SetMap(this, ++localActorCounter, ++globalActorCounter);
	actors.push_back( actor );
	//if a visible aggressive actor was put on the map, it is an autopause reason
	//guess game is always loaded? if not, then we'll crash
	ieDword gametime = core->GetGame()->GameTime;

	if (IsVisible(actor->Pos, false) && actor->Schedule(gametime) ) {
       		if (actor->Modified[IE_EA]>=EA_EVILCUTOFF) {
			core->Autopause(AP_ENEMY);
		}
	}
}

void Map::DeleteActor(int i)
{
	Actor *actor = actors[i];
	Game *game = core->GetGame();
	game->LeaveParty( actor );
	game->DelNPC( game->InStore(actor) );
	actors.erase( actors.begin()+i );
	delete (actor);
}

Actor* Map::GetActorByGlobalID(ieDword objectID)
{
	if (!objectID) {
		return NULL;
	}
	//truncation is intentional
	ieWord globalID = (ieWord) objectID;
	unsigned int i = actors.size();
	while (i--) {
		Actor* actor = actors[i];
		
		if (actor->globalID==globalID) {
			return actor;
		}
	}
	return NULL;
}

/** flags:
 GA_SELECT  1 - unselectable actors don't play
 GA_NO_DEAD 2 - dead actors don't play
*/
Actor* Map::GetActor(Point &p, int flags)
{
	unsigned int i = actors.size();
	while (i--) {
		Actor* actor = actors[i];
		
		if (!actor->ValidTarget(flags) ) {
			continue; 
		}
		if (actor->IsOver( p ))
			return actor;
	}
	return NULL;
}

Actor* Map::GetActor(const char* Name)
{
	unsigned int i = actors.size();
	while (i--) {
		Actor* actor = actors[i];
		if (strnicmp( actor->GetScriptName(), Name, 32 ) == 0) {
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
	unsigned int i=actors.size();
	while(i--) {
		if (MustSave(actors[i])) {
			ret++;
		}
	}
	return ret;
}

//before writing the area out, perform some cleanups
void Map::PurgeArea(bool items)
{
	//1. remove dead actors without 'keep corpse' flag
	unsigned int i=actors.size();
	while(i--) {
		Actor *ac = actors[i];

		if (ac->GetStat(IE_STATE_ID)&STATE_NOSAVE) {
			if (ac->GetStat(IE_MC_FLAGS) & MC_KEEP_CORPSE) {
				continue;
			}
			delete ac;
			actors.erase( actors.begin()+i );
		}
	}
	//2. remove any non critical items
	if (items) {
		unsigned int i=TMap->GetContainerCount();
		while(i--) {
			Container *c = TMap->GetContainer(i);
			unsigned int j=c->inventory.GetSlotCount();
			while(j--) {
				CREItem *itemslot = c->inventory.GetSlotItem(j);
				if (itemslot->Flags&IE_INV_ITEM_CRITICAL) {
					continue;
				}
			}
			TMap->CleanupContainer(c);
		}
	}
}

Actor* Map::GetActor(int index, bool any)
{
	if(any) {
		return actors[index];
	}
	unsigned int i=0;
	while(i<actors.size() ) {
		Actor *ac = actors[i++];
		if (MustSave(ac) ) {
			if (!index--) {
				return ac;
			}
		}
	}
	return NULL;
}

Actor* Map::GetActorByDialog(const char *resref)
{
	unsigned int i = actors.size();
	while (i--) {
		Actor* actor = actors[i];
		if (strnicmp( actor->Dialog, resref, 8 ) == 0) {
			return actor;
		}
	}
	return NULL;
}

int Map::GetActorInRect(Actor**& actorlist, Region& rgn, bool onlyparty)
{
	actorlist = ( Actor * * ) malloc( actors.size() * sizeof( Actor * ) );
	int count = 0;
	unsigned int i = actors.size();
	while (i--) {
		Actor* actor = actors[i];
//use this function only for party?
		if (onlyparty && !actor->InParty)
			continue;
		if (!actor->ValidTarget(GA_SELECT|GA_NO_DEAD) )
			continue;
		if (( actor->BBox.x > ( rgn.x + rgn.w ) ) ||
			( actor->BBox.y > ( rgn.y + rgn.h ) ))
			continue;
		if (( ( actor->BBox.x + actor->BBox.w ) < rgn.x ) ||
			( ( actor->BBox.y + actor->BBox.h ) < rgn.y ))
			continue;
		actorlist[count++] = actor;
	}
	actorlist = ( Actor * * ) realloc( actorlist, count * sizeof( Actor * ) );
	return count;
}

void Map::PlayAreaSong(int SongType)
{
	//you can speed this up by loading the songlist once at startup
	int column;
	const char* tablename;

	if (core->HasFeature( GF_HAS_SONGLIST )) {
		column = 1;
		tablename = "songlist";
	} else {
		/*since bg1 and pst has no .2da for songlist, 
		we must supply one in the gemrb/override folder.
		It should be: music.2da, first column is a .mus filename
		*/
		column = 0;
		tablename = "music";
	}
	int songlist = core->LoadTable( tablename );
	if (songlist < 0) {
		return;
	}
	TableMgr* tm = core->GetTable( songlist );
	if (!tm) {
		core->DelTable( songlist );
		return;
	}
	char* poi = tm->QueryField( SongHeader.SongList[SongType], column );
	core->GetMusicMgr()->SwitchPlayList( poi, true );
}

int Map::GetBlocked(Point &c)
{
	int block = SearchMap->GetPixelIndex( c.x / 16, c.y / 12 );
	return Passable[block];
}
/* i hate vectors, they are slow
void Map::AddWallGroup(WallGroup* wg)
{
	wallGroups.push_back( wg );
}
*/
//this function determines actor drawing order
//it should be extended to wallgroups, animations, effects!
void Map::GenerateQueues()
{
	int priority;

	for (priority=0;priority<3;priority++) {
		if (lastActorCount[priority] != actors.size()) {
			if (queue[priority]) {
				free(queue[priority]);
				queue[priority] = NULL;
			}
			queue[priority] = (Actor **) calloc( actors.size(), sizeof(Actor *) );
			lastActorCount[priority] = ( int ) actors.size();
		}
		Qcount[priority] = 0;
	}
	unsigned int i=actors.size();
	while (i--) {
		Actor* actor = actors[i];

		if (actor->CheckOnDeath()) {
			DeleteActor( i );
			continue;
		}

		ieDword gametime = core->GetGame()->GameTime;

		if (actor->Active&SCR_ACTIVE) {
			if ((actor->GetStance() == IE_ANI_TWITCH) && !actor->playDeadCounter) {
				priority = 1; //display
			} else {
				priority = 0; //run scripts and display
			}
		} else {
			//dead actors are always visible on the map, but run no scripts
			if (actor->GetStance() == IE_ANI_TWITCH) {
				priority = 1;
			} else {
				//isvisible flag is false (visibilitymap) here,
				//coz we want to reactivate creatures that
				//just became visible
				if (IsVisible(actor->Pos, false) && actor->Schedule(gametime) ) {
					priority = 0; //run scripts and display, activated now
					actor->Active|=SCR_ACTIVE;
					if (actor->Modified[IE_EA]>=EA_EVILCUTOFF) {
						core->Autopause(AP_ENEMY);
					}
					//here you can flag for autopause if actor->Modified[IE_EA] is enemy, coz we just revealed it!
				} else {
					priority = 2;
				}
			}
		}

		queue[priority][Qcount[priority]] = actor;
		Qcount[priority]++;
		int lastPos = Qcount[priority];
		while (lastPos != 1) {
			int parentPos = ( lastPos / 2 ) - 1;
			Actor* parent = queue[priority][parentPos];
			if (actor->Pos.y < parent->Pos.y) {
				queue[priority][parentPos] = actor;
				queue[priority][lastPos - 1] = parent;
				lastPos = parentPos + 1;
			} else
				break;
		}
	}
}

Actor* Map::GetRoot(int priority, int &index)
{
	if (index == 0) {
		return NULL;
	}

	Actor* ret = queue[priority][Qcount[priority]-index];
	index--;
	if (index == 0) {
		return ret;
	}
	Actor **baseline=queue[priority]+(Qcount[priority]-index);
	int lastPos = 1;
	Actor* node = baseline[0];
	while (true) {
		int leftChildPos = ( lastPos*2 ) - 1;
		int rightChildPos = lastPos*2;
		if (leftChildPos >= index)
			break;
		Actor* child = baseline[leftChildPos];
		int childPos = leftChildPos;
		if (rightChildPos < index) {
			//If both Child Exist
			Actor* rightChild = baseline[rightChildPos];
			if (rightChild->Pos.y < child->Pos.y) {
				childPos = rightChildPos;
				child = rightChild;
			}
		}
		if (node->Pos.y > child->Pos.y) {
			baseline[lastPos-1] = child;
			baseline[childPos] = node;
			lastPos = childPos + 1;
		} else
			break;
	}
	return ret;
}

void Map::AddVVCCell(ScriptedAnimation* vvc)
{
	unsigned int i=vvcCells.size();
	while (i--) {
		if (vvcCells[i] == NULL) {
			vvcCells[i] = vvc;
			return;
		}
	}
	vvcCells.push_back( vvc );
}

AreaAnimation* Map::GetAnimation(const char* Name)
{
	unsigned int i=animations.size();
	while (i--) {
		AreaAnimation *anim = animations[i];

		if (anim->Name && (strnicmp( anim->Name, Name, 32 ) == 0)) {
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
	sp->Pos.x = XPos;
	sp->Pos.y = YPos;
	sp->Count = count;
	sp->Creatures = (ieResRef *) calloc( count, sizeof(ieResRef) );
	for( unsigned int i=0;i<count;i++) {
		strnuprcpy(sp->Creatures[i],creatures[i],8);
	}
	spawns.push_back( sp );
	return sp;
}

void Map::AddEntrance(char* Name, int XPos, int YPos, short Face)
{
	Entrance* ent = new Entrance();
	strncpy( ent->Name, Name, 32 );
	ent->Pos.x = XPos;
	ent->Pos.y = YPos;
	ent->Face = Face;
	entrances.push_back( ent );
}

Entrance* Map::GetEntrance(const char* Name)
{
	unsigned int i=entrances.size();
	while (i--) {
		Entrance *e = entrances[i];

		if (stricmp( e->Name, Name ) == 0) {
			return e;
		}
	}
	return NULL;
}

bool Map::HasActor(Actor *actor)
{
	unsigned int i=actors.size();
	while (i--) {
		if (actors[i] == actor) {
			return true;
		}
	}
	return false;
}

void Map::RemoveActor(Actor* actor)
{
	unsigned int i=actors.size();
	while (i--) {
		if (actors[i] == actor) {
			actors.erase( actors.begin()+i );
			return;
		}
	}
}

//returns true if none of the partymembers are on the map
bool Map::CanFree()
{
	unsigned int i=actors.size();
	while (i--) {
		if (actors[i]->InParty) {
			return false;
		}
	}
	//we expect the area to be swapped out, so we simply remove the corpses now
	PurgeArea(false);
	return true;
}

void Map::DebugDump()
{
	printf( "DebugDump of Area %s:\n", scriptName );
}

/******************************************************************************/

void Map::Leveldown(unsigned int px, unsigned int py,
	unsigned int& level, Point &n, unsigned int& diff)
{
	int pos;
	unsigned int nlevel;

	if (( px >= Width ) || ( py >= Height )) {
		return;
	} //walked off the map
	pos = py * Width + px;
	nlevel = MapSet[pos];
	if (!nlevel) {
		return;
	} //not even considered
	if (level <= nlevel) {
		return;
	}
	unsigned int ndiff = level - nlevel;
	if (ndiff > diff) {
		level = nlevel;
		diff = ndiff;
		n.x = px;
		n.y = py;
	}
}

void Map::SetupNode(unsigned int x, unsigned int y, unsigned int Cost)
{
	unsigned int pos;

	if (( x >= Width ) || ( y >= Height )) {
		return;
	}
	pos = y * Width + x;
	if (MapSet[pos]) {
		return;
	}
	if (!( Passable[SearchMap->GetPixelIndex( x, y )] & PATH_MAP_PASSABLE)) {
		MapSet[pos] = 65535;
		return;
	}
	MapSet[pos] = Cost;
	InternalStack.push( ( x << 16 ) | y );
}

void Map::AdjustPosition(Point &goal, unsigned int radius)
{
	unsigned int maxr = Width;
	if (maxr < Height) {
		maxr = Height;
	}
	if ((unsigned int) goal.x > Width) {
		goal.x = Width;
	}
	if ((unsigned int) goal.y > Height) {
		goal.y = Height;
	}
	for (; radius < maxr; radius++) {
		unsigned int minx = 0;
		if ((unsigned int) goal.x > radius)
			minx = goal.x - radius;
		unsigned int maxx = goal.x + radius + 1;
		if (maxx > Width)
			maxx = Width;

		for (unsigned int scanx = minx; scanx < maxx; scanx++) {
			if ((unsigned int) goal.y >= radius) {
				if (Passable[SearchMap->GetPixelIndex( scanx, goal.y - radius )] & PATH_MAP_PASSABLE) {
					goal.x = scanx;
					goal.y -= radius;
					return;
				}
			}
			if (goal.y + radius < Height) {
				if (Passable[SearchMap->GetPixelIndex( scanx, goal.y + radius )] & PATH_MAP_PASSABLE) {
					goal.x = scanx;
					goal.y += radius;
					return;
				}
			}
		}
		unsigned int miny = 0;
		if ((unsigned int) goal.y > radius)
			miny = goal.y - radius;
		unsigned int maxy = goal.y + radius + 1;
		if (maxy > Height)
			maxy = Height;
		for (unsigned int scany = miny; scany < maxy; scany++) {
			if ((unsigned int) goal.x >= radius) {
				if (Passable[SearchMap->GetPixelIndex( goal.x - radius, scany )] & PATH_MAP_PASSABLE) {
					goal.x -= radius;
					goal.y = scany;
					return;
				}
			}
			if (goal.x + radius < Width) {
				if (Passable[SearchMap->GetPixelIndex( goal.x - radius, scany )] & PATH_MAP_PASSABLE) {
					goal.x += radius;
					goal.y = scany;
					return;
				}
			}
		}
	}
}

void Map::FixAllPositions()
{
	for (unsigned int e = 0; e<actors.size(); e++) {
		Actor *actor = actors[e];
		if (actor->GetStat( IE_DONOTJUMP ) ) continue;
		Point p;
		p.x=actor->Pos.x/16;
		p.y=actor->Pos.y/12;
		AdjustPosition(p);
		actor->Pos.x=p.x*16+8;
		actor->Pos.y=p.y*12+6;
	}
}
//run away from dX, dY (ie.: find the best path of limited length that brings us the farthest from dX, dY)
PathNode* Map::RunAway(Point &s, Point &d, unsigned int PathLen, int flags)
{
	Point start(s.x/16, s.y/12);
	Point goal (d.x/16, d.y/12);
	unsigned int dist;

	memset( MapSet, 0, Width * Height * sizeof( unsigned short ) );
	while (InternalStack.size())
		InternalStack.pop();

	if (!( Passable[SearchMap->GetPixelIndex( start.x, start.y )] & PATH_MAP_PASSABLE )) {
		AdjustPosition( start );
	}
	unsigned int pos = ( start.x << 16 ) | start.y;
	InternalStack.push( pos );
	MapSet[start.y * Width + start.x] = 1;
	dist = 0;
	Point best = start;
	while (InternalStack.size()) {
		pos = InternalStack.front();
		InternalStack.pop();
		unsigned int x = pos >> 16;
		unsigned int y = pos & 0xffff;
		long tx = ( x - goal.x );
		long ty = ( y - goal.y );
		unsigned int distance = (unsigned int) sqrt( ( double ) ( tx* tx + ty* ty ) );
		if (dist<distance) {
			best.x=x;
			best.y=y;
			dist=distance;
		}

		unsigned int Cost = MapSet[y * Width + x] + NormalCost;
		if (Cost > PathLen) {
			//printf("Path not found!\n");
			break;
		}
		SetupNode( x - 1, y - 1, Cost );
		SetupNode( x + 1, y - 1, Cost );
		SetupNode( x + 1, y + 1, Cost );
		SetupNode( x - 1, y + 1, Cost );

		Cost += AdditionalCost;
		SetupNode( x, y - 1, Cost );
		SetupNode( x + 1, y, Cost );
		SetupNode( x, y + 1, Cost );
		SetupNode( x - 1, y, Cost );
	}

	//find path backwards from best to start
	PathNode* StartNode = new PathNode;
	PathNode* Return = StartNode;
	StartNode->Next = NULL;
	StartNode->Parent = NULL;
	StartNode->x = best.x;
	StartNode->y = best.y;
	if (flags) {
		StartNode->orient = GetOrient( start, best );
	} else {
		StartNode->orient = GetOrient( best, start );
	}
	Point p = best;
	unsigned int pos2 = start.y * Width + start.x;
	while (( pos = p.y * Width + p.x ) != pos2) {
		Return = new PathNode;
		StartNode->Parent = Return;
		Return->Next = StartNode;
		unsigned int level = MapSet[pos];
		unsigned int diff = 0;
		Point n;
		Leveldown( p.x, p.y + 1, level, n, diff );
		Leveldown( p.x + 1, p.y, level, n, diff );
		Leveldown( p.x - 1, p.y, level, n, diff );
		Leveldown( p.x, p.y - 1, level, n, diff );
		Leveldown( p.x - 1, p.y + 1, level, n, diff );
		Leveldown( p.x + 1, p.y + 1, level, n, diff );
		Leveldown( p.x + 1, p.y - 1, level, n, diff );
		Leveldown( p.x - 1, p.y - 1, level, n, diff );
		if (!diff)
			return Return;
		Return->x = n.x;
		Return->y = n.y;

		if (flags) {
			Return->orient = GetOrient( p, n );
		} else {
			Return->orient = GetOrient( n, p );
		}
		p = n;
	}
	return Return;
}

bool Map::TargetUnreachable(Point &s, Point &d)
{
	Point start( s.x/16, s.y/12 );
	Point goal ( d.x/16, d.y/12 );
	memset( MapSet, 0, Width * Height * sizeof( unsigned short ) );
	while (InternalStack.size())
		InternalStack.pop();

	if (!( Passable[SearchMap->GetPixelIndex( goal.x, goal.y )] & PATH_MAP_PASSABLE )) {
		return true;
	}
	if (!( Passable[SearchMap->GetPixelIndex( start.x, start.y )] & PATH_MAP_PASSABLE )) {
		return true;
	}

	unsigned int pos = ( goal.x << 16 ) | goal.y;
	unsigned int pos2 = ( start.x << 16 ) | start.y;
	InternalStack.push( pos );
	MapSet[goal.y * Width + goal.x] = 1;

	while (InternalStack.size() && pos!=pos2) {
		pos = InternalStack.front();
		InternalStack.pop();
		unsigned int x = pos >> 16;
		unsigned int y = pos & 0xffff;

		SetupNode( x - 1, y - 1, 1 );
		SetupNode( x + 1, y - 1, 1 );
		SetupNode( x + 1, y + 1, 1 );
		SetupNode( x - 1, y + 1, 1 );
		SetupNode( x, y - 1, 1 );
		SetupNode( x + 1, y, 1 );
		SetupNode( x, y + 1, 1 );
		SetupNode( x - 1, y, 1 );
	}
	return pos!=pos2;
}

/* Use this function when you target something by a straight line projectile (like a lightning bolt, arrow, etc)
*/
PathNode* Map::GetLine(Point &start, Point &dest, int flags)
{
	int Orientation = GetOrient(start, dest);
	int Steps = Distance(start, dest);

	return GetLine(start, dest, Steps, Orientation, flags);
}

PathNode* Map::GetLine(Point &start, int Steps, int Orientation, int flags)
{
	Point dest;

	//FIXME: calculate dest based on distance and orientation
	return GetLine(start, dest, Steps, Orientation, flags);
}

PathNode* Map::GetLine(Point &start, Point &dest, int Steps, int Orientation, int flags)
{
	PathNode* StartNode = new PathNode;
	PathNode *Return = StartNode;
	StartNode->Next = NULL;
	StartNode->Parent = NULL;
	StartNode->x = start.x;
	StartNode->y = start.y;
	StartNode->orient = Orientation;

	int Max = Steps;
	while(Steps--) {
		int x,y;

		StartNode->Next = new PathNode;
		StartNode->Next->Parent = StartNode;
		StartNode = StartNode->Next;
		StartNode->Next = NULL;
		x = (start.x + dest.x) * Steps / Max;
		y = (start.y + dest.y) * Steps / Max;
		StartNode->x = x;
		StartNode->y = y;
		StartNode->orient = Orientation;
		bool wall = (!( Passable[SearchMap->GetPixelIndex( x, y )] & PATH_MAP_PASSABLE ));
		if (wall) switch (flags) {
			case GL_REBOUND:
				Orientation = (Orientation + 8) &15;
				break;
			case GL_PASS:
				break;
			default: //premature end
				return Return;
		}
	}

	return Return;
}

PathNode* Map::FindPath(Point &s, Point &d, int MinDistance)
{
	Point start( s.x/16, s.y/12 );
	Point goal ( d.x/16, d.y/12 );
	memset( MapSet, 0, Width * Height * sizeof( unsigned short ) );
	while (InternalStack.size())
		InternalStack.pop();

	if (!( Passable[SearchMap->GetPixelIndex( goal.x, goal.y )] & PATH_MAP_PASSABLE )) {
		AdjustPosition( goal );
	}
	unsigned int pos = ( goal.x << 16 ) | goal.y;
	unsigned int pos2 = ( start.x << 16 ) | start.y;
	InternalStack.push( pos );
	MapSet[goal.y * Width + goal.x] = 1;

	while (InternalStack.size()) {
		pos = InternalStack.front();
		InternalStack.pop();
		unsigned int x = pos >> 16;
		unsigned int y = pos & 0xffff;

		if (pos == pos2) {
			//We've found _a_ path
			//printf("GOAL!!!\n");
			break;
		}
		unsigned int Cost = MapSet[y * Width + x] + NormalCost;
		if (Cost > 65500) {
			//printf("Path not found!\n");
			break;
		}
		SetupNode( x - 1, y - 1, Cost );
		SetupNode( x + 1, y - 1, Cost );
		SetupNode( x + 1, y + 1, Cost );
		SetupNode( x - 1, y + 1, Cost );

		Cost += AdditionalCost;
		SetupNode( x, y - 1, Cost );
		SetupNode( x + 1, y, Cost );
		SetupNode( x, y + 1, Cost );
		SetupNode( x - 1, y, Cost );
	}

	//find path from start to goal
	PathNode* StartNode = new PathNode;
	PathNode* Return = StartNode;
	StartNode->Next = NULL;
	StartNode->Parent = NULL;
	StartNode->x = start.x;
	StartNode->y = start.y;
	StartNode->orient = GetOrient( goal, start );
	if (pos != pos2) {
		return Return;
	}
	Point p = start;
	pos2 = goal.y * Width + goal.x;
	while (( pos = p.y * Width + p.x ) != pos2) {
		StartNode->Next = new PathNode;
		StartNode->Next->Parent = StartNode;
		StartNode = StartNode->Next;
		StartNode->Next = NULL;
		unsigned int level = MapSet[pos];
		unsigned int diff = 0;
		Point n;
		Leveldown( p.x, p.y + 1, level, n, diff );
		Leveldown( p.x + 1, p.y, level, n, diff );
		Leveldown( p.x - 1, p.y, level, n, diff );
		Leveldown( p.x, p.y - 1, level, n, diff );
		Leveldown( p.x - 1, p.y + 1, level, n, diff );
		Leveldown( p.x + 1, p.y + 1, level, n, diff );
		Leveldown( p.x + 1, p.y - 1, level, n, diff );
		Leveldown( p.x - 1, p.y - 1, level, n, diff );
		if (!diff)
			return Return;
		StartNode->x = n.x;
		StartNode->y = n.y;
		StartNode->orient = GetOrient( n, p );
		p = n;
	}
	//stepping back on the calculated path
	while (MinDistance-- && StartNode->Parent) {
		StartNode = StartNode->Parent;
		delete StartNode->Next;
		StartNode->Next = NULL;
	}
	return Return;
}

//single point visible or not (visible/exploredbitmap)
//if explored = true then visible in fog
bool Map::IsVisible(Point &pos, int explored)
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

//point a is visible from point b (searchmap)
bool Map::IsVisible(Point &s, Point &d)
{
	int sX=s.x/16;
	int sY=s.y/12;
	int dX=d.x/16;
	int dY=d.y/12;
	int diffx = sX - dX;
	int diffy = sY - dY;
	if (abs( diffx ) >= abs( diffy )) {
		//vertical
		double elevationy = fabs((double)diffx ) / diffy;
		if (sX > dX) {
			for (int startx = sX; startx > dX; startx--) {
				if (Passable[SearchMap->GetPixelIndex( startx, sY - ( int ) ( ( sX - startx ) / elevationy ) )] & PATH_MAP_NO_SEE)
					return false;
			}
		} else {
			for (int startx = sX; startx < dX; startx++) {
				if (Passable[SearchMap->GetPixelIndex( startx, sY + ( int ) ( ( sX - startx ) / elevationy ) )] & PATH_MAP_NO_SEE)
					return false;
			}
		}
	} else {
		double elevationx = fabs((double)diffy ) / diffx;
		if (sY > dY) {
			for (int starty = sY; starty > dY; starty--) {
				if (Passable[SearchMap->GetPixelIndex( sX - ( int ) ( ( sY - starty ) / elevationx ), starty )] & PATH_MAP_NO_SEE)
					return false;
			}
		} else {
			for (int starty = sY; starty < dY; starty++) {
				if (Passable[SearchMap->GetPixelIndex( sX + ( int ) ( ( sY - starty ) / elevationx ), starty )] & PATH_MAP_NO_SEE)
					return false;
			}
		}
	}
	return true;
}

//returns direction of area boundary, returns -1 if it isn't a boundary
int Map::WhichEdge(Point &s)
{
	unsigned int sX=s.x/16;
	unsigned int sY=s.y/12;
	if (!(Passable[SearchMap->GetPixelIndex( sX, sY )]&PATH_MAP_TRAVEL)) {
		printf("[Map] This isn't a travel region [%d.%d]?\n",sX, sY);
		return -1;
	}
	sX*=Height;
	sY*=Width;
	if (sX<sY) { //north or west
		if (Width*Height<sX+sY) { //
			return WMP_NORTH;
		}
		return WMP_WEST;
	}
	//south or east
	if (Width*Height<sX+sY) { //
		return WMP_SOUTH; 
	}
	return WMP_EAST;
}

//--------ambients----------------
void Map::SetupAmbients()
{
	AmbientMgr *ambim = core->GetSoundMgr()->GetAmbientMgr();
	ambim->reset();
	ambim->setAmbients( ambients );
}
//--------mapnotes----------------
//text must be a pointer we can claim ownership of
void Map::AddMapNote(Point &point, int color, char *text)
{
	MapNote *mn = new MapNote;

	mn->Pos=point;
	mn->color=color;
	mn->text=text;
	RemoveMapNote(point); //delete previous mapnote
	mapnotes.push_back(mn);
}

void Map::RemoveMapNote(Point &point)
{
	int i = mapnotes.size();
	while (i--) {
		if ((point.x==mapnotes[i]->Pos.x) &&
			(point.y==mapnotes[i]->Pos.y)) {
			delete mapnotes[i];
			mapnotes.erase(mapnotes.begin()+i);
		}
	}
}

MapNote *Map::GetMapNote(Point &point)
{
	int i = mapnotes.size();
	while (i--) {
		if (Distance(point, mapnotes[i]->Pos) < 10 ) {
			return mapnotes[i];
		}
	}
	return NULL;
}
//--------spawning------------------
void Map::SpawnCreature(Point &pos, char *CreName, int radius)
{
	char *Spawngroup=NULL;
	Actor *creature;
	if ( !Spawns.Lookup( CreName, Spawngroup) ) {
		DataStream *stream = core->GetResourceMgr()->GetResource( CreName, IE_CRE_CLASS_ID );
		creature = core->GetCreature(stream); 
		if ( creature ) {
			creature->SetPosition( this, pos, true, radius );
			AddActor(creature);
		}
		return;
	}
	//adjust this with difflev too
	int count = *(ieDword *) Spawngroup;
	//int difficulty = *(((ieDword *) Spawngroup)+1);
	while ( count-- ) {
		DataStream *stream = core->GetResourceMgr()->GetResource( ((ieResRef *) Spawngroup)[count+1], IE_CRE_CLASS_ID );
		creature = core->GetCreature(stream); 
		if ( creature ) {
			creature->SetPosition( this, pos, true, radius );
			AddActor(creature);
		}
	}
}

void Map::TriggerSpawn(Spawn *spawn)
{
	//is it still active
	if (!spawn->Flags) {
		return;
	}
	//check schedule
	ieDword bit = 1<<(core->GetGame()->GameTime%7200/300);
	if (!(spawn->appearance & bit)) {
		return;
	}

	//check day or night chance
	if (rand()%100>spawn->DayChance) {
		return;
	}
	//check difficulty
	//create spawns
	for(unsigned int i = 0;i<spawn->Count;i++) {
		SpawnCreature(spawn->Pos, spawn->Creatures[i], 0);
	}
	//disable spawnpoint
	spawn->Flags = 0;
}

//--------restheader----------------
bool Map::Rest(Point &pos, int hours)
{
	int chance=RestHeader.DayChance; //based on ingame timer
	if ( !RestHeader.CreatureNum) return false;
	for (int i=0;i<hours;i++) {
		if ( rand()%100<chance ) {
			int idx = rand()%RestHeader.CreatureNum;
			char *str=core->GetString( RestHeader.Strref[idx] );
			core->DisplayString( str );
			free(str);
			SpawnCreature(pos, RestHeader.CreResRef[idx], 20 );
			return true;
		}
	}
	return false;
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

// x, y are in tile coordinates
void Map::ExploreTile(Point &pos)
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

void Map::ExploreMapChunk(Point &Pos, int range, bool los)
{
	Point Tile;

	if (range>MaxVisibility) {
		range=MaxVisibility;
	}
	int p=Perimeter;
	while (p--) {
		int Pass = 2;
		bool block = false;
		bool sidewall = false;
		for (int i=0;i<range;i++) {
			Tile.x = Pos.x+VisibilityMasks[i][p].x;
			Tile.y = Pos.y+VisibilityMasks[i][p].y;
			
			if (los) {
				if (!block) {
					int type = GetBlocked(Tile);
					if (type & PATH_MAP_NO_SEE) {
						block=true;
					} else if (type & PATH_MAP_SIDEWALL) {
						sidewall=true;
					} else if (sidewall) block=true;
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
	if (!core->FogOfWar) {
		SetMapVisibility( -1 );
		return;
	}

	SetMapVisibility( 0 );
	for (unsigned int e = 0; e<actors.size(); e++) {
		Actor *actor = actors[e];
		if (!actor->GetStat( IE_EXPLORE ) ) continue;
		int state = actor->GetStat( IE_STATE_ID );
		if (state & STATE_CANTSEE) continue;
		int vis2 = actor->GetStat( IE_VISUALRANGE );
		if (state&STATE_BLIND) vis2=2; //can see only themselves
		ExploreMapChunk (actor->Pos, vis2, true);
		Spawn *sp = GetSpawnRadius(actor->Pos, SPAWN_RANGE); //30 * 12
		if (sp) {
			TriggerSpawn(sp);
		}
	}	
}

Spawn* Map::GetSpawn(const char* Name)
{
	for (size_t i = 0; i < spawns.size(); i++) {
		Spawn* sp = spawns[i];

		if (stricmp( sp->Name, Name ) == 0)
			return sp;
	}
	return NULL;
}

Spawn *Map::GetSpawnRadius(Point &point, unsigned int radius)
{
	for (size_t i = 0; i < spawns.size(); i++) {
		Spawn* sp = spawns[i];

		if (Distance(point, sp->Pos)<radius) {
			return sp;
		}
	}
	return NULL;
}

int Map::ConsolidateContainers()
{
	int itemcount = 0;
	int containercount = TMap->GetContainerCount();
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
void Map::CopyGroundPiles(Map *othermap, Point &Pos)
{
	int containercount = TMap->GetContainerCount();
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

Container *Map::GetPile(Point &position)
{
	Point tmp[4];
	char heapname[32];

	//converting to search square
	position.x/=16;
	position.y/=12;
	sprintf(heapname,"heap_%hd.%hd",position.x,position.y);
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

void Map::AddItemToLocation(Point &position, CREItem *item)
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

////////////////////AreaAnimation//////////////////
//Area animation

AreaAnimation::AreaAnimation()
{
	animation=NULL;
	animcount=0;
}

AreaAnimation::~AreaAnimation()
{
	for(int i=0;i<animcount;i++) {
		if (animation[i]) {
			delete (animation[i]);
		}
	}
	free(animation);
}

void AreaAnimation::SetPalette(ieResRef Pal)
{
	Flags |= A_ANI_PALETTE;
	strnuprcpy(Palette, Pal, 8);
	ImageMgr *bmp = (ImageMgr *) core->GetInterface( IE_BMP_CLASS_ID);
	if (!bmp) {
		return;
	}
	DataStream* s = core->GetResourceMgr()->GetResource( Pal, IE_BMP_CLASS_ID );
	bmp->Open( s, true );
	Color *pal = (Color *) malloc( sizeof(Color) * 256 );
	bmp->GetPalette( 0, 256, pal );
	core->FreeInterface( bmp );
	for (int i=0;i<animcount;i++) {
		animation[i]->SetPalette( pal, true );
	}
	free (pal);
}

bool AreaAnimation::Schedule(ieDword gametime)
{
	if (!(Flags&A_ANI_ACTIVE) ) {
		return false;
	}

	//check for schedule
	ieDword bit = 1<<(gametime%7200/300);
	if (appearance & bit) {
		return true;
	}
	return false;
}
