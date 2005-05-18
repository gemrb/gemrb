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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/WorldMapControl.cpp,v 1.14 2005/05/18 14:20:16 avenger_teambg Exp $
 */

#ifndef WIN32
#include <sys/time.h>
#endif
#include "../../includes/win32def.h"
#include "WorldMapControl.h"
#include "Interface.h"

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
	Area = NULL;
	if (Value!=(ieDword) -1) {
		Game* game = core->GetGame();
		WorldMap* worldmap = core->GetWorldMap();
		worldmap->CalculateDistances(game->CurrentArea, Value);
	}
	// alpha bit is unfortunately ignored
	Color fore = {0x00, 0x00, 0x00, 0xff};
	Color back = {0x00, 0x00, 0x00, 0x00};
	text_pal = core->GetVideoDriver()->CreatePalette( fore, back );
}

WorldMapControl::~WorldMapControl(void)
{
	if(text_pal) {
		core->GetVideoDriver()->FreePalette(text_pal);
	}
}

/** Draws the Control on the Output Display */
void WorldMapControl::Draw(unsigned short x, unsigned short y)
{
	WorldMap* worldmap = core->GetWorldMap();
	if (!Width || !Height) {
		return;
	}
	if(!Changed)
		return;
	Changed = false;
	Video* video = core->GetVideoDriver();
	Region r( x+XPos, y+YPos, Width, Height );
	video->BlitSprite( worldmap->MapMOS, XPos - ScrollX, YPos - ScrollY, true, &r );

	std::vector< WMPAreaEntry*>::iterator m;

	for (m = worldmap->area_entries.begin(); m != worldmap->area_entries.end(); ++m) {
		if (! ((*m)->AreaStatus & WMP_ENTRY_VISIBLE)) continue;

		if( (*m)->MapIcon) {
			Region r2 = Region( MAP_TO_SCREENX((*m)->X), MAP_TO_SCREENY((*m)->Y), (*m)->MapIcon->Width, (*m)->MapIcon->Height );
			video->BlitSprite( (*m)->MapIcon, MAP_TO_SCREENX((*m)->X), MAP_TO_SCREENY((*m)->Y), true, &r );
		}

		// wmpty.bam
	}

	Font* fnt = core->GetButtonFont();

	// Draw WMP entry labels
	for (m = worldmap->area_entries.begin(); m != worldmap->area_entries.end(); ++m) {
		if (! ((*m)->AreaStatus & WMP_ENTRY_VISIBLE)) continue;
		Sprite2D *icon=(*m)->MapIcon;
		int h=0,w=0;
		if (icon) {
			h=icon->Height;
			w=icon->Width;
		}

		Region r2 = Region( MAP_TO_SCREENX((*m)->X), MAP_TO_SCREENY((*m)->Y), w, h );
		if (r2.y+r2.h<r.y) continue;

		char *text = core->GetString( (*m)->LocCaptionName );
		int tw = fnt->CalcStringWidth( text ) + 5;
		int th = fnt->maxHeight;

		fnt->Print( Region( r2.x + (r2.w - tw)/2, r2.y + r2.h, tw, th ),
				( unsigned char * ) text, text_pal, 0, true );
		free(text);
	}
}

/** Key Release Event */
void WorldMapControl::OnKeyRelease(unsigned char Key, unsigned short Mod)
{
	switch (Key) {
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
}
void WorldMapControl::AdjustScrolling(short x, short y)
{
	WorldMap* worldmap = core->GetWorldMap();
	ScrollX += x;
	ScrollY += y;
	if (ScrollX > worldmap->MapMOS->Width - Width)
		ScrollX = worldmap->MapMOS->Width - Width;
	if (ScrollY > worldmap->MapMOS->Height - Height)
		ScrollY = worldmap->MapMOS->Height - Height;
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
	int nextCursor = IE_CURSOR_GRAB;

	if (MouseIsDown) {
		AdjustScrolling(lastMouseX-x, lastMouseY-y);
	}

	lastMouseX = x;
	lastMouseY = y;

	if (Value!=(ieDword) -1) {
		x += ScrollX;
		y += ScrollY;

		std::vector< WMPAreaEntry*>::iterator m;
		for (m = worldmap->area_entries.begin(); m != worldmap->area_entries.end(); ++m) {
			if (! ((*m)->AreaStatus & WMP_ENTRY_VISIBLE)) continue;

			Sprite2D *icon=(*m)->MapIcon;
			int h=0,w=0;
			if (icon) {
				h=icon->Height;
				w=icon->Width;
			}
			if(h<48)
				h=48;
			if(w<48)
				w=48;
			if ((*m)->X > x) continue;
			if ((*m)->X + w < x) continue;
			if ((*m)->Y > y) continue;
			if ((*m)->Y + h < y) continue;
			nextCursor = IE_CURSOR_NORMAL;
			if(Area!=*m) {
				Area=*m;
				printf("A: %s, Distance: %d\n", Area->AreaName, worldmap->GetDistance(Area->AreaName) );
				break;
			}
		}
	}

	( ( Window * ) Owner )->Cursor = nextCursor;
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

	if (ScrollX > worldmap->MapMOS->Width - Width)
		ScrollX = worldmap->MapMOS->Width - Width;
	if (ScrollY > worldmap->MapMOS->Height - Height)
		ScrollY = worldmap->MapMOS->Height - Height;
	if (ScrollX < 0)
		ScrollX = 0;
	if (ScrollY < 0)
		ScrollY = 0;
}

