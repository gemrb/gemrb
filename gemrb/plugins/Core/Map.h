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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Map.h,v 1.6 2003/11/26 14:02:16 balrog994 Exp $
 *
 */

#ifndef MAP_H
#define MAP_H

#include "TileMap.h"
#include "ImageMgr.h"
#include "Actor.h"

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

typedef struct ActorBlock {
	unsigned short XPos, YPos;
	unsigned short XDes, YDes;
	unsigned short MinX, MaxX, MinY, MaxY;
	unsigned char Orientation;
	unsigned char AnimID;
	Sprite2D * lastFrame;
	Actor * actor;
} ActorBlock;

class GEM_EXPORT Map
{
private:
	TileMap * tm;
	ImageMgr * LightMap;
	std::vector<Animation*> animations;
	std::vector<ActorBlock> actors;
public:
	Map(void);
	~Map(void);
	void AddTileMap(TileMap * tm, ImageMgr * lm);
	void DrawMap(Region viewport);
	void AddAnimation(Animation * anim);
	void AddActor(ActorBlock actor);
	Actor * GetActor(int x, int y);
};

#endif
