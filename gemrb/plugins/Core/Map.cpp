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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Map.cpp,v 1.153 2005/04/09 19:13:42 avenger_teambg Exp $
 *
 */

#include "../../includes/win32def.h"
#include "Map.h"
#include "Interface.h"
#include "PathFinder.h"
#include "Ambient.h"
#include "../../includes/strrefs.h"
#include "AmbientMgr.h"

//#include <stdlib.h>
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
static int Perimeter;  //calculated from MaxVisibility
static int NormalCost = 10;
static int AdditionalCost = 4;
static int Passable[16] = {
	4, 1, 1, 1, 1, 1, 1, 1, 0, 1, 8, 0, 0, 0, 3, 1
};
static Point **VisibilityMasks=NULL;

static bool PathFinderInited = false;
static Variables Spawns;
int LargeFog;

#define STEP_TIME 150

void ReleaseMemoryMap()
{
	for (int i=0;i<MaxVisibility;i++) {
		free(VisibilityMasks[i]);
	}
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
	vars = NULL;
	TMap = NULL;
	LightMap = NULL;
	SearchMap = NULL;
	SmallMap = NULL;
	MapSet = NULL;
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
	}
	ExploredBitmap = NULL;
	VisibleBitmap = NULL;
}

Map::~Map(void)
{
	unsigned int i;

	if (vars) {
		delete vars;
	}

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
	case CT_CANTMOVE:
		return;
	case CT_ACTIVE: case CT_WHOLE:
		break;
	}

	ip->Flags&=~TRAP_RESET; //exit triggered
	if (ip->Destination[0] != 0) {
		CreateMovement(Tmp, ip->Destination, ip->EntranceName);
		if (EveryOne&CT_GO_CLOSER) {
			int i=game->GetPartySize(false);
			while (i--) {
				game->GetPC(i)->ClearPath();
				game->GetPC(i)->ClearActions();
				game->GetPC(i)->AddAction( GameScript::GenerateAction( Tmp ) );
			}
			return;
		}
		actor->ClearPath();
		actor->ClearActions();
		actor->AddAction( GameScript::GenerateAction( Tmp ) );
	} else {
		if (ip->Scripts[0]) {
			ip->LastEntered = actor;
			ip->LastTrigger = actor;
			ip->ExecuteScript( ip->Scripts[0] );
			ip->ProcessActions();
			//this isn't a continuously running script
			//turning oncreation to false on first run
			ip->OnCreation = false;
		}
	}
}

void Map::UpdateScripts()
{
	//Run the Global Script
	Game* game = core->GetGame();
	game->ExecuteScript( game->Scripts[0] );
	game->OnCreation = false;
	game->ProcessActions();
	//Run the Map Script
	if (Scripts[0]) {
		ExecuteScript( Scripts[0] );
	}
	OnCreation = false;
	//Execute Pending Actions
	ProcessActions();

	//Run actor scripts (only for 0 priority)
	int q=Qcount[0];

	while (q--) {
		Actor* actor = queue[0][q];
		actor->ProcessActions();
		for (unsigned int i = 0; i < 8; i++) {
		if (actor->Scripts[i]) {
			if(actor->GetNextAction()) break;
				actor->ExecuteScript( actor->Scripts[i] );
			}
		}

		//returns true if actor should be completely removed
		actor->OnCreation = false;
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
		//Execute Pending Actions
		ip->ProcessActions();
		//If this InfoPoint is a Switch Trigger
		if (ip->Type == ST_TRIGGER) {
			//Check if this InfoPoint was activated
			if (ip->LastTrigger) {
				//Run the InfoPoint script
				ip->ExecuteScript( ip->Scripts[0] );
				//OnCreation won't trigger the INFO point
				//If it does, alter the condition above
				ip->OnCreation = false;
			}
			continue;
		}
		
		q=Qcount[0];
		while (q--) {
			Actor* actor = queue[0][q];
			if (ip->Type == ST_PROXIMITY) {
				if (ip->outline->PointIn( actor->Pos )) {
					ip->Entered(actor);
				}
				ip->ExecuteScript( ip->Scripts[0] );
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
		ip->OnCreation = false;
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

void Map::DrawMap(Region viewport, GameControl* gc)
{
	unsigned int i;
	//Draw the Map
	if (TMap) {
		TMap->DrawOverlay( 0, viewport );
	}
	//Blit the Background Map Animations (before actors)
	Video* video = core->GetVideoDriver();
	for (i = 0; i < animations.size(); i++) {
		Animation *anim = animations[i];

		if (!(anim->Flags&A_ANI_BACKGROUND)) continue; //these are drawn after actors
		if (!(anim->Flags&A_ANI_ACTIVE)) continue;
		
		Point p;
		p.x = anim->x;
		p.y = anim->y;
		if (!IsVisible( p, anim->Flags & A_ANI_NOT_IN_FOG) ) continue;
		Color tint = {0,0,0,0};
		if (!(anim->Flags&A_ANI_NO_SHADOW)) {
			tint = LightMap->GetPixel( p.x / 16, p.y / 12);
			tint.a = 255;
		}
		video->BlitSpriteTinted( anim->NextFrame(),
			p.x + viewport.x, p.y + viewport.y,
			tint, anim->Palette, &viewport );		
	}
	//Draw Selected Door Outline
	if (gc->overDoor) {
		gc->overDoor->DrawOutline();
	}
	if (gc->overContainer) {
		gc->overContainer->DrawOutline();
	}
	Region vp = video->GetViewport();
	Region Screen = vp;
	Screen.x = viewport.x;
	Screen.y = viewport.y;
	// starting with lowest priority (so they are drawn over)
	GenerateQueues();
	int q = 3;
	while (q--) {
		int index = Qcount[q];
		while (true) {
			Actor* actor = GetRoot( q, index );
			if(!actor)
				break;
			//text feedback
			if (actor->textDisplaying) {
				unsigned long time;
				GetTime( time );
				if (( time - actor->timeStartDisplaying ) >= 6000) {
					actor->textDisplaying = 0;
				}
				if (actor->textDisplaying == 1) {
					Font* font = core->GetFont( 1 );
					Region rgn( actor->Pos.x - 100 + viewport.x,
						actor->Pos.y - 100 + viewport.y, 200, 400 );
					font->Print( rgn, ( unsigned char * ) actor->overHeadText,
							NULL, IE_FONT_ALIGN_CENTER | IE_FONT_ALIGN_TOP,
							false );
				}
			}

			int cx = actor->Pos.x;
			int cy = actor->Pos.y;
			//actor isn't visible
			//visual feedback
			CharAnimations* ca = actor->GetAnims();
			if (!ca)
				continue;
			//explored or visibilitymap (bird animations are visible in fog)
			int explored = actor->Modified[IE_DONOTJUMP]&2;
			if (!IsVisible( actor->Pos, explored)) {
				if (actor->Modified[IE_ENABLEOFFSCREENAI])
					continue;
				if (!actor->GetNextAction())
					continue;
				//turning actor inactive
				actor->Active&=~SCR_ACTIVE;
				continue;
			}
			//0 means opaque
			int Trans = actor->Modified[IE_TRANSLUCENT] * 255 / 100;
			if (Trans>255) Trans=255;
			int State = actor->Modified[IE_STATE_ID];
			if (State&STATE_INVISIBLE) {
				//friendlies are half transparent
				if (actor->Modified[IE_EA]<6) {
					Trans=128;
				} else {
					//enemies are fully invisible if invis flag 2 set
					if (actor->Modified[IE_EA]>128) {
						if (State&STATE_INVIS2)
							Trans=256;
						else
							Trans=128;
					} else {
						Trans=256;
					}
				}
			}
			//no visual feedback
			if (Trans>255)
				continue;
			if (( !actor->Modified[IE_NOCIRCLE] ) &&
					( !( State & STATE_DEAD ) )) {
				actor->DrawCircle();
				actor->DrawTargetPoint();
			}

			unsigned char StanceID = actor->GetStance();
			Animation* anim = ca->GetAnimation( StanceID, actor->GetOrientation() );
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
					if (!actor->BBox.InsideRegion( vp ))
						continue;
					Color tint = LightMap->GetPixel( cx / 16, cy / 12);
					tint.a = 255-Trans;
					video->BlitSpriteTinted( nextFrame, cx + viewport.x, cy + viewport.y, tint, anim->Palette, &Screen );
					if (anim->endReached) {
						if (HandleActorStance(actor, ca, StanceID) ) {
							anim->endReached = false;
						}
					}
				}
			}
		}
	}

	//draw normal animations after actors
	for (i = 0; i < animations.size(); i++) {
		Animation *anim = animations[i];

		if (anim->Flags&A_ANI_BACKGROUND) continue; //these are drawn before actors
		if (!(anim->Flags&A_ANI_ACTIVE)) continue;

		Point p;
		p.x = anim->x;
		p.y = anim->y;
		if (!IsVisible( p, anim->Flags & A_ANI_NOT_IN_FOG) ) continue;
		Color tint = {0,0,0,0};
		if (!(anim->Flags&A_ANI_NO_SHADOW)) {
			tint = LightMap->GetPixel( p.x / 16, p.y / 12);
			tint.a = 0xA0;
		}
		video->BlitSpriteTinted( anim->NextFrame(),
			p.x + viewport.x, p.y + viewport.y,
			tint, anim->Palette, &viewport );
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
			video->BlitSprite( frame, vvc->XPos + viewport.x,
					vvc->YPos + viewport.y, false, &viewport );
		} else {
			video->BlitSprite( frame, vvc->XPos + viewport.x,
					vvc->YPos + viewport.y, false, &viewport );
		}
	}

	if (core->FogOfWar && TMap) {
		TMap->DrawFogOfWar( ExploredBitmap, VisibleBitmap, viewport );
	}
}

void Map::AddAnimation(Animation* anim)
{
	animations.push_back( anim );
}

void Map::Shout(Scriptable* actor, int shoutID, unsigned int radius)
{
	int i=actors.size();
	while (i--) {
		if (radius) {
			if (Distance(actor->Pos, actors[i]->Pos)>radius) {
				continue;
			}
		}
		if (shoutID) {
			actors[i]->LastHeard = (Actor *) actor;
			actors[i]->LastShout = shoutID;
		} else {
			actors[i]->LastHelp = (Actor *) actor;
		}
	}
}

void Map::AddActor(Actor* actor)
{
	//setting the current area for the actor as this one
	strnuprcpy(actor->Area, scriptName, 8);
	actor->SetMap(this);
	actors.push_back( actor );
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

/** flags: GA_SELECT=1   - unselectable actors don't play
		 GA_NO_DEAD=2	 - dead actors don't play
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

void Map::AddWallGroup(WallGroup* wg)
{
	wallGroups.push_back( wg );
}

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

		if (actor->Active&SCR_ACTIVE) {
			if (actor->GetStance() == IE_ANI_TWITCH) {
				priority=1;
			} else {
				priority=0;
			}
		} else {
			priority=2;
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

Animation* Map::GetAnimation(const char* Name)
{
	unsigned int i=animations.size();
	while (i--) {
		Animation *anim = animations[i];

		if (anim->ScriptName && (strnicmp( anim->ScriptName, Name, 32 ) == 0)) {
			return anim;
		}
	}
	return NULL;
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
		if (stricmp( entrances[i]->Name, Name ) == 0) {
			return entrances[i];
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
PathNode* Map::RunAway(Point &s, Point &d, unsigned int PathLen, bool Backing)
{
	Point start = {s.x/16, s.y/12};
	Point goal = {d.x/16, d.y/12};
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

		unsigned int Cost = MapSet[y * Width + x] + 1;
		if (Cost > PathLen) {
			//printf("Path not found!\n");
			break;
		}
		SetupNode( x - 1, y - 1, Cost );
		SetupNode( x + 1, y - 1, Cost );
		SetupNode( x + 1, y + 1, Cost );
		SetupNode( x - 1, y + 1, Cost );

		Cost ++;
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
	if (Backing) {
		StartNode->orient = GetOrient( start, best );
	} else {
		StartNode->orient = GetOrient( best, start );
	}
	Point p = best;
	unsigned int pos2 = start.y * Width + start.x;
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

		if (Backing) {
			StartNode->orient = GetOrient( p, n );
		} else {
			StartNode->orient = GetOrient( n, p );
		}
		p = n;
	}
	return Return;
}

bool Map::TargetUnreachable(Point &s, Point &d)
{
	Point start = { s.x/16, s.y/12 };
	Point goal = { d.x/16, d.y/12 };
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

PathNode* Map::FindPath(Point &s, Point &d)
{
	Point start = { s.x/16, s.y/12 };
	Point goal = { d.x/16, d.y/12 };
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
	return Return;
}

//single point visible or not (visible/exploredbitmap)
//if explored = true then visible in fog
bool Map::IsVisible(Point &pos, int explored)
{
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
void Map::AddMapNote(Point point, int color, char *text)
{
	MapNote *mn = new MapNote;

	mn->Pos=point;
	mn->color=color;
	mn->text=text;
	RemoveMapNote(point);    //delete previous mapnote
	mapnotes.push_back(mn);
}

void Map::RemoveMapNote(Point point)
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

MapNote *Map::GetMapNote(Point point)
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
void Map::SpawnCreature(Point pos, char *CreName, int radius)
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

//--------restheader----------------
bool Map::Rest(Point pos, int hours)
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
	if(!core->FogOfWar) {
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
	}	
}
