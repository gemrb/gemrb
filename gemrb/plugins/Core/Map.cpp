/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Map.cpp,v 1.89 2004/04/17 11:28:10 avenger_teambg Exp $
 *
 */

#include "../../includes/win32def.h"
#include "Map.h"
#include "Interface.h"
#include "PathFinder.h"
//#include <stdlib.h>
#ifndef WIN32
#include <sys/time.h>
#endif

extern Interface* core;
static int NormalCost = 10;
static int AdditionalCost = 4;
static int Passable[16] = {
	4, 1, 1, 1, 1, 1, 1, 1, 4, 1, 0, 0, 0, 0, 2, 1
};
static bool PathFinderInited = false;

#define STEP_TIME 150

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
	tm = NULL;
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
	}
}

Map::~Map(void)
{
	if (tm) {
		delete( tm );
	}
	for (unsigned int i = 0; i < animations.size(); i++) {
		delete( animations[i] );
	}

	for (unsigned int i = 0; i < actors.size(); i++) {
		Actor* a = actors[i];
		if (a && !a->InParty && !a->FromGame) {
			delete ( a );
		}
	}

	for (unsigned int i = 0; i < entrances.size(); i++) {
		delete( entrances[i] );
	}
	core->FreeInterface( LightMap );
	core->FreeInterface( SearchMap );
	for (int i = 0; i < 3; i++) {
		if (queue[i]) {
			delete[] queue[i];
			queue[i] = NULL;
		}
	}
	for (unsigned int i = 0; i < vvcCells.size(); i++) {
		if (vvcCells[i]) {
			delete vvcCells[i];
			vvcCells[i] = NULL;
		}
	}
}

void Map::AddTileMap(TileMap* tm, ImageMgr* lm, ImageMgr* sr)
{
	this->tm = tm;
	LightMap = lm;
	SearchMap = sr;
}

void Map::DrawMap(Region viewport, GameControl* gc)
{
	//Draw the Map
	if (tm) {
		tm->DrawOverlay( 0, viewport );
	}
	//Run the Global Script
	Game* game = core->GetGame();
	game->ExecuteScript( game->Scripts[0] );
	game->OnCreation = false;
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
		InfoPoint* ip = tm->GetInfoPoint( ipCount++ );
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
			if (ip->LastTrigger)
				//Run the InfoPoint script
				ip->ExecuteScript( ip->Scripts[0] );
				ip->OnCreation = false;
			continue;
		}
		Region BBox = ip->outline->BBox;
		if (ip->Type == ST_PROXIMITY) {
			if (BBox.x <= 500)
				BBox.x = 0;
			else
				BBox.x -= 500;
			if (BBox.y <= 500)
				BBox.y = 0;
			else
				BBox.y -= 500;
			BBox.h += 1000;
			BBox.w += 1000;
		}
		int i = 0;
		while (true) {
			Actor* actor = core->GetGame()->GetPC( i++ );
			if (!actor)
				break;
			if (!actor->InParty)
				break;
			if (BBox.PointInside( actor->XPos, actor->YPos )) {
				if (ip->Type == ST_PROXIMITY) {
					if (ip->outline->BBox.PointInside( actor->XPos,
											actor->YPos )) {
						if (ip->outline->PointIn( actor->XPos, actor->YPos )) {
							ip->LastEntered = actor;
							ip->LastTrigger = actor;
						}
					}
					ip->ExecuteScript( ip->Scripts[0] );
					ip->OnCreation = false;
				} else {
					//ST_TRAVEL
					if (ip->outline->PointIn( actor->XPos, actor->YPos )) {
						char Tmp[256];
						sprintf( Tmp,
							"[color=00FF00]%s TravelTrigger Activated: [/color]Should Move to Area %s near %s",
							ip->Name, ip->Destination, ip->EntranceName );
						gc->DisplayString( Tmp );

						if (ip->Destination[0] != 0) {
							sprintf( Tmp,
								"LeaveAreaLUA(\"%s\", \"\", [-1.-1], -1)",
								ip->Destination );
							actor->ClearPath();
							actor->AddActionInFront( GameScript::GenerateAction( Tmp ) );
							strcpy( gc->EntranceName, ip->EntranceName );
						} else {
							if (ip->Scripts[0]) {
								ip->LastTrigger = actor;
								//ip->Scripts[0]->Update();
								ip->ExecuteScript( ip->Scripts[0] );
								ip->ProcessActions();
								ip->OnCreation = false;
							}
						}
					}
				}
				break;
			}
		}
	}
	//Blit the Map Animations
	Video* video = core->GetVideoDriver();
	for (unsigned int i = 0; i < animations.size(); i++) {
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
			if (actor->DeleteMe) {
				DeleteActor( actor );
				continue;
			}
			actor->ProcessActions();
			actor->DoStep( LightMap );
			CharAnimations* ca = actor->GetAnims();
			if (!ca)
				continue;
			Animation* anim = ca->GetAnimation( actor->AnimID, actor->Orientation );
			if (anim &&
				anim->autoSwitchOnEnd &&
				anim->endReached &&
				anim->nextAnimID) {
				actor->AnimID = anim->nextAnimID;
				anim = ca->GetAnimation( actor->AnimID, actor->Orientation );
			}
			if (( !actor->Modified[IE_NOCIRCLE] ) &&
				( !( actor->Modified[IE_STATE_ID] & STATE_DEAD ) ))
				actor->DrawCircle();
			if (anim) {
				Sprite2D* nextFrame = anim->NextFrame();
				if (nextFrame) {
					if (actor->lastFrame != nextFrame) {
						Region newBBox;
						newBBox.x = actor->XPos - nextFrame->XPos;
						newBBox.w = nextFrame->Width;
						newBBox.y = actor->YPos - nextFrame->YPos;
						newBBox.h = nextFrame->Height;
						actor->lastFrame = nextFrame;
						actor->SetBBox( newBBox );
					}
					if (!actor->BBox.InsideRegion( vp ))
						continue;
					int ax = actor-> XPos, ay = actor->YPos;
					int cx = ax / 16;
					int cy = ay / 12;
					Color tint = LightMap->GetPixel( cx, cy );
					tint.a = 0xA0;
					video->BlitSpriteTinted( nextFrame, ax + viewport.x,
							ay + viewport.y, tint, &Screen );
					if (anim->endReached && anim->autoSwitchOnEnd) {
						actor->AnimID = anim->nextAnimID;
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
					Region rgn( actor->XPos - 100 + viewport.x,
						actor->YPos - 100 + viewport.y, 200, 400 );
					font->Print( rgn, ( unsigned char * ) actor->overHeadText,
							NULL, IE_FONT_ALIGN_CENTER | IE_FONT_ALIGN_TOP,
							false );
				}
			}
			for (int i = 0; i < 8; i++) {
				if (actor->Scripts[i])
					actor->ExecuteScript( actor->Scripts[i] );
			}
			actor->OnCreation = false;
		}
	}
	for (unsigned int i = 0; i < vvcCells.size(); i++) {
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
}

void Map::AddAnimation(Animation* anim)
{
	animations.push_back( anim );
}

void Map::AddActor(Actor* actor)
{
	CharAnimations* ca = actor->GetAnims();
	if (ca) {
		Animation* anim = ca->GetAnimation( actor->AnimID, actor->Orientation );
		if (anim) {
			Sprite2D* nextFrame = anim->NextFrame();
			if (actor->lastFrame != nextFrame) {
				Region newBBox;
				newBBox.x = actor->XPos - nextFrame->XPos;
				newBBox.w = nextFrame->Width;
				newBBox.y = actor->YPos - nextFrame->YPos;
				newBBox.h = nextFrame->Height;
				actor->lastFrame = nextFrame;
				actor->SetBBox( newBBox );
			}
		}
	}
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
Actor* Map::GetActor(unsigned int x, unsigned int y, int flags)
{
	unsigned int i = actors.size();
	while (i--) {
		Actor* actor = actors[i];
		
		if (actor->DeleteMe) {
			continue; //actor is already marked for removal
		}
		if (flags&GA_SELECT) {
			if (actor->BaseStats[IE_UNSELECTABLE]) {
				continue;
			}
		}
		if (flags&GA_NO_DEAD) {
			if (actor->BaseStats[IE_STATE_ID] & STATE_DEAD) {
				continue;
			}
		}
		if (actor->IsOver( ( unsigned short ) x, ( unsigned short ) y ))
			return actor;
	}
	return NULL;
}

Actor* Map::GetActor(const char* Name)
{
	for (size_t i = 0; i < actors.size(); i++) {
		Actor* actor = actors.at( i );
		if (stricmp( actor->scriptName, Name ) == 0) {
			return actor;
		}
	}
	return NULL;
}

int Map::GetActorInRect(Actor**& actorlist, Region& rgn)
{
	actorlist = ( Actor * * ) malloc( actors.size() * sizeof( Actor * ) );
	int count = 0;
	for (size_t i = 0; i < actors.size(); i++) {
		Actor* actor = actors.at( i );
		if (actor->BaseStats[IE_UNSELECTABLE] ||
			( actor->BaseStats[IE_STATE_ID] & STATE_DEAD ) ||
			( !actor->Active ))
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
		/*since bg1 and pst has no .2da for songlist, we must supply one in
			  the gemrb/override folder.
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

int Map::GetBlocked(unsigned int cx, unsigned int cy)
{
	int block = SearchMap->GetPixelIndex( cx / 16, cy / 12 );
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
	for (unsigned int i = 0; i < actors.size(); i++) {
		Actor* actor = actors.at( i );
		switch (priority) {
			case 0:
				//Top Priority
				 {
					if (actor->AnimID != IE_ANI_SLEEP)
						continue;
				}
				break;

			case 1:
				//Normal Priority
				 {
					if (actor->AnimID == IE_ANI_SLEEP)
						continue;
				}
				break;

			case 2:
				 {
					continue;
				}
				break;
		} 
		Qcount[priority]++;
		queue[priority][Qcount[priority] - 1] = actor;
		int lastPos = Qcount[priority];
		while (lastPos != 1) {
			int parentPos = ( lastPos / 2 ) - 1;
			Actor* parent = queue[priority][parentPos];
			if (actor->YPos < parent->YPos) {
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
			if (rightChild->YPos < child->YPos) {
				childPos = rightChildPos;
				child = rightChild;
			}
		}
		//if((node->YPos > child->YPos) || (child->AnimID == IE_ANI_SLEEP)) {
		if (node->YPos > child->YPos) {
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
	for (unsigned int i = 0; i < vvcCells.size(); i++) {
		if (vvcCells[i] == NULL) {
			vvcCells[i] = vvc;
			return;
		}
	}
	vvcCells.push_back( vvc );
}

Animation* Map::GetAnimation(const char* Name)
{
	for (unsigned int i = 0; i < animations.size(); i++) {
		if (strnicmp( animations[i]->ResRef, Name, 8 ) == 0) {
			return animations[i];
		}
	}
	return NULL;
}

void Map::AddEntrance(char* Name, short XPos, short YPos, short Face)
{
	Entrance* ent = new Entrance();
	strncpy( ent->Name, Name, 32 );
	ent->XPos = XPos;
	ent->YPos = YPos;
	ent->Face = Face;
	entrances.push_back( ent );
}

Entrance* Map::GetEntrance(char* Name)
{
	for (unsigned int i = 0; i < entrances.size(); i++) {
		if (stricmp( entrances[i]->Name, Name ) == 0) {
			return entrances[i];
		}
	}
	return NULL;
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

void Map::DebugDump()
{
	printf( "DebugDump of Area %s:\n", scriptName );
}

/********************************************************************************/

PathFinder::PathFinder(void)
{
	MapSet = NULL;
	area = NULL;
	if (!PathFinderInited) {
		InitPathFinder();
	}
}


PathFinder::~PathFinder(void)
{
	if (MapSet) {
		free(MapSet);
	}
}

void PathFinder::SetMap(Map *newarea)
{
	if(area==newarea) {
		return;
	}
	if (MapSet) {
		free(MapSet);
	}
	area=newarea;
	sMap=area->SearchMap;
	Width=area->tm->XCellCount * 4;
	Height=( area->tm->YCellCount * 64 ) / 12;
	//Filling Matrices
	MapSet = (unsigned short *) malloc(sizeof(unsigned short) * Width * Height);
}

void PathFinder::Leveldown(unsigned int px, unsigned int py,
	unsigned int& level, unsigned int& nx, unsigned int& ny,
	unsigned int& diff)
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
		nx = px;
		ny = py;
	}
}

void PathFinder::SetupNode(unsigned int x, unsigned int y, unsigned int Cost)
{
	unsigned int pos;

	if (( x >= Width ) || ( y >= Height )) {
		return;
	}
	pos = y * Width + x;
	if (MapSet[pos]) {
		return;
	}
	if (!( Passable[sMap->GetPixelIndex( x, y )] & 3 )) {
		MapSet[pos] = 65535;
		return;
	}
	MapSet[pos] = Cost;
	InternalStack.push( ( x << 16 ) | y );
}

void PathFinder::AdjustPosition(unsigned int& goalX, unsigned int& goalY)
{
	unsigned int maxr = Width;
	if (maxr < Height) {
		maxr = Height;
	}
	if (goalX > Width) {
		goalX = Width;
	}
	if (goalY > Height) {
		goalY = Height;
	}
	for (unsigned int radius = 1; radius < maxr; radius++) {
		unsigned int minx = 0;
		if (goalX > radius)
			minx = goalX - radius;
		unsigned int maxx = goalX + radius + 1;
		if (maxx > Width)
			maxx = Width;

		for (unsigned int scanx = minx; scanx < maxx; scanx++) {
			if (goalY >= radius) {
				if (Passable[sMap->GetPixelIndex( scanx, goalY - radius )] & 3) {
					goalX = scanx;
					goalY -= radius;
					return;
				}
			}
			if (goalY + radius < Height) {
				if (Passable[sMap->GetPixelIndex( scanx, goalY + radius )] & 3) {
					goalX = scanx;
					goalY += radius;
					return;
				}
			}
		}
		unsigned int miny = 0;
		if (goalY > radius)
			miny = goalY - radius;
		unsigned int maxy = goalY + radius + 1;
		if (maxy > Height)
			maxy = Height;
		for (unsigned int scany = miny; scany < maxy; scany++) {
			if (goalX >= radius) {
				if (Passable[sMap->GetPixelIndex( goalX - radius, scany )] & 3) {
					goalX -= radius;
					goalY = scany;
					return;
				}
			}
			if (goalX + radius < Width) {
				if (Passable[sMap->GetPixelIndex( goalX - radius, scany )] & 3) {
					goalX += radius;
					goalY = scany;
					return;
				}
			}
		}
	}
}

//run away from dX, dY (ie.: find the best path of limited length that brings us the farthest from dX, dY)
PathNode* PathFinder::RunAway(short sX, short sY, short dX, short dY, unsigned int PathLen, bool Backing)
{
	unsigned int startX = sX / 16, startY = sY / 12;
	unsigned int goalX = dX / 16, goalY = dY / 12;
	unsigned int bestX, bestY, dist;

	memset( MapSet, 0, Width * Height * sizeof( unsigned short ) );
	while (InternalStack.size())
		InternalStack.pop();

	if (!( Passable[sMap->GetPixelIndex( startX, startY )] & 3 )) {
		AdjustPosition( startX, startY );
	}
	unsigned int pos = ( startX << 16 ) | startY;
	InternalStack.push( pos );
	MapSet[startY * Width + startX] = 1;
	dist = 0;
	bestX = startX;
	bestY = startY;
	while (InternalStack.size()) {
		pos = InternalStack.front();
		InternalStack.pop();
		unsigned int x = pos >> 16;
		unsigned int y = pos & 0xffff;
		long tx = ( x - goalX );
		long ty = ( y - goalY );
		unsigned int distance = (unsigned int) sqrt( ( double ) ( tx* tx + ty* ty ) );
		if(dist<distance) {
			bestX=x;
			bestY=y;
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
	StartNode->x = bestX;
	StartNode->y = bestY;
	if(Backing) {
		StartNode->orient = GetOrient( startX, startY, bestX, bestY );
	}
	else {
		StartNode->orient = GetOrient( bestX, bestY, startX, startY );
	}
	unsigned int px = bestX;
	unsigned int py = bestY;
	unsigned int pos2 = startY * Width + startX;
	while (( pos = py * Width + px ) != pos2) {
		StartNode->Next = new PathNode;
		StartNode->Next->Parent = StartNode;
		StartNode = StartNode->Next;
		StartNode->Next = NULL;
		unsigned int level = MapSet[pos];
		unsigned int diff = 0;
		unsigned int nx, ny;
		Leveldown( px, py + 1, level, nx, ny, diff );
		Leveldown( px + 1, py, level, nx, ny, diff );
		Leveldown( px - 1, py, level, nx, ny, diff );
		Leveldown( px, py - 1, level, nx, ny, diff );
		Leveldown( px - 1, py + 1, level, nx, ny, diff );
		Leveldown( px + 1, py + 1, level, nx, ny, diff );
		Leveldown( px + 1, py - 1, level, nx, ny, diff );
		Leveldown( px - 1, py - 1, level, nx, ny, diff );
		if (!diff)
			return Return;
		StartNode->x = nx;
		StartNode->y = ny;

		if(Backing) {
			StartNode->orient = GetOrient( px, py, nx, ny );
		}
		else {
			StartNode->orient = GetOrient( nx, ny, px, py );
		}
		px = nx;
		py = ny;
	}
	return Return;
}

bool PathFinder::TargetUnreachable(short sX, short sY, short dX, short dY)
{
	unsigned int startX = sX / 16, startY = sY / 12, goalX = dX / 16,
	goalY = dY / 12;
	memset( MapSet, 0, Width * Height * sizeof( unsigned short ) );
	while (InternalStack.size())
		InternalStack.pop();

	if (!( Passable[sMap->GetPixelIndex( goalX, goalY )] & 3 )) {
		return true;
	}
	if (!( Passable[sMap->GetPixelIndex( startX, startY )] & 3 )) {
		return true;
	}

	unsigned int pos = ( goalX << 16 ) | goalY;
	unsigned int pos2 = ( startX << 16 ) | startY;
	InternalStack.push( pos );
	MapSet[goalY * Width + goalX] = 1;

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

PathNode* PathFinder::FindPath(short sX, short sY, short dX, short dY)
{
	unsigned int startX = sX / 16, startY = sY / 12, goalX = dX / 16,
	goalY = dY / 12;
	memset( MapSet, 0, Width * Height * sizeof( unsigned short ) );
	while (InternalStack.size())
		InternalStack.pop();

	if (!( Passable[sMap->GetPixelIndex( goalX, goalY )] & 3 )) {
		AdjustPosition( goalX, goalY );
	}
	unsigned int pos = ( goalX << 16 ) | goalY;
	unsigned int pos2 = ( startX << 16 ) | startY;
	InternalStack.push( pos );
	MapSet[goalY * Width + goalX] = 1;

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
	StartNode->x = startX;
	StartNode->y = startY;
	StartNode->orient = GetOrient( goalX, goalY, startX, startY );
	if (pos != pos2) {
		return Return;
	}
	unsigned int px = startX;
	unsigned int py = startY;
	pos2 = goalY * Width + goalX;
	while (( pos = py * Width + px ) != pos2) {
		StartNode->Next = new PathNode;
		StartNode->Next->Parent = StartNode;
		StartNode = StartNode->Next;
		StartNode->Next = NULL;
		unsigned int level = MapSet[pos];
		unsigned int diff = 0;
		unsigned int nx, ny;
		Leveldown( px, py + 1, level, nx, ny, diff );
		Leveldown( px + 1, py, level, nx, ny, diff );
		Leveldown( px - 1, py, level, nx, ny, diff );
		Leveldown( px, py - 1, level, nx, ny, diff );
		Leveldown( px - 1, py + 1, level, nx, ny, diff );
		Leveldown( px + 1, py + 1, level, nx, ny, diff );
		Leveldown( px + 1, py - 1, level, nx, ny, diff );
		Leveldown( px - 1, py - 1, level, nx, ny, diff );
		if (!diff)
			return Return;
		StartNode->x = nx;
		StartNode->y = ny;
		StartNode->orient = GetOrient( nx, ny, px, py );
		px = nx;
		py = ny;
	}
	return Return;
}

unsigned char PathFinder::GetOrient(short sX, short sY, short dX, short dY)
{
	short deltaX = ( dX- sX), deltaY = ( dY - sY );
	if (deltaX > 0) {
		if (deltaY > 0) {
			return 6;
		} else if (deltaY == 0) {
			return 4;
		} else {
			return 2;
		}
	} else if (deltaX == 0) {
		if (deltaY > 0) {
			return 8;
		} else {
			return 0;
		}
	} else {
		if (deltaY > 0) {
			return 10;
		} else if (deltaY == 0) {
			return 12;
		} else {
			return 14;
		}
	}
	return 0;
}

bool PathFinder::IsVisible(short sX, short sY, short dX, short dY)
{
	int diffx = sX - dX;
	int diffy = sY - dY;
	if (abs( diffx ) >= abs( diffy )) {
		//vertical
		double elevationy = fabs((double)diffx ) / diffy;
		if (sX > dX) {
			for (int startx = sX; startx > dX; startx--) {
				if (Passable[sMap->GetPixelIndex( startx, sY - ( int ) ( ( sX - startx ) / elevationy ) )] & 4)
					return false;
			}
		} else {
			for (int startx = sX; startx < dX; sX++) {
				if (Passable[sMap->GetPixelIndex( startx, sY + ( int ) ( ( sX - startx ) / elevationy ) )] & 4)
					return false;
			}
		}
	} else {
		double elevationx = fabs((double)diffy ) / diffx;
		if (sY > dY) {
			for (int starty = sY; starty > dY; starty--) {
				if (Passable[sMap->GetPixelIndex( sX - ( int ) ( ( sY - starty ) / elevationx ), starty )] & 4)
					return false;
			}
		} else {
			for (int starty = sY; starty < dY; starty++) {
				if (Passable[sMap->GetPixelIndex( sX + ( int ) ( ( sY - starty ) / elevationx ), starty )] & 4)
					return false;
			}
		}
	}
	return true;
}
