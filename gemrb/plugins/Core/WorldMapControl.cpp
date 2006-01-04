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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/WorldMapControl.cpp,v 1.22 2006/01/04 16:34:06 avenger_teambg Exp $
 */

#ifndef WIN32
#include <sys/time.h>
#endif
#include "../../includes/win32def.h"
#include "WorldMapControl.h"
#include "Interface.h"
#include "Video.h"
#include "Game.h"
#include "WorldMap.h"

#define MAP_TO_SCREENX(x) XWin + XPos - ScrollX + (x)
#define MAP_TO_SCREENY(y) YWin + YPos - ScrollY + (y)

WorldMapControl::WorldMapControl(const char *font, int direction)
{
	ScrollX = 0;
	ScrollY = 0;
	MouseIsDown = false;
	Changed = true;
	Area = NULL;
	Value = direction;
	if (Value!=(ieDword) -1) {
		Game* game = core->GetGame();
		WorldMap* worldmap = core->GetWorldMap();
		worldmap->CalculateDistances(game->CurrentArea, Value);
		strncpy(ca, game->CurrentArea, 8);
	}
	
	// alpha bit is unfortunately ignored
	if (font[0]) {
		ftext = core->GetFont(font);
	} else {
		ftext = NULL;
	}
	ResetEventHandler( WorldMapControlOnPress );
	ResetEventHandler( WorldMapControlOnEnter );
}

WorldMapControl::~WorldMapControl(void)
{
}

/** Draws the Control on the Output Display */
void WorldMapControl::Draw(unsigned short XWin, unsigned short YWin)
{
	WorldMap* worldmap = core->GetWorldMap();
	if (!Width || !Height) {
		return;
	}
	if(!Changed)
		return;
	Changed = false;
	Video* video = core->GetVideoDriver();
	Region r( XWin+XPos, YWin+YPos, Width, Height );
	video->BlitSprite( worldmap->GetMapMOS(), MAP_TO_SCREENX(0), MAP_TO_SCREENY(0), true, &r );

	unsigned int i;
	unsigned int ec = worldmap->GetEntryCount();
	for(i=0;i<ec;i++) {
		WMPAreaEntry *m = worldmap->GetEntry(i);
		if (! (m->GetAreaStatus() & WMP_ENTRY_VISIBLE)) continue;

		short xOffs = MAP_TO_SCREENX(m->X);
		short yOffs = MAP_TO_SCREENY(m->Y);
		if( m->MapIcon) {
			video->BlitSprite( m->MapIcon, xOffs, yOffs, true, &r );
		}

		if (AnimPicture && !strnicmp(m->AreaResRef, ca, 8) ) {
			core->GetVideoDriver()->BlitSprite( AnimPicture, xOffs, yOffs, true, &r );
		}
	}

	// Draw WMP entry labels
	if (ftext==NULL) {
		return;
	}
	for(i=0;i<ec;i++) {
		WMPAreaEntry *m = worldmap->GetEntry(i);
		if (! (m->GetAreaStatus() & WMP_ENTRY_VISIBLE)) continue;
		Sprite2D *icon=m->MapIcon;
		int h=0,w=0,xpos=0,ypos=0;
		if (icon) {
			h=icon->Height;
			w=icon->Width;
			xpos=icon->XPos;
			ypos=icon->YPos;
		}

		Region r2 = Region( MAP_TO_SCREENX(m->X-xpos), MAP_TO_SCREENY(m->Y-ypos), w, h );
		if (r2.y+r2.h<r.y) continue;

		char *text = core->GetString( m->LocCaptionName );
		int tw = ftext->CalcStringWidth( text ) + 5;
		int th = ftext->maxHeight;
		
		Color fore = {0xf0, 0xf0, 0xf0, 0xff};
		Color back = {0x00, 0x00, 0x00, 0x00};
		
		if (Area == m) {
			//if mouse is over it, then make it pale red
			fore.b = 0x80;
			fore.g = 0x80;
		} else {
			if (! (m->GetAreaStatus() & WMP_ENTRY_VISITED)) {
				//if not visited, make it pale blue
				fore.r=0x80;
				fore.g=0x80;
			}
		} //otherwise leave it white

		Color *text_pal = video->CreatePalette( fore, back );

		ftext->Print( Region( r2.x + (r2.w - tw)/2, r2.y + r2.h, tw, th ),
				( unsigned char * ) text, text_pal, 0, true );
		free(text);
		video->FreePalette(text_pal);
	}
}

/** Key Release Event */
void WorldMapControl::OnKeyRelease(unsigned char Key, unsigned short Mod)
{
	switch (Key) {
		case 'f':
			if (Mod & GEM_MOD_CTRL)
				core->GetVideoDriver()->ToggleFullscreenMode();
			break;
		case 'g':
			if (Mod & GEM_MOD_CTRL)
				core->GetVideoDriver()->ToggleGrabInput();
			break;
		default:
			break;
	}
}
void WorldMapControl::AdjustScrolling(short x, short y)
{
	WorldMap* worldmap = core->GetWorldMap();
	ScrollX += x;
	ScrollY += y;
	Sprite2D *MapMOS = worldmap->GetMapMOS();
	if (ScrollX > MapMOS->Width - Width)
		ScrollX = MapMOS->Width - Width;
	if (ScrollY > MapMOS->Height - Height)
		ScrollY = MapMOS->Height - Height;
	if (ScrollX < 0)
		ScrollX = 0;
	if (ScrollY < 0)
		ScrollY = 0;
	Changed = true;
	Area = NULL;
}

/** Mouse Over Event */
void WorldMapControl::OnMouseOver(unsigned short x, unsigned short y)
{
	WorldMap* worldmap = core->GetWorldMap();
	lastCursor = IE_CURSOR_GRAB;

	if (MouseIsDown) {
		AdjustScrolling(lastMouseX-x, lastMouseY-y);
	}

	lastMouseX = x;
	lastMouseY = y;

	if (Value!=(ieDword) -1) {
		x += ScrollX;
		y += ScrollY;

		WMPAreaEntry *oldArea = Area;
		Area = NULL;

		unsigned int i;
		unsigned int ec = worldmap->GetEntryCount();
		for (i=0;i<ec;i++) {
			WMPAreaEntry *ae = worldmap->GetEntry(i);

			if ( (ae->GetAreaStatus() & WMP_ENTRY_WALKABLE)!=WMP_ENTRY_WALKABLE) {
				continue; //invisible or inaccessible
			}
			if (!strnicmp(ae->AreaResRef, ca, 8) ) {
				continue; //current area
			}

			Sprite2D *icon=ae->MapIcon;
			int h=0,w=0;
			if (icon) {
				h=icon->Height;
				w=icon->Width;
			}
			if (ftext) {
				char *text = core->GetString( ae->LocCaptionName );
				int tw = ftext->CalcStringWidth( text ) + 5;
				int th = ftext->maxHeight;
				if(h<th)
					h=th;        
				if(w<tw)
					w=tw;
			}
			if (ae->X > x) continue;
			if (ae->X + w < x) continue;
			if (ae->Y > y) continue;
			if (ae->Y + h < y) continue;
			lastCursor = IE_CURSOR_NORMAL;
			Area=ae;
			if(oldArea!=ae) {
				RunEventHandler(WorldMapControlOnEnter);
			}
			break;
		}
	}

	( ( Window * ) Owner )->Cursor = lastCursor;
}

/** Sets the tooltip to be displayed on the screen now */
void WorldMapControl::DisplayTooltip()
{
	if (Area) {
		int x = ((Window*) Owner)->XPos+XPos+lastMouseX;
		int y = ((Window*) Owner)->YPos+YPos+lastMouseY-50;
		core->DisplayTooltip( x, y, this );
	} else {
		core->DisplayTooltip( 0, 0, NULL );
	}
}

/** Mouse Leave Event */
void WorldMapControl::OnMouseLeave(unsigned short /*x*/, unsigned short /*y*/)
{
	( ( Window * ) Owner )->Cursor = IE_CURSOR_NORMAL;
	Area = NULL;
}

/** Mouse Button Down */
void WorldMapControl::OnMouseDown(unsigned short x, unsigned short y,
	unsigned char Button, unsigned short /*Mod*/)
{
	if ((Button != GEM_MB_ACTION) ) {
		return;
	}
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
	if (lastCursor==IE_CURSOR_NORMAL) {
		RunEventHandler( WorldMapControlOnPress );
	}
	MouseIsDown = false;
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

	Sprite2D *MapMOS = worldmap->GetMapMOS();
	if (ScrollX > MapMOS->Width - Width)
		ScrollX = MapMOS->Width - Width;
	if (ScrollY > MapMOS->Height - Height)
		ScrollY = MapMOS->Height - Height;
	if (ScrollX < 0)
		ScrollX = 0;
	if (ScrollY < 0)
		ScrollY = 0;
}

bool WorldMapControl::SetEvent(int eventType, EventHandler handler)
{
	Changed = true;

	switch (eventType) {
	case IE_GUI_WORLDMAP_ON_PRESS:
		SetEventHandler( WorldMapControlOnPress, handler );
		break;
	case IE_GUI_MOUSE_ENTER_WORLDMAP:
		SetEventHandler( WorldMapControlOnEnter, handler );
		break;
	default:
		return Control::SetEvent( eventType, handler );
	}

	return true;
}
