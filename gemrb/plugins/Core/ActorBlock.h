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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/ActorBlock.h,v 1.3 2003/12/15 09:18:13 balrog994 Exp $
 *
 */

class Scriptable;
class Selectable;
class Highlightable;
class Moveble;
class Door;

#ifndef ACTORBLOCK_H
#define ACTORBLOCK_H

#include "Sprite2D.h"
#include "CharAnimations.h"
#include "PathFinder.h"
#include "GameScript.h"
#include "Polygon.h"
#include "TileOverlay.h"

#define STEP_TIME		150
#define MAX_SCRIPTS		8

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

typedef enum ScriptableType {
	ST_ACTOR = 0,
	ST_TRIGGER = 1,
	ST_DOOR = 2
} ScriptableType;

class GEM_EXPORT Scriptable {
public:
	Scriptable(ScriptableType type);
	virtual ~Scriptable(void);
public:
	ScriptableType Type;
	unsigned short XPos, YPos;
	Scriptable * MySelf;
    GameScript * Scripts[MAX_SCRIPTS];
	char * overHeadText;
	unsigned char textDisplaying;
	unsigned long timeStartDisplaying;
	char scriptName[33];
public:
	void SetPosition(unsigned short XPos, unsigned short YPos);
	void SetMySelf(Scriptable * MySelf);
	void SetScript(int index, GameScript * script);
	void DisplayHeadText(char * text);
	void SetScriptName(char * text);
};

class GEM_EXPORT Selectable : public Scriptable {
public:
	Selectable(ScriptableType type);
	virtual ~Selectable(void);
public:
	Region BBox;
private:
	bool Selected;
	bool Over;
	Color selectedColor;
	Color overColor;
	int size;
public:
	void SetBBox(Region newBBox);
	void DrawCircle();
	bool IsOver(unsigned short XPos, unsigned short YPos);
	void SetOver(bool over);
	void Select(bool Value);
	void SetCircle(int size, Color color);
};

class GEM_EXPORT Highlightable : public Scriptable {
public:
	Highlightable(ScriptableType type);
	virtual ~Highlightable(void);
public:
	Gem_Polygon * outline;
	Color outlineColor;
public:
	bool IsOver(unsigned short XPos, unsigned short YPos);
	void DrawOutline();
};

class GEM_EXPORT Moveble : public Selectable {
public:
	Moveble(ScriptableType type);
	virtual ~Moveble(void);
	unsigned short XDes, YDes;
	unsigned char Orientation;
	unsigned char AnimID;
	PathNode * path;
	PathNode * step;
	unsigned long timeStartStep;
	Sprite2D * lastFrame;
public:
	void DoStep(ImageMgr * LightMap);
	void WalkTo(unsigned short XDes, unsigned short YDes);
	void MoveTo(unsigned short XDes, unsigned short YDes);
};

/*class ActorBlock : public Moveble {
public:
	ActorBlock(void) : Moveble(ST_ACTOR) { actor = NULL; DeleteMe = false };
	~ActorBlock(void) { if(actor) delete(actor) };
	Actor * actor;
	bool DeleteMe;
} ActorBlock;*/

class GEM_EXPORT Door : public Highlightable {
public:
	Door(TileOverlay * Overlay);
	~Door(void);
public:
	char Name[9];
	TileOverlay * overlay;
	unsigned short * tiles;
	unsigned char count;
	bool DoorClosed;
	Gem_Polygon * open;
	Gem_Polygon * closed;
	unsigned long Cursor;
	char OpenSound[9];
	char CloseSound[9];
private:
	void ToggleTiles(bool playsound = false);
public:
	void SetName(char * Name);
	void SetTiles(unsigned short * Tiles, int count);
	void SetDoorClosed(bool Closed, bool playsound = false);
	void ToggleDoorState();
	void SetPolygon(bool Open, Gem_Polygon * poly);
	void SetCursor(unsigned char CursorIndex);
};

#endif
