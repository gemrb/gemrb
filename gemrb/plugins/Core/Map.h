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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Map.h,v 1.15 2003/12/02 19:48:21 balrog994 Exp $
 *
 */

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
#include "PathFinder.h"

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

typedef struct ActorBlock {
	unsigned short XPos, YPos;
	unsigned short XDes, YDes;
	unsigned short MinX, MaxX, MinY, MaxY;
	unsigned char Orientation;
	unsigned char AnimID;
	Sprite2D * lastFrame;
	Actor * actor;
	bool Selected;
	PathNode * path;
	PathNode * step;
	unsigned long timeStartStep;
} ActorBlock;

class GEM_EXPORT Map
{
public:
	TileMap * tm;
	ImageMgr * LightMap;
	ImageMgr * SearchMap;
private:
	std::vector<Animation*> animations;
	std::vector<ActorBlock> actors;
public:
	Map(void);
	~Map(void);
	void AddTileMap(TileMap * tm, ImageMgr * lm, ImageMgr * sr);
	void DrawMap(Region viewport);
	void PlayAreaSong(int);
	void AddAnimation(Animation * anim);
	void AddActor(ActorBlock actor);
	int GetBlocked(int x, int y);
	ActorBlock * GetActor(int x, int y);
	int GetActorInRect(ActorBlock ** & actors, Region &rgn);
	SongHeaderType SongHeader;
};

#endif
