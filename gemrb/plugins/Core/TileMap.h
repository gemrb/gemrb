/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/TileMap.h,v 1.7 2003/11/29 07:47:05 balrog994 Exp $
 *
 */

#ifndef TILEMAP_H
#define TILEMAP_H

#include "TileOverlay.h"
#include "Polygon.h"

typedef struct Door {
	char Name[9];
	unsigned short * tiles;
	unsigned char count;
	unsigned char DoorClosed;
	Gem_Polygon * open;
	Gem_Polygon * closed;
	unsigned long Cursor;
	char OpenSound[9];
	char CloseSound[9];
} Door;

typedef struct Container {
	char Name[33];
	Point toOpen;
	unsigned short Type;
	unsigned short LockDifficulty;
	unsigned short Locked;
	unsigned short TrapDetectionDiff;
	unsigned short TrapRemovalDiff;
	unsigned short Trapped;
	unsigned short TrapDetected;
	//Region BBox;
	Point trapTarget;
	Gem_Polygon * outline;
} Container;

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

class GEM_EXPORT TileMap
{
private:
	std::vector<TileOverlay *> overlays;
	std::vector<Door> doors;
	std::vector<Container> containers;
public:
	TileMap(void);
	~TileMap(void);
	void AddOverlay(TileOverlay * overlay);
	Door * AddDoor(char * Name, unsigned char DoorClosed, unsigned short * indexes, int count, Gem_Polygon * open, Gem_Polygon * closed);
	Door * GetDoor(unsigned short x, unsigned short y);
	Container * AddContainer(char * Name, unsigned short Type, Gem_Polygon * outline);
	Container * GetContainer(unsigned short x, unsigned short y);
	void ToogleDoor(Door * door);
	void DrawOverlay(unsigned int index, Region viewport);
public:
	int XCellCount, YCellCount;
};

#endif
