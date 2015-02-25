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

#include "Game.h"
#include "GameData.h"
#include "Interface.h"
#include "WorldMap.h"
#include "GUI/EventMgr.h"
#include "GUI/TextSystem/Font.h"
#include "GUI/Window.h"

namespace GemRB {

WorldMapControl::WorldMapControl(const Region& frame, const char *font, int direction)
	: Control(frame)
{
	ControlType = IE_GUI_WORLDMAP;
	ScrollX = 0;
	ScrollY = 0;
	MouseIsDown = false;
	lastCursor = 0;
	Area = NULL;
	Value = direction;
	OverrideIconPalette = false;
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

	pal_normal = new Palette ( normal, black );
	pal_selected = new Palette ( selected, black );
	pal_notvisited = new Palette ( notvisited, black );


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
void WorldMapControl::DrawSelf(Region rgn, const Region& /*clip*/)
{
#define MAP_TO_SCREENX(x) XWin - ScrollX + (x)
#define MAP_TO_SCREENY(y) YWin - ScrollY + (y)
	ieWord XWin = rgn.x;
	ieWord YWin = rgn.y;
	WorldMap* worldmap = core->GetWorldMap();

	Video* video = core->GetVideoDriver();
	video->BlitSprite( worldmap->GetMapMOS(), MAP_TO_SCREENX(0), MAP_TO_SCREENY(0), true, &rgn );

	unsigned int i;
	unsigned int ec = worldmap->GetEntryCount();
	for(i=0;i<ec;i++) {
		WMPAreaEntry *m = worldmap->GetEntry(i);
		if (! (m->GetAreaStatus() & WMP_ENTRY_VISIBLE)) continue;

		int xOffs = MAP_TO_SCREENX(m->X);
		int yOffs = MAP_TO_SCREENY(m->Y);
		Sprite2D* icon = m->GetMapIcon(worldmap->bam, OverrideIconPalette);
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
			Sprite2D::FreeSprite( icon );
		}

		if (AnimPicture && (!strnicmp(m->AreaResRef, currentArea, 8)
			|| !strnicmp(m->AreaName, currentArea, 8))) {
			video->BlitSprite( AnimPicture, xOffs, yOffs, true, &rgn );
		}
	}

	// Draw WMP entry labels
	if (ftext==NULL) {
		return;
	}
	for(i=0;i<ec;i++) {
		WMPAreaEntry *m = worldmap->GetEntry(i);
		if (! (m->GetAreaStatus() & WMP_ENTRY_VISIBLE)) continue;
		Sprite2D *icon=m->GetMapIcon(worldmap->bam, OverrideIconPalette);
		int h=0,w=0,xpos=0,ypos=0;
		if (icon) {
			h=icon->Height;
			w=icon->Width;
			xpos=icon->XPos;
			ypos=icon->YPos;
			Sprite2D::FreeSprite( icon );
		}

		Region r2 = Region( MAP_TO_SCREENX(m->X-xpos), MAP_TO_SCREENY(m->Y-ypos), w, h );
		if (!m->GetCaption())
			continue;

		Palette* text_pal = pal_normal;
		
		if (Area == m) {
			text_pal = pal_selected;
		} else {
			if (! (m->GetAreaStatus() & WMP_ENTRY_VISITED)) {
				text_pal = pal_notvisited;
			}
		}

		Size ts = ftext->StringSize(*m->GetCaption());
		ts.w += 10;
		ftext->Print( Region( Point(r2.x + (r2.w - ts.w)/2, r2.y + r2.h), ts ),
					 *m->GetCaption(), text_pal, 0 );
	}
#undef MAP_TO_SCREENX
#undef MAP_TO_SCREENY
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
			ScrollX = m->X - frame.w / 2;
			ScrollY = m->Y - frame.h / 2;
		}
	}
	Sprite2D *MapMOS = worldmap->GetMapMOS();
	if (ScrollX > MapMOS->Width - frame.w)
		ScrollX = MapMOS->Width - frame.w;
	if (ScrollY > MapMOS->Height - frame.h)
		ScrollY = MapMOS->Height - frame.h;
	if (ScrollX < 0)
		ScrollX = 0;
	if (ScrollY < 0)
		ScrollY = 0;
	MarkDirty();
	Area = NULL;
}

/** Mouse Over Event */
void WorldMapControl::OnMouseOver(const Point& p)
{
	WorldMap* worldmap = core->GetWorldMap();
	lastCursor = IE_CURSOR_GRAB;

	if (MouseIsDown) {
		AdjustScrolling(LastMousePos.x - p.x, LastMousePos.y - p.y);
	}

	LastMousePos = p;

	if (Value!=(ieDword) -1) {
		Point mapOff = p + Point(ScrollX, ScrollY);

		WMPAreaEntry *oldArea = Area;
		Area = NULL;

		unsigned int i;
		unsigned int ec = worldmap->GetEntryCount();
		for (i=0;i<ec;i++) {
			WMPAreaEntry *ae = worldmap->GetEntry(i);

			if ( (ae->GetAreaStatus() & WMP_ENTRY_WALKABLE)!=WMP_ENTRY_WALKABLE) {
				continue; //invisible or inaccessible
			}

			Sprite2D *icon=ae->GetMapIcon(worldmap->bam, OverrideIconPalette);
			Region rgn(ae->X, ae->Y, 0, 0);
			if (icon) {
				rgn.x -= icon->XPos;
				rgn.y -= icon->YPos;
				rgn.w = icon->Width;
				rgn.h = icon->Height;
				Sprite2D::FreeSprite( icon );
			}
			if (ftext && ae->GetCaption()) {
				Size ts = ftext->StringSize(*ae->GetCaption());
				ts.w += 10;
				if(rgn.h < ts.h)
					rgn.h = ts.h;
				if(rgn.w < ts.w)
					rgn.w = ts.w;
			}
			if (!rgn.PointInside(mapOff)) continue;

			lastCursor = IE_CURSOR_NORMAL;
			Area=ae;
			if(oldArea!=ae) {
				String* str = core->GetString(23084);
				if (str) {
					wchar_t dist[10];
					swprintf(dist, 10, L": %d", worldmap->GetDistance(Area->AreaName));
					SetTooltip(*str + dist);
				}
			}
			break;
		}
		if (Area == NULL) {
			SetTooltip(L"");
		}
	}

	Owner->Cursor = lastCursor;
}

/** Mouse Leave Event */
void WorldMapControl::OnMouseLeave(const Point& /*p*/, const DragOp*)
{
	Owner->Cursor = IE_CURSOR_NORMAL;
	Area = NULL;
}

/** Mouse Button Down */
void WorldMapControl::OnMouseDown(const Point& p, unsigned short Button, unsigned short /*Mod*/)
{
	switch(Button) {
	case GEM_MB_ACTION:
		MouseIsDown = true;
		LastMousePos = p;
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
void WorldMapControl::OnMouseUp(const Point& p, unsigned short Button, unsigned short /*Mod*/)
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
	if (ScrollX > MapMOS->Width - frame.w)
		ScrollX = MapMOS->Width - frame.w;
	if (ScrollY > MapMOS->Height - frame.h)
		ScrollY = MapMOS->Height - frame.h;
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

bool WorldMapControl::SetEvent(int eventType, ControlEventHandler handler)
{
	switch (eventType) {
	case IE_GUI_WORLDMAP_ON_PRESS:
		WorldMapControlOnPress = handler;
		return true;
	}

	return false;
}

void WorldMapControl::SetColor(int which, Color color)
{
	Palette* pal;
	// FIXME: clearly it can cause palettes to be re-created several times,
	//   because setting background color creates all palettes anew.
	switch (which) {
	case IE_GUI_WMAP_COLOR_BACKGROUND:
		pal = new Palette( pal_normal->front, color );
		gamedata->FreePalette( pal_normal );
		pal_normal = pal;
		pal = new Palette( pal_selected->front, color );
		gamedata->FreePalette( pal_selected );
		pal_selected = pal;
		pal = new Palette( pal_notvisited->front, color );
		gamedata->FreePalette( pal_notvisited );
		pal_notvisited = pal;
		break;
	case IE_GUI_WMAP_COLOR_NORMAL:
		pal = new Palette( color, pal_normal->back );
		gamedata->FreePalette( pal_normal );
		pal_normal = pal;
		break;
	case IE_GUI_WMAP_COLOR_SELECTED:
		pal = new Palette( color, pal_selected->back );
		gamedata->FreePalette( pal_selected );
		pal_selected = pal;
		break;
	case IE_GUI_WMAP_COLOR_NOTVISITED:
		pal = new Palette( color, pal_notvisited->back );
		gamedata->FreePalette( pal_notvisited );
		pal_notvisited = pal;
		break;
	default:
		break;
	}

	MarkDirty();
}

}
