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
 */

#include "GUI/WorldMapControl.h"

#include "win32def.h"
#include "ie_cursors.h"

#include "Font.h"
#include "Game.h"
#include "GameData.h"
#include "Interface.h"
#include "Video.h"
#include "WorldMap.h"
#include "GUI/EventMgr.h"
#include "GUI/Window.h"

namespace GemRB {

#define MAP_TO_SCREENX(x) XWin - ScrollX + (x)
#define MAP_TO_SCREENY(y) YWin - ScrollY + (y)

WorldMapControl::WorldMapControl(const Region& frame, const char *font, int direction)
	: Control(frame)
{
	ControlType = IE_GUI_WORLDMAP;
	ScrollX = 0;
	ScrollY = 0;
	MouseIsDown = false;
	Area = NULL;
	Value = direction;
	Game* game = core->GetGame();
	WorldMap* worldmap = core->GetWorldMap();
	CopyResRef(currentArea, game->CurrentArea);
	int entry = core->GetAreaAlias(currentArea);
	if (entry >= 0) {
		WMPAreaEntry *m = worldmap->GetEntry(entry);
		CopyResRef(currentArea, m->AreaResRef);
	}

	//if there is no trivial area, look harder
	if (!worldmap->GetArea(currentArea, (unsigned int &) entry) && 
		core->HasFeature(GF_FLEXIBLE_WMAP) ) {
		WMPAreaEntry *m = worldmap->FindNearestEntry(currentArea, (unsigned int &) entry);
		if (m) {
			CopyResRef(currentArea, m->AreaResRef);
		}
	}

	//this also updates visible locations
	worldmap->CalculateDistances(currentArea, Value);
	
	// alpha bit is unfortunately ignored
	if (font[0]) {
		ftext = core->GetFont(font);
	} else {
		ftext = NULL;
	}

	// initialize label colors
	// NOTE: it would be better to initialize these colors from
	//   some 2da file
	Color normal = { 0xf0, 0xf0, 0xf0, 0xff };
	Color selected = { 0xf0, 0x80, 0x80, 0xff };
	Color notvisited = { 0x80, 0x80, 0xf0, 0xff };
	Color black = { 0x00, 0x00, 0x00, 0x00 };

	pal_normal = core->CreatePalette ( normal, black );
	pal_selected = core->CreatePalette ( selected, black );
	pal_notvisited = core->CreatePalette ( notvisited, black );


	ResetEventHandler( WorldMapControlOnPress );
	ResetEventHandler( WorldMapControlOnEnter );
}

WorldMapControl::~WorldMapControl(void)
{
	//Video *video = core->GetVideoDriver();

	gamedata->FreePalette( pal_normal );
	gamedata->FreePalette( pal_selected );
	gamedata->FreePalette( pal_notvisited );
}

/** Draws the Control on the Output Display */
void WorldMapControl::DrawInternal(Region& rgn)
{
	ieWord XWin = rgn.x;
	ieWord YWin = rgn.y;
	WorldMap* worldmap = core->GetWorldMap();

	Video* video = core->GetVideoDriver();
	Region clipbackup;
	video->GetClipRect(clipbackup);
	video->SetClipRect(&rgn);
	video->BlitSprite( worldmap->GetMapMOS(), MAP_TO_SCREENX(0), MAP_TO_SCREENY(0), true, &rgn );

	unsigned int i;
	unsigned int ec = worldmap->GetEntryCount();
	for(i=0;i<ec;i++) {
		WMPAreaEntry *m = worldmap->GetEntry(i);
		if (! (m->GetAreaStatus() & WMP_ENTRY_VISIBLE)) continue;

		int xOffs = MAP_TO_SCREENX(m->X);
		int yOffs = MAP_TO_SCREENY(m->Y);
		Sprite2D* icon = m->GetMapIcon(worldmap->bam);
		if( icon ) {
			if (m == Area && m->HighlightSelected()) {
				Palette *pal = icon->GetPalette();
				icon->SetPalette(pal_selected);
				video->BlitSprite( icon, xOffs, yOffs, true, &rgn );
				icon->SetPalette(pal);
				pal->release();
			} else {
				video->BlitSprite( icon, xOffs, yOffs, true, &rgn );
			}
			video->FreeSprite( icon );
		}

		if (AnimPicture && (!strnicmp(m->AreaResRef, currentArea, 8)
			|| !strnicmp(m->AreaName, currentArea, 8))) {
			video->BlitSprite( AnimPicture, xOffs, yOffs, true, &rgn );
		}
	}

	// Draw WMP entry labels
	if (ftext==NULL) {
		video->SetClipRect(&clipbackup);
		return;
	}
	for(i=0;i<ec;i++) {
		WMPAreaEntry *m = worldmap->GetEntry(i);
		if (! (m->GetAreaStatus() & WMP_ENTRY_VISIBLE)) continue;
		Sprite2D *icon=m->GetMapIcon(worldmap->bam);
		int h=0,w=0,xpos=0,ypos=0;
		if (icon) {
			h=icon->Height;
			w=icon->Width;
			xpos=icon->XPos;
			ypos=icon->YPos;
			video->FreeSprite( icon );
		}

		Region r2 = Region( MAP_TO_SCREENX(m->X-xpos), MAP_TO_SCREENY(m->Y-ypos), w, h );
		if (!m->GetCaption())
			continue;

		int tw = ftext->CalcStringWidth( (unsigned char*)m->GetCaption() ) + 5;
		int th = ftext->maxHeight;
		
		Palette* text_pal = pal_normal;
		
		if (Area == m) {
			text_pal = pal_selected;
		} else {
			if (! (m->GetAreaStatus() & WMP_ENTRY_VISITED)) {
				text_pal = pal_notvisited;
			}
		}

		ftext->Print( Region( r2.x + (r2.w - tw)/2, r2.y + r2.h, tw, th ),
				( const unsigned char * ) m->GetCaption(), text_pal, 0, true );
	}
	video->SetClipRect(&clipbackup);
}

void WorldMapControl::AdjustScrolling(short x, short y)
{
	WorldMap* worldmap = core->GetWorldMap();
	if (x || y) {
		ScrollX += x;
		ScrollY += y;
	} else {
		//center worldmap on current area
		unsigned entry;

		WMPAreaEntry *m = worldmap->GetArea(currentArea,entry);
		if (m) {
			ScrollX = m->X - Width/2;
			ScrollY = m->Y - Height/2;
		}
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
	MarkDirty();
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
		x =(ieWord) (x + ScrollX);
		y =(ieWord) (y + ScrollY);

		WMPAreaEntry *oldArea = Area;
		Area = NULL;

		unsigned int i;
		unsigned int ec = worldmap->GetEntryCount();
		for (i=0;i<ec;i++) {
			WMPAreaEntry *ae = worldmap->GetEntry(i);

			if ( (ae->GetAreaStatus() & WMP_ENTRY_WALKABLE)!=WMP_ENTRY_WALKABLE) {
				continue; //invisible or inaccessible
			}

			Sprite2D *icon=ae->GetMapIcon(worldmap->bam);
			int h=0, w=0, iconx=0, icony=0;
			if (icon) {
				h=icon->Height;
				w=icon->Width;
				iconx = icon->XPos;
				icony = icon->YPos;
				core->GetVideoDriver()->FreeSprite( icon );
			}
			if (ftext && ae->GetCaption()) {
				int tw = ftext->CalcStringWidth( (unsigned char*)ae->GetCaption() ) + 5;
				int th = ftext->maxHeight;
				if(h<th)
					h=th;
				if(w<tw)
					w=tw;
			}
			if (ae->X - iconx > x) continue;
			if (ae->X - iconx + w < x) continue;
			if (ae->Y - icony > y) continue;
			if (ae->Y - icony + h < y) continue;
			lastCursor = IE_CURSOR_NORMAL;
			Area=ae;
			if(oldArea!=ae) {
				RunEventHandler(WorldMapControlOnEnter);
			}
			break;
		}
	}

	Owner->Cursor = lastCursor;
}

/** Sets the tooltip to be displayed on the screen now */
void WorldMapControl::DisplayTooltip()
{
	if (Area) {
		int x = Owner->XPos+XPos+lastMouseX;
		int y = Owner->YPos+YPos+lastMouseY-50;
		core->DisplayTooltip( x, y, this );
	} else {
		core->DisplayTooltip( 0, 0, NULL );
	}
}

/** Mouse Leave Event */
void WorldMapControl::OnMouseLeave(unsigned short /*x*/, unsigned short /*y*/)
{
	Owner->Cursor = IE_CURSOR_NORMAL;
	Area = NULL;
}

/** Mouse Button Down */
void WorldMapControl::OnMouseDown(unsigned short x, unsigned short y,
	unsigned short Button, unsigned short /*Mod*/)
{
	switch(Button) {
	case GEM_MB_ACTION:
		MouseIsDown = true;
		lastMouseX = x;
		lastMouseY = y;
		break;
	case GEM_MB_SCRLUP:
		OnSpecialKeyPress(GEM_UP);
		break;
	case GEM_MB_SCRLDOWN:
		OnSpecialKeyPress(GEM_DOWN);
		break;
	}
}
/** Mouse Button Up */
void WorldMapControl::OnMouseUp(unsigned short /*x*/, unsigned short /*y*/,
	unsigned short Button, unsigned short /*Mod*/)
{
	if (Button != GEM_MB_ACTION) {
		return;
	}
	MouseIsDown = false;
	if (lastCursor==IE_CURSOR_NORMAL) {
		RunEventHandler( WorldMapControlOnPress );
	}
}

/** Mouse wheel scroll */
void WorldMapControl::OnMouseWheelScroll(short x, short y)
{
	ScrollX += x;
	ScrollY += y;
	
	WorldMap* worldmap = core->GetWorldMap();
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

/** Special Key Press */
bool WorldMapControl::OnSpecialKeyPress(unsigned char Key)
{
	ieDword keyScrollSpd = 64;
	core->GetDictionary()->Lookup("Keyboard Scroll Speed", keyScrollSpd);
	switch (Key) {
		case GEM_LEFT:
			OnMouseWheelScroll(keyScrollSpd * -1, 0);
			break;
		case GEM_UP:
			OnMouseWheelScroll(0, keyScrollSpd * -1);
			break;
		case GEM_RIGHT:
			OnMouseWheelScroll(keyScrollSpd, 0);
			break;
		case GEM_DOWN:
			OnMouseWheelScroll(0, keyScrollSpd);
			break;
		default:
			return false;
	}
	return true;
}

bool WorldMapControl::SetEvent(int eventType, EventHandler handler)
{
	switch (eventType) {
	case IE_GUI_WORLDMAP_ON_PRESS:
		WorldMapControlOnPress = handler;
		break;
	case IE_GUI_MOUSE_ENTER_WORLDMAP:
		WorldMapControlOnEnter = handler;
		break;
	default:
		return false;
	}

	return true;
}

void WorldMapControl::SetColor(int which, Color color)
{
	Palette* pal;
	// FIXME: clearly it can cause palettes to be re-created several times,
	//   because setting background color creates all palettes anew.
	switch (which) {
	case IE_GUI_WMAP_COLOR_BACKGROUND:
		pal = core->CreatePalette( pal_normal->front, color );
		gamedata->FreePalette( pal_normal );
		pal_normal = pal;
		pal = core->CreatePalette( pal_selected->front, color );
		gamedata->FreePalette( pal_selected );
		pal_selected = pal;
		pal = core->CreatePalette( pal_notvisited->front, color );
		gamedata->FreePalette( pal_notvisited );
		pal_notvisited = pal;
		break;
	case IE_GUI_WMAP_COLOR_NORMAL:
		pal = core->CreatePalette( color, pal_normal->back );
		gamedata->FreePalette( pal_normal );
		pal_normal = pal;
		break;
	case IE_GUI_WMAP_COLOR_SELECTED:
		pal = core->CreatePalette( color, pal_selected->back );
		gamedata->FreePalette( pal_selected );
		pal_selected = pal;
		break;
	case IE_GUI_WMAP_COLOR_NOTVISITED:
		pal = core->CreatePalette( color, pal_notvisited->back );
		gamedata->FreePalette( pal_notvisited );
		pal_notvisited = pal;
		break;
	default:
		break;
	}

	MarkDirty();
}

}
