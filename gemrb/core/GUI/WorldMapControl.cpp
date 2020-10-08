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
	Area = NULL;
	SetValue(direction);
	SetCursor(core->Cursors[IE_CURSOR_GRAB]);
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
	worldmap->CalculateDistances(currentArea, direction);

	// alpha bit is unfortunately ignored
	if (font[0]) {
		ftext = core->GetFont(font);
	} else {
		ftext = NULL;
	}
	
	SetColor(IE_GUI_WMAP_COLOR_BACKGROUND, ColorBlack);
}

/** Draws the Control on the Output Display */
void WorldMapControl::DrawSelf(Region rgn, const Region& /*clip*/)
{
#define MAP_TO_SCREENX(i) rgn.x - Pos.x + (i)
#define MAP_TO_SCREENY(i) rgn.y - Pos.y + (i)
	WorldMap* worldmap = core->GetWorldMap();

	Video* video = core->GetVideoDriver();
	video->BlitSprite( worldmap->GetMapMOS(), MAP_TO_SCREENX(0), MAP_TO_SCREENY(0), &rgn );

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
				PaletteHolder pal = icon->GetPalette();
				icon->SetPalette(pal_selected.get());
				video->BlitSprite( icon, xOffs, yOffs, &rgn );
				icon->SetPalette(pal);
			} else {
				video->BlitSprite( icon, xOffs, yOffs, &rgn );
			}
			Sprite2D::FreeSprite( icon );
		}

		if (AnimPicture && (!strnicmp(m->AreaResRef, currentArea, 8)
			|| !strnicmp(m->AreaName, currentArea, 8))) {
			video->BlitSprite( AnimPicture.get(), xOffs, yOffs, &rgn );
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
			h=icon->Frame.h;
			w=icon->Frame.w;
			xpos=icon->Frame.x;
			ypos=icon->Frame.y;
			Sprite2D::FreeSprite( icon );
		}

		Region r2 = Region( MAP_TO_SCREENX(m->X-xpos), MAP_TO_SCREENY(m->Y-ypos), w, h );
		if (!m->GetCaption())
			continue;

		PaletteHolder text_pal = pal_normal;

		if (Area == m) {
			text_pal = pal_selected.get();
		} else {
			if (! (m->GetAreaStatus() & WMP_ENTRY_VISITED)) {
				text_pal = pal_notvisited.get();
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

void WorldMapControl::ScrollDelta(const Point& delta)
{
	ScrollTo(Pos + delta);
}

void WorldMapControl::ScrollTo(const Point& pos)
{
	Pos = pos;
	WorldMap* worldmap = core->GetWorldMap();
	Sprite2D *MapMOS = worldmap->GetMapMOS();

	int maxx = MapMOS->Frame.w - frame.w;
	int maxy = MapMOS->Frame.h - frame.h;
	Pos.x = Clamp<int>(Pos.x, 0, maxx);
	Pos.y = Clamp<int>(Pos.y, 0, maxy);

	MarkDirty();
}

/** Mouse Over Event */
bool WorldMapControl::OnMouseOver(const MouseEvent& me)
{
	WorldMap* worldmap = core->GetWorldMap();
	Point p = ConvertPointFromScreen(me.Pos());

	if (GetValue()!=(ieDword) -1) {
		Point mapOff = p + Pos;

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
				rgn.x -= icon->Frame.x;
				rgn.y -= icon->Frame.y;
				rgn.w = icon->Frame.w;
				rgn.h = icon->Frame.h;
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

			SetCursor(core->Cursors[IE_CURSOR_NORMAL]);
			Area=ae;
			if(oldArea!=ae) {
				String* str = core->GetString(23084);
				if (str) {
					wchar_t dist[10];
					swprintf(dist, 10, L": %d", worldmap->GetDistance(Area->AreaName));
					SetTooltip(*str + dist);
					delete str;
				}
			}
			break;
		}
		if (Area == NULL) {
			SetTooltip(L"");
		}
	}
	return true;
}

bool WorldMapControl::OnMouseDrag(const MouseEvent& me)
{
	if (me.ButtonState(GEM_MB_ACTION)) {
		ScrollDelta(me.Delta());
	}
	return true;
}

/** Mouse Leave Event */
void WorldMapControl::OnMouseLeave(const MouseEvent& me, const DragOp* op)
{
	Area = NULL;
	Control::OnMouseLeave(me, op);
}

/** Mouse Button Down */
bool WorldMapControl::OnMouseDown(const MouseEvent& me, unsigned short /*Mod*/)
{
	if (me.button == GEM_MB_ACTION) {
		SetCursor(core->Cursors[IE_CURSOR_GRAB+1]);
	}
	return true;
}

/** Mouse Button Up */
bool WorldMapControl::OnMouseUp(const MouseEvent& me, unsigned short Mod)
{
	if (me.button == GEM_MB_ACTION) {
		SetCursor(core->Cursors[IE_CURSOR_GRAB]);
        Control::OnMouseUp(me, Mod);
	}
	return true;
}

/** Mouse wheel scroll */
bool WorldMapControl::OnMouseWheelScroll(const Point& delta)
{
	// Game coordinates start at the top left to the bottom right
	// so we need to invert the 'y' axis
	Point d = delta;
	d.y *= -1;
	ScrollDelta(d);
	return true;
}

bool WorldMapControl::OnKeyPress(const KeyboardEvent& Key, unsigned short /*Mod*/)
{
	ieDword keyScrollSpd = 64;
	core->GetDictionary()->Lookup("Keyboard Scroll Speed", keyScrollSpd);
	switch (Key.keycode) {
		case GEM_LEFT:
			OnMouseWheelScroll(Point(keyScrollSpd * -1, 0));
			break;
		case GEM_UP:
			OnMouseWheelScroll(Point(0, keyScrollSpd * -1));
			break;
		case GEM_RIGHT:
			OnMouseWheelScroll(Point(keyScrollSpd, 0));
			break;
		case GEM_DOWN:
			OnMouseWheelScroll(Point(0, keyScrollSpd));
			break;
		default:
			return false;
	}
	return true;
}

void WorldMapControl::SetColor(int which, Color color)
{
	// initialize label colors
	// NOTE: it would be better to initialize these colors from
	//   some 2da file
	static const Color normal(0xf0, 0xf0, 0xf0, 0xff);
	static const Color selected(0xf0, 0x80, 0x80, 0xff);
	static const Color notvisited(0x80, 0x80, 0xf0, 0xff);
	// FIXME: clearly it can cause palettes to be re-created several times,
	//   because setting background color creates all palettes anew.
	switch (which) {
	case IE_GUI_WMAP_COLOR_BACKGROUND:
		pal_normal = MakeHolder<Palette>(normal, color);
		pal_selected = MakeHolder<Palette>(selected, color);
		pal_notvisited = MakeHolder<Palette>(notvisited, color);
		break;
	case IE_GUI_WMAP_COLOR_NORMAL:
		pal_normal = MakeHolder<Palette>(normal, color);
		break;
	case IE_GUI_WMAP_COLOR_SELECTED:
		pal_selected = MakeHolder<Palette>(selected, color);
		break;
	case IE_GUI_WMAP_COLOR_NOTVISITED:
		pal_notvisited = MakeHolder<Palette>(notvisited, color);
		break;
	default:
		break;
	}

	MarkDirty();
}

}
