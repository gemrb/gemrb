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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Window.h,v 1.19 2005/03/19 16:15:58 avenger_teambg Exp $
 *
 */

#ifndef WINDOW_H
#define WINDOW_H

#include "Sprite2D.h"
#include "Control.h"
#include "TextArea.h"
#include "ScrollBar.h"
#include <vector>

/**This class defines a Window as an Items Container.
  *@author GemRB Developement Team
  */

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

// Window position anchors (actually flags for WindowSetPos())
// !!! Keep these synchronized with GUIDefines.py !!!
#define WINDOW_TOPLEFT       0x00
#define WINDOW_CENTER        0x01
#define WINDOW_ABSCENTER     0x02
#define WINDOW_RELATIVE      0x04
#define WINDOW_SCALE         0x08
#define WINDOW_BOUNDED       0x10

// IE specific cursor types
#define IE_CURSOR_NORMAL  0
#define IE_CURSOR_TAKE    2  //over pile type containers
#define IE_CURSOR_WALK    4
#define IE_CURSOR_BLOCKED 6
#define IE_CURSOR_USE     8  //never hardcoded
#define IE_CURSOR_WAIT    10 //hourglass
#define IE_CURSOR_ATTACK  12
#define IE_CURSOR_SWAP    14 //dragging portraits
#define IE_CURSOR_DEFEND  16
#define IE_CURSOR_TALK    18
#define IE_CURSOR_CAST    20 //targeting with non weapon
#define IE_CURSOR_INFO    22 //never hardcoded
#define IE_CURSOR_LOCK    24 //locked door
#define IE_CURSOR_LOCK2   26 //locked container
#define IE_CURSOR_STAIR   28 //never hardcoded
#define IE_CURSOR_DOOR    30 //doors
#define IE_CURSOR_CHEST   32
#define IE_CURSOR_TRAVEL  34
#define IE_CURSOR_STEALTH 36
#define IE_CURSOR_TRAP    38
#define IE_CURSOR_PICK    40 //pickpocket
#define IE_CURSOR_PASS    42 //never hardcoded
#define IE_CURSOR_GRAB    44
#define IE_CURSOR_WAY     46 //waypoint (not in PST)
#define IE_CURSOR_INFO2   46 //PST
#define IE_CURSOR_PORTAL  48 //PST
#define IE_CURSOR_STAIR2  50 //PST
#define IE_CURSOR_EXTRA   52 //PST

class GEM_EXPORT Window {
public: 
	Window(unsigned short WindowID, unsigned short XPos, unsigned short YPos,
		unsigned short Width, unsigned short Height);
	~Window();
	/** Set the Window's BackGround Image. If 'img' is NULL, no background will be set. If the 'clean' parameter is true (default is false) the old background image will be deleted. */
	void SetBackGround(Sprite2D* img, bool clean = false);
	/** Add a Control in the Window */
	void AddControl(Control* ctrl);
	/** This function Draws the Window on the Output Screen */
	void DrawWindow();
	/** Set window frame used to fill screen on higher resolutions*/
	void SetFrame();
	/** Returns the Control at X,Y Coordinates */
	Control* GetControl(unsigned short x, unsigned short y);
	/** Returns the Control by Index */
	Control* GetControl(unsigned short i);
	/** Deletes the xth. Control */
	void DelControl(unsigned short i);
	/** Returns the Default Control which is a button atm */
	Control* GetDefaultControl();
	/** Sets 'ctrl' as Focused */
	void SetFocused(Control* ctrl);
	/** Redraw all the Window */
	void Invalidate();
	/** Redraw controls of the same group */
	void RedrawControls(char* VarName, unsigned int Sum);
	/** Links a scrollbar to a text area */
	void Link(unsigned short SBID, unsigned short TAID);
public: //Public attributes
	/** WinPack */
	char WindowPack[10];
	/** Window ID */
	unsigned short WindowID;
	/** X Position */
	unsigned short XPos;
	/** Y Position */
	unsigned short YPos;
	/** Width */
	unsigned short Width;
	/** Height */
	unsigned short Height;
	/** Visible */
	char Visible;  //0,1,2
	/** Changed Flag */
	bool Changed;
	/** Floating Flag */
	bool Floating;
	/** Window is framed */
	bool Frame;
	int Cursor;
	int DefaultControl;
private: // Private attributes
	/** BackGround Image. No BackGround if this variable is NULL. */
	Sprite2D* BackGround;
	/** Controls Array */
	std::vector< Control*> Controls;
	/** Last Control returned by GetControl */
	Control* lastC;
	/** Last Focused Control */
	Control* lastFocus;
public:
	void release(void);
};

#endif
