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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Map.cpp,v 1.138 2005/02/27 19:13:24 edheldil Exp $
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

static int NormalCost = 10;
static int AdditionalCost = 4;
static int Passable[16] = {
	4, 1, 1, 1, 1, 1, 1, 1, 4, 1, 0, 0, 0, 0, 3, 1
};
static bool PathFinderInited = false;
static Variables Spawns;

#define STEP_TIME 150

void InitSpawnGroups()
{
	ieResRef GroupName;
	int i;
	TableMgr * tab;

	int table=core->LoadTable( "spawngrp" );

	Spawns.RemoveAll();
	Spawns.SetType( GEM_VARIABLES_STRING );
	
	if(table<0) {
		return;
	}
	tab = core->GetTable( table );
	if(!tab) {
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
			for(;j;j--) {
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

Map::Map(void)
	: Scriptable( ST_AREA )
{
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
			delete[] queue[i];
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
	Map* map = game->GetMap(area);
	if(!map) {
		printf("Invalid map: %s\n",area);
		command[0]=0;
		return;
	}
	Entrance* ent = map->GetEntrance( entrance );
	int X,Y, face;
	if (!ent) {
		textcolor( YELLOW );
		printf( "WARNING!!! %s EntryPoint does not Exists\n", entrance );
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
	case 2:
		core->DisplayConstantString(STR_WHOLEPARTY,0xffffff); //white
		if(game->EveryoneStopped()) {
			ip->Flags&=~TRAP_RESET; //exit triggered
		}
		return;
	case 0:
		return;
	case 1: case 3:
		break;
	}

	ip->Flags&=~TRAP_RESET; //exit triggered
	if (ip->Destination[0] != 0) {
		CreateMovement(Tmp, ip->Destination, ip->EntranceName);
		if(EveryOne&2) {
			int i=game->GetPartySize(false);
			while(i--) {
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

void Map::DrawMap(Region viewport, GameControl* gc)
{
	unsigned int i;
	//Draw the Map
	if (TMap) {
		TMap->DrawOverlay( 0, viewport );
	}
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
	//Check if we need to start some trigger scripts
	int ipCount = 0;
	while (true) {
		//For each InfoPoint in the map
		InfoPoint* ip = TMap->GetInfoPoint( ipCount++ );
		if (!ip)
			break;
		if (!ip->Active)
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
		i=actors.size();
		while (i--) {
			Actor* actor = actors[i];
			if (!actor)
				continue;
			if (ip->Type == ST_PROXIMITY) {
				if (ip->outline->PointIn( actor->Pos )) {
					ip->LastEntered = actor;
					ip->LastTrigger = actor;
				}
				ip->ExecuteScript( ip->Scripts[0] );
			} else {
				//ST_TRAVEL
				//don't move if doing something else
				if(actor->GetNextAction())
					continue;
				if (ip->outline->PointIn( actor->Pos )) {
					ip->LastEntered = actor;
					UseExit(actor, ip);
				}
			}
		}
		ip->OnCreation = false;
	}
	//Blit the Map Animations
	Video* video = core->GetVideoDriver();
	for (i = 0; i < animations.size(); i++) {
		if (animations[i]->Active) {
			video->BlitSpriteMode( animations[i]->NextFrame(),
					animations[i]->x + viewport.x,
					animations[i]->y + viewport.y, animations[i]->BlitMode,
					false, &viewport );
		}
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
	for (int q = 0; q < 3; q++) {
		GenerateQueue( q );
		while (true) {
			Actor* actor = GetRoot( q );
			if (!actor)
				break;
			if (!actor->Active)
				continue;
			actor->ProcessActions();
			//moved scripts before display
			//this should enable scripts for offscreen actors
			for (i = 0; i < 8; i++) {
				if (actor->Scripts[i])
					actor->ExecuteScript( actor->Scripts[i] );
			}

			//returns true if actor should be completely removed
			if(actor->CheckOnDeath()) {
				DeleteActor( actor );
				continue;
			}

			actor->OnCreation = false;
			actor->inventory.CalculateWeight();
			actor->SetStat( IE_ENCUMBRANCE, actor->inventory.GetWeight() );

			actor->DoStep( );
			CharAnimations* ca = actor->GetAnims();
			if (!ca)
				continue;
			Animation* anim = ca->GetAnimation( actor->GetStance(), actor->GetOrientation() );
			if (anim &&
				anim->autoSwitchOnEnd &&
				anim->endReached &&
				anim->nextStanceID) {
				actor->SetStance( anim->nextStanceID );
				anim = ca->GetAnimation( actor->GetStance(), actor->GetOrientation() );
			}
			if (( !actor->Modified[IE_NOCIRCLE] ) &&
			    ( !( actor->Modified[IE_STATE_ID] & STATE_DEAD ) )) {
				actor->DrawCircle();
				actor->DrawTargetPoint();
			}
			if (anim) {
				Sprite2D* nextFrame = anim->NextFrame();
				if (nextFrame) {
					if (actor->lastFrame != nextFrame) {
						Region newBBox;
						newBBox.x = actor->Pos.x - nextFrame->XPos;
						newBBox.w = nextFrame->Width;
						newBBox.y = actor->Pos.y - nextFrame->YPos;
						newBBox.h = nextFrame->Height;
						actor->lastFrame = nextFrame;
						actor->SetBBox( newBBox );
					}
					if (!actor->BBox.InsideRegion( vp ))
						continue;
					Point a = actor->Pos;
					int cx = a.x / 16;
					int cy = a.y / 12;
					Color tint = LightMap->GetPixel( cx, cy );
					tint.a = 0xA0;
					video->BlitSpriteTinted( nextFrame, a.x + viewport.x,
							a.y + viewport.y, tint, &Screen );
					if (anim->endReached && anim->autoSwitchOnEnd) {
						actor->SetStance( anim->nextStanceID );
						anim->autoSwitchOnEnd = false;
					}
				}
			}
			if (actor->textDisplaying) {
				unsigned long time;
				GetTime( time );
				if (( time - actor->timeStartDisplaying ) >= 6000) {
					actor->textDisplaying = 0;
				}
				if (actor->textDisplaying == 1) {
					Font* font = core->GetFont( 9 );
					Region rgn( actor->Pos.x - 100 + viewport.x,
						actor->Pos.y - 100 + viewport.y, 200, 400 );
					font->Print( rgn, ( unsigned char * ) actor->overHeadText,
							NULL, IE_FONT_ALIGN_CENTER | IE_FONT_ALIGN_TOP,
							false );
				}
			}
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
			video->BlitSpriteMode( frame, vvc->XPos + viewport.x,
					vvc->YPos + viewport.y, 1, false, &viewport );
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
	while(i--) {
		if(radius) {
			if(Distance(actor->Pos, actors[i]->Pos)>radius) {
				continue;
			}
		}
		if(shoutID) {
			actors[i]->LastHeard = (Actor *) actor;
			actors[i]->LastShout = shoutID;
		}
		else {
			actors[i]->LastHelp = (Actor *) actor;
		}
	}
}

void Map::AddActor(Actor* actor)
{
	//setting the current area for the actor as this one
	memcpy(actor->Area, scriptName, 9);
	actors.push_back( actor );
}

void Map::DeleteActor(Actor* actor)
{
	std::vector< Actor*>::iterator m;
	for (m = actors.begin(); m != actors.end(); ++m) {
		if (( *m ) == actor) {
			Game *game = core->GetGame();
			game->LeaveParty( actor );
			game->DelNPC( game->InStore(actor) );
			actors.erase( m );
			delete (actor);
			lastActorCount[0] = 0;
			lastActorCount[1] = 0;
			lastActorCount[2] = 0;
			return;
		}
	}
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
	while(i--) {
		Actor* actor = actors[i];
		if (stricmp( actor->scriptName, Name ) == 0) {
			return actor;
		}
	}
	return NULL;
}

Actor* Map::GetActorByDialog(const char *resref)
{
	unsigned int i = actors.size();
	while(i--) {
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
	while(i--) {
		Actor* actor = actors[i];
//use this function only for party?
		if(onlyparty && !actor->InParty)
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

void Map::GenerateQueue(int priority)
{
	if (lastActorCount[priority] != actors.size()) {
		if (queue[priority]) {
			delete[] queue[priority];
			queue[priority] = NULL;
		}
		queue[priority] = new Actor * [actors.size()];
		lastActorCount[priority] = ( int ) actors.size();
	}
	Qcount[priority] = 0;
	unsigned int i=actors.size();
	while(i--) {
		Actor* actor = actors[i];
		switch (priority) {
			case 0:
				//Top Priority
				if (actor->GetStance() != IE_ANI_SLEEP)
					continue;
				break;

			case 1:
				//Normal Priority
				if (actor->GetStance() == IE_ANI_SLEEP)
					continue;
				break;

			case 2:
				continue;
		} 
		Qcount[priority]++;
		queue[priority][Qcount[priority] - 1] = actor;
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

Actor* Map::GetRoot(int priority)
{
	if (Qcount[priority] == 0) {
		return NULL;
	}
	Actor* ret = queue[priority][0];
	Actor* node = queue[priority][0] = queue[priority][Qcount[priority] - 1];
	Qcount[priority]--;
	int lastPos = 1;
	while (true) {
		int leftChildPos = ( lastPos*2 ) - 1;
		int rightChildPos = lastPos*2;
		if (leftChildPos >= Qcount[priority])
			break;
		Actor* child = queue[priority][leftChildPos];
		int childPos = leftChildPos;
		if (rightChildPos < Qcount[priority]) {
			//If both Child Exist
			Actor* rightChild = queue[priority][lastPos*2];
			if (rightChild->Pos.y < child->Pos.y) {
				childPos = rightChildPos;
				child = rightChild;
			}
		}
		if (node->Pos.y > child->Pos.y) {
			queue[priority][lastPos - 1] = child;
			queue[priority][childPos] = node;
			lastPos = childPos + 1;
		} else
			break;
	}
	return ret;
}

void Map::AddVVCCell(ScriptedAnimation* vvc)
{
	unsigned int i=vvcCells.size();
	while(i--) {
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
	while(i--) {
		if (strnicmp( animations[i]->ResRef, Name, 8 ) == 0) {
			return animations[i];
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
	while(i--) {
		if (stricmp( entrances[i]->Name, Name ) == 0) {
			return entrances[i];
		}
	}
	return NULL;
}

bool Map::HasActor(Actor *actor)
{
	unsigned int i=actors.size();
	while(i--) {
		if (actors[i] == actor) {
			return true;
		}
	}
	return false;
}

void Map::RemoveActor(Actor* actor)
{
	unsigned int i=actors.size();
	while(i--) {
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
	while(i--) {
		if(actors[i]->InParty) {
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
		if(dist<distance) {
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
	if(Backing) {
		StartNode->orient = GetOrient( start, best );
	}
	else {
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

		if(Backing) {
			StartNode->orient = GetOrient( p, n );
		}
		else {
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
		printf("This isn't a travel region [%d.%d]?\n",sX, sY);
		return -1;
	}
	sX*=Height;
	sY*=Width;
	if(sX<sY) { //north or west
		if(Width*Height<sX+sY) { //
			return WMP_NORTH;
		}
		return WMP_WEST;
	}
	//south or east
	if(Width*Height<sX+sY) { //
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
	while(i--) {
		if((point.x==mapnotes[i]->Pos.x) &&
			(point.y==mapnotes[i]->Pos.y)) {
			delete mapnotes[i];
			mapnotes.erase(mapnotes.begin()+i);
		}
	}
}

MapNote *Map::GetMapNote(Point point)
{
	int i = mapnotes.size();
	while(i--) {
		if(Distance(point, mapnotes[i]->Pos) < 10 ) {
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
	if( !Spawns.Lookup( CreName, Spawngroup) ) {
		DataStream *stream = core->GetResourceMgr()->GetResource( CreName, IE_CRE_CLASS_ID );
		creature = core->GetCreature(stream); 
		if( creature ) {
			creature->SetPosition( this, pos, true, radius );
			AddActor(creature);
		}
		return;
	}
	//adjust this with difflev too
	int count = *(ieDword *) Spawngroup;
	//int difficulty = *(((ieDword *) Spawngroup)+1);
	while( count-- ) {
		DataStream *stream = core->GetResourceMgr()->GetResource( ((ieResRef *) Spawngroup)[count+1], IE_CRE_CLASS_ID );
		creature = core->GetCreature(stream); 
		if( creature ) {
			creature->SetPosition( this, pos, true, radius );
			AddActor(creature);
		}
	}
}

//--------restheader----------------
bool Map::Rest(Point pos, int hours)
{
	int chance=RestHeader.DayChance; //based on ingame timer
	if( !RestHeader.CreatureNum) return false;
	for(int i=0;i<hours;i++) {
		if( rand()%100<chance ) {
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
	if (!core->HasFeature(GF_SMALL_FOG) ) {
		x++;
		y++;
	}
	return (x*y+7)/8;
}

