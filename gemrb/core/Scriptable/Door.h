/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
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

#ifndef DOOR_H
#define DOOR_H

#include "Scriptable/Scriptable.h"

namespace GemRB {

class TileOverlay;

//door flags
#define DOOR_OPEN        1
#define DOOR_LOCKED      2
#define DOOR_RESET       4   //reset trap
#define DOOR_DETECTABLE  8   //trap detectable
#define DOOR_BROKEN      16  //door is broken (force opened)
#define DOOR_CANTCLOSE   32  //from IE dev source
#define DOOR_LINKED      64   //info trigger linked to this door
#define DOOR_SECRET      128  //door is secret
#define DOOR_FOUND       256  //secret door found
#define DOOR_TRANSPARENT 512  //obscures vision (LOCKEDINFOTEXT in iwd2)
#define DOOR_KEY         1024 //key removed when used (SEETHROUGH in iwd2)
#define DOOR_SLIDE       2048 //impeded blocks ignored (WARNINGINFOTEXT in iwd2)
#define DOOR_WARNINGTEXTDISPLAYED 0x1000 // iwd2
#define DOOR_HIDDEN      8192 // iwd2, ignore the door
#define DOOR_USEUPKEY    0x4000 // treating as identical to DOOR_KEY
#define DOOR_LOCKEDINFOTEXT 0x8000
#define DOOR_WARNINGINFOTEXT 0x10000

class GEM_EXPORT Door : public Highlightable {
public:
	Door(TileOverlay* Overlay);
	~Door(void);
public:
	ieVariable LinkedInfo;
	ieResRef ID; //WED ID
	TileOverlay* overlay;
	unsigned short* tiles;
	int tilecount;
	ieDword Flags;
	int closedIndex;
	//trigger areas
	Gem_Polygon* open;
	Gem_Polygon* closed;
	//impeded blocks
	Point* open_ib; //impeded blocks stored in a Point array
	int oibcount;
	Point* closed_ib;
	int cibcount;
	//wallgroup covers
	unsigned int open_wg_index;
	unsigned int open_wg_count;
	unsigned int closed_wg_index;
	unsigned int closed_wg_count;
	Point toOpen[2];
	ieResRef OpenSound;
	ieResRef CloseSound;
	ieResRef LockSound;
	ieResRef UnLockSound;
	ieDword DiscoveryDiff;
	ieDword LockDifficulty; //this is a dword?
	ieStrRef OpenStrRef;
	ieStrRef NameStrRef;
	ieWord hp, ac;          //unused???, but learned from IE DEV info
private:
	void SetWallgroups(int count, int value);
	void ImpedeBlocks(int count, Point *points, unsigned char value);
	void UpdateDoor();
	bool BlockedOpen(int Open, int ForceOpen);
public:
	void ToggleTiles(int State, int playsound = false);
	void SetName(const char* Name); // sets door ID
	void SetTiles(unsigned short* Tiles, int count);
	void SetDoorLocked(int Locked, int playsound);
	void SetDoorOpen(int Open, int playsound, ieDword ID);
	void SetPolygon(bool Open, Gem_Polygon* poly);
	int IsOpen() const;
	void TryPickLock(Actor *actor);
	void TryBashLock(Actor* actor) ;
	bool TryUnlock(Actor *actor);
	void TryDetectSecret(int skill, ieDword actorID);
	bool Visible();
	void dump() const;
	int TrapResets() const { return Flags & DOOR_RESET; }
	void SetNewOverlay(TileOverlay *Overlay);
};

}

#endif
