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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Map.h,v 1.44 2004/08/07 00:46:59 avenger_teambg Exp $
 *
 */

class Map;

#ifndef MAP_H
#define MAP_H

#include "TileMap.h"
#include "ImageMgr.h"
#include "Actor.h"
#include "ScriptedAnimation.h"
#include "GameControl.h"
#include "PathFinder.h"
#include <queue>

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

typedef struct SongHeaderType {
	ieDword SongList[5];
} SongHeaderType;

typedef struct WallGroup {
	Gem_Polygon** polys;
	int polygons;
} WallGroup;

typedef struct Entrance {
	char Name[33];
	ieDword XPos, YPos;
	ieDword Face;
} Entrance;

class GEM_EXPORT Map : public Scriptable {
public:
	TileMap* tm;
	ImageMgr* LightMap;
	ImageMgr* SearchMap;
	ieDword AreaFlags;
	ieWord AreaType;
	bool ChangeArea; //set true if movement is allowed between areas
	Variables *vars;
private:
	unsigned short* MapSet;
	std::queue< unsigned int> InternalStack;
	unsigned int Width, Height;
	std::vector< Animation*> animations;
	std::vector< Actor*> actors;
	std::vector< WallGroup*> wallGroups;
	std::vector< ScriptedAnimation*> vvcCells;
	std::vector< Entrance*> entrances;
	Actor** queue[3];
	int Qcount[3];
	unsigned int lastActorCount[3];
	void GenerateQueue(int priority);
	Actor* GetRoot(int priority);
	void DeleteActor(Actor* actor);
public:
	Map(void);
	~Map(void);
	/** prints useful information on console */
	void DebugDump();
	void AddTileMap(TileMap* tm, ImageMgr* lm, ImageMgr* sr);
	void CreateMovement(char *command, const char *area, const char *entrance);
	void DrawMap(Region viewport, GameControl* gc);
	void PlayAreaSong(int);
	void AddAnimation(Animation* anim);
	Animation* GetAnimation(const char* Name);
	void AddActor(Actor* actor);
	void AddWallGroup(WallGroup* wg);
	int GetBlocked(unsigned int x, unsigned int y);
	Actor* GetActor(unsigned int x, unsigned int y, int flags);
	Actor* GetActor(const char* Name);
	Actor* GetActorByDialog(const char* resref);
	void RemoveActor(Actor* actor);
	//returns actors in rect (onlyparty could be more sophisticated)
	int GetActorInRect(Actor**& actors, Region& rgn, bool onlyparty);
	SongHeaderType SongHeader;
	void AddVVCCell(ScriptedAnimation* vvc);
	void AddEntrance(char* Name, short XPos, short YPos, short Face);
	Entrance* GetEntrance(const char* Name);
	bool CanFree();
	int GetActorCount() { return (int)actors.size(); }
	Actor* GetActor(int i) { return actors[i]; }

	//PathFinder
	/* Finds the nearest passable point */
	void AdjustPosition(unsigned int& goalX, unsigned int& goalY, unsigned int radius=0);
	/* Finds the path which leads the farthest from d */
	PathNode* RunAway(short sX, short sY, short dX, short dY, unsigned int PathLen, bool Backing);
	/* Returns true if there is no path to d */
	bool TargetUnreachable(short sX, short sY, short dX, short dY);
	/* Finds the path which leads to d */
	PathNode* FindPath(short sX, short sY, short dX, short dY);
	bool IsVisible(short sX, short sY, short dX, short dY);
private:
	void Leveldown(unsigned int px, unsigned int py, unsigned int& level,
		unsigned int& nx, unsigned int& ny, unsigned int& diff);
	void SetupNode(unsigned int x, unsigned int y, unsigned int Cost);
	//maybe this is unneeded and orientation could be calculated on the fly
	unsigned char GetOrient(short sX, short sY, short dX, short dY);
	void UseExit(Actor *pc, InfoPoint *ip);
};

#endif
