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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Map.h,v 1.27 2004/01/05 23:44:18 balrog994 Exp $
 *
 */

class Map;

#ifndef MAP_H
#define MAP_H

#define INANIMATE		1
#define PC				2
#define FAMILIAR		3
#define ALLY			4
#define CONTROLLED		5
#define CHARMED			6
#define GOODBUTRED		28
#define GOODBUTBLUE		29
#define GOODCUTOFF		30
#define NOTGOOD			31
#define ANYTHING		126
#define NEUTRAL			128
#define NOTEVIL			199
#define EVILCUTOFF		200
#define EVILBUTGREEN	201
#define EVILBUTBLUE		202
#define ENEMY			255

#include "TileMap.h"
#include "ImageMgr.h"
#include "Actor.h"
#include "ScriptedAnimation.h"
#include "GameControl.h"

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
	unsigned long SongList[5];
} SongHeaderType;

typedef struct WallGroup {
	Gem_Polygon ** polys;
	int polygons;
} WallGroup;

typedef struct Entrance {
	char Name[33];
	unsigned int XPos, YPos;
} Entrance;

class GEM_EXPORT Map : public Scriptable
{
public:
	TileMap * tm;
	ImageMgr * LightMap;
	ImageMgr * SearchMap;
	bool justCreated;
private:
	std::vector<Animation*> animations;
	std::vector<Actor*> actors;
	std::vector<WallGroup*> wallGroups;
	std::vector<ScriptedAnimation*> vvcCells;
	std::vector<Entrance*> entrances;
	Actor ** queue[3];
	int Qcount[3];
	int lastActorCount[3];
	void GenerateQueue(int priority);
	Actor * GetRoot(int priority);
	void DeleteActor(Actor * actor);
public:
	Map(void);
	~Map(void);
	void AddTileMap(TileMap * tm, ImageMgr * lm, ImageMgr * sr);
	void DrawMap(Region viewport, GameControl * gc);
	void PlayAreaSong(int);
	void AddAnimation(Animation * anim);
	Animation* GetAnimation(const char * Name);
	void AddActor(Actor *actor);
	void AddWallGroup(WallGroup * wg);
	int GetBlocked(int x, int y);
	Actor * GetActor(int x, int y);
	Actor * GetActor(const char * Name);
	void RemoveActor(Actor * actor);
	int GetActorInRect(Actor ** & actors, Region &rgn);
	SongHeaderType SongHeader;
	void AddVVCCell(ScriptedAnimation * vvc);
	void AddEntrance(char *Name, short XPos, short YPos);
	Entrance * GetEntrance(char *Name);
};

#endif
