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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/WorldMapControl.cpp,v 1.2 2004/08/20 13:32:37 avenger_teambg Exp $
 */

#ifndef WIN32
#include <sys/time.h>
#endif
#include "../../includes/win32def.h"
#include "WorldMapControl.h"
#include "Interface.h"
//#include "AnimationMgr.h"
//#include "../../includes/strrefs.h"

extern Interface* core;
static Color green = {
	0x00, 0xff, 0x00, 0xff
};

#define MAP_TO_SCREENX(x) XPos - ScrollX + (x)
#define MAP_TO_SCREENY(y) YPos - ScrollY + (y)

#define SCREEN_TO_MAPX(x) (x) - XPos + ScrollX
#define SCREEN_TO_MAPY(y) (y) - YPos + ScrollY

WorldMapControl::WorldMapControl(void)
{
	ScrollX = 0;
	ScrollY = 0;
	MouseIsDown = false;
	Changed = true;
}


WorldMapControl::~WorldMapControl(void)
{
}

/** Draws the Control on the Output Display */
void WorldMapControl::Draw(unsigned short /*x*/, unsigned short /*y*/)
{
	WorldMap* worldmap = core->GetWorldMap();
	if (!Width || !Height) {
		return;
	}
	Video* video = core->GetVideoDriver();
	Region r( XPos, YPos, Width, Height );
	video->BlitSprite( worldmap->MapMOS, XPos - ScrollX, YPos - ScrollY, true, &r );


	std::vector< WMPAreaEntry*>::iterator m;
	unsigned int xm = SCREEN_TO_MAPX(lastMouseX + XPos);
	unsigned int ym = SCREEN_TO_MAPY(lastMouseY + YPos);


	for (m = worldmap->area_entries.begin(); m != worldmap->area_entries.end(); ++m) {
		Region r2 = Region( MAP_TO_SCREENX((*m)->X), MAP_TO_SCREENY((*m)->Y), (*m)->MapIcon->Width, (*m)->MapIcon->Height );
		if (xm >= (*m)->X && xm < (*m)->X + (*m)->MapIcon->Width && ym >= (*m)->Y && ym < (*m)->Y + (*m)->MapIcon->Height)
			video->BlitSprite( (*m)->MapIcon, MAP_TO_SCREENX((*m)->X), MAP_TO_SCREENY((*m)->Y), true, &r );
		video->DrawRect ( r2, green, false, false );
		// wmpty.bam
	}

}

#if 0
/** Key Press Event */
void WorldmapControl::OnKeyPress(unsigned char Key, unsigned short Mod)
{
	HotKey=tolower(Key);
}
#endif

/** Key Release Event */
void WorldMapControl::OnKeyRelease(unsigned char Key, unsigned short Mod)
{
	//unsigned int i;

	switch (Key) {
		case '\t':
			//not GEM_TAB
			printf( "TAB released\n" );
			return;
	        case 'f':
			if (Mod & 64)
				core->GetVideoDriver()->ToggleFullscreenMode();
			break;
		case 'g':
			if (Mod & 64)
				core->GetVideoDriver()->ToggleGrabInput();
			break;
		default:
			break;
	}
	if (!core->CheatEnabled()) {
		return;
	}
	if (Mod & 64) //ctrl
	{
	}
}
/** Mouse Over Event */
void WorldMapControl::OnMouseOver(unsigned short x, unsigned short y)
{
	WorldMap* worldmap = core->GetWorldMap();
	//int nextCursor = 0;

	if (MouseIsDown) {
		ScrollX -= x - lastMouseX;
		ScrollY -= y - lastMouseY;

		if (ScrollX > worldmap->MapMOS->Width - Width)
			ScrollX = worldmap->MapMOS->Width - Width;
		if (ScrollY > worldmap->MapMOS->Height - Height)
			ScrollY = worldmap->MapMOS->Height - Height;
		if (ScrollX < 0)
			ScrollX = 0;
		if (ScrollY < 0)
			ScrollY = 0;
	}

	lastMouseX = x;
	lastMouseY = y;


	x += ScrollX;
	y += ScrollY;

	std::vector< WMPAreaEntry*>::iterator m;
	for (m = worldmap->area_entries.begin(); m != worldmap->area_entries.end(); ++m) {
		if ((*m)->X <= x && (*m)->X + (*m)->MapIcon->Width > x && (*m)->Y <= y && (*m)->Y + (*m)->MapIcon->Height > y)
			printf("A: %s\n", (*m)->AreaName);
	}


#if 0
	short WorldmapX = x, WorldmapY = y;
	core->GetVideoDriver()->ConvertToWorldmap( WorldmapX, WorldmapY );
	if (MouseIsDown && ( !DrawSelectionRect )) {
		if (( abs( WorldmapX - StartX ) > 5 ) || ( abs( WorldmapY - StartY ) > 5 )) {
			DrawSelectionRect = true;
		}
	}
	Worldmap* worldmap = core->GetWorldmap();
	Map* area = worldmap->GetCurrentMap( );

	switch (area->GetBlocked( WorldmapX, WorldmapY ) & 3) {
		case 0:
			nextCursor = 6;
			break;

		case 1:
			nextCursor = 4;
			break;

		case 2:
		case 3:
			nextCursor = 34;
			break;
	}

	overInfoPoint = area->tm->GetInfoPoint( WorldmapX, WorldmapY );
	if (overInfoPoint) {
		if (overInfoPoint->Type != ST_PROXIMITY) {
			nextCursor = overInfoPoint->Cursor;
		}
	}

	if (overDoor) {
		overDoor->Highlight = false;
	}
	overDoor = area->tm->GetDoor( WorldmapX, WorldmapY );
	if (overDoor) {
		overDoor->Highlight = true;
		nextCursor = overDoor->Cursor;
		overDoor->outlineColor = cyan;
	}

	if (overContainer) {
		overContainer->Highlight = false;
	}
	overContainer = area->tm->GetContainer( WorldmapX, WorldmapY );
	if (overContainer) {
		overContainer->Highlight = true;
		if (overContainer->TrapDetected && overContainer->Trapped) {
			nextCursor = 38;
			overContainer->outlineColor = red;
		} else {
			nextCursor = 2;
			overContainer->outlineColor = cyan;
		}
	}


	if (lastCursor != nextCursor) {
		( ( Window * ) Owner )->Cursor = nextCursor;
		lastCursor = nextCursor;
	}
#endif
}

#if 0
bool WorldMapControl::HandleActiveRegion(InfoPoint *trap, Actor *actor)
{
	switch(trap->Type) {
		case ST_TRAVEL:
			trap->Flags|=TRAP_RESET;
			return false;
		case ST_TRIGGER:
			//the importer shouldn't load the script
			//if it is unallowed anyway (though 
			//deactivated scripts could be reactivated)
			//only the 'trapped' flag should be honoured
			//there. Here we have to check on the 
			//reset trap and deactivated flags
			if (trap->Scripts[0]) {
				if(!(trap->Flags&TRAP_DEACTIVATED) ) {
					trap->LastTrigger = selected[0];
					trap->Scripts[0]->Update();
					//if reset trap flag not set, deactivate it
					if(!(trap->Flags&TRAP_RESET)) {
						trap->Flags|=TRAP_DEACTIVATED;
					}
				}
			} else {
				if (trap->overHeadText) {
					if (trap->textDisplaying != 1) {
						trap->textDisplaying = 1;
						GetTime( trap->timeStartDisplaying );
						DisplayString( trap );
					}
				}
			}
			return true;
		default:;
	}
	return false;
}
#endif
/** Mouse Button Down */
void WorldMapControl::OnMouseDown(unsigned short x, unsigned short y,
	unsigned char Button, unsigned short /*Mod*/)
{
	if ((Button != GEM_MB_ACTION) ) {
		return;
	}
	//short WorldmapX = x, WorldmapY = y;
	//core->GetVideoDriver()->ConvertToWorldmap( WorldmapX, WorldmapY );
	MouseIsDown = true;
	lastMouseX = x;
	lastMouseY = y;
}
/** Mouse Button Up */
void WorldMapControl::OnMouseUp(unsigned short /*x*/, unsigned short /*y*/,
	unsigned char Button, unsigned short /*Mod*/)
{
	if (Button != GEM_MB_ACTION) {
		return;
	}

	MouseIsDown = false;
#if 0
		if(overInfoPoint) {
			if(HandleActiveRegion(overInfoPoint, selected[0])) {
				return;
			}
		}
#endif
}

/** Special Key Press */
void WorldMapControl::OnSpecialKeyPress(unsigned char Key)
{
	WorldMap* worldmap = core->GetWorldMap();
	switch (Key) {
		case GEM_LEFT:
			ScrollX -= 64;
			break;
		case GEM_UP:
			ScrollY -= 64;
			break;
		case GEM_RIGHT:
			ScrollX += 64;
			break;
		case GEM_DOWN:
			ScrollY += 64;
			break;
		case GEM_ALT:
			printf( "ALT pressed\n" );
			break;
		case GEM_TAB:
			printf( "TAB pressed\n" );
			break;
	}

	if (ScrollX > worldmap->MapMOS->Width - Width)
		ScrollX = worldmap->MapMOS->Width - Width;
	if (ScrollY > worldmap->MapMOS->Height - Height)
		ScrollY = worldmap->MapMOS->Height - Height;
	if (ScrollX < 0)
		ScrollX = 0;
	if (ScrollY < 0)
		ScrollY = 0;
}

#if 0
void WorldmapControl::DisplayString(int X, int Y, const char *Text)
{
	Scriptable* scr = new Scriptable( ST_TRIGGER );
	scr->overHeadText = (char *) Text;
	scr->textDisplaying = 1;
	scr->timeStartDisplaying = 0;
	scr->XPos = X;
	scr->YPos = Y;
	scr->MySelf = NULL;
	infoTexts.push_back( scr );
}


void WorldmapControl::DisplayString(const char* Text)
{
	unsigned long WinIndex, TAIndex;

	core->GetDictionary()->Lookup( "MessageWindow", WinIndex );
	if (( WinIndex != (unsigned long) -1 ) &&
		( core->GetDictionary()->Lookup( "MessageTextArea", TAIndex ) )) {
		Window* win = core->GetWindow( WinIndex );
		if (win) {
			TextArea* ta = ( TextArea* ) win->GetControl( TAIndex );
			ta->AppendText( Text, -1 );
		}
	}
}

#endif
