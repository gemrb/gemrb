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

#include "ie_cursors.h"
#include "strrefs.h"

#include "DisplayMessage.h"
#include "Game.h"
#include "Interface.h"
#include "WorldMap.h"
#include "GUI/EventMgr.h"
#include "GUI/TextSystem/Font.h"
#include "GUI/Window.h"

namespace GemRB {

WorldMapControl::WorldMapControl(const Region& frame, Font *font)
	: Control(frame), ftext(font)
{
	ControlType = IE_GUI_WORLDMAP;
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
		
	ControlEventHandler handler = [this](Control* /*this*/) {
		//this also updates visible locations
		WorldMap* worldmap = core->GetWorldMap();
		worldmap->CalculateDistances(currentArea, GetValue());
	};
	
	SetAction(handler, Control::ValueChange);
}

/** Draws the Control on the Output Display */
void WorldMapControl::DrawSelf(Region rgn, const Region& /*clip*/)
{
	auto MapToScreen = [&rgn, this](const Point& p) {
		return rgn.Origin() - Pos + p;
	};
	
	WorldMap* worldmap = core->GetWorldMap();

	Video* video = core->GetVideoDriver();
	video->BlitSprite( worldmap->GetMapMOS(), MapToScreen(Point()));

	unsigned int ec = worldmap->GetEntryCount();
	for (unsigned int i = 0; i < ec; i++) {
		WMPAreaEntry *m = worldmap->GetEntry(i);
		if (! (m->GetAreaStatus() & WMP_ENTRY_VISIBLE)) continue;

		Point offset = MapToScreen(m->pos);
		Holder<Sprite2D> icon = m->GetMapIcon(worldmap->bam, OverrideIconPalette);
		if (icon) {
			if (m == Area && m->HighlightSelected()) {
				video->BlitGameSprite(icon, offset, BLIT_COLOR_MOD, color_selected);
			} else {
				video->BlitSprite( icon, offset);
			}
		}

		if (AnimPicture && (!strnicmp(m->AreaResRef, currentArea, 8)
			|| !strnicmp(m->AreaName, currentArea, 8))) {
			video->BlitSprite(AnimPicture, offset);
		}
	}

	// draw labels in separate pass, so icons don't overlap them
	for (unsigned int i = 0; i < ec; i++) {
		WMPAreaEntry *m = worldmap->GetEntry(i);
		if (! (m->GetAreaStatus() & WMP_ENTRY_VISIBLE)) continue;
		const String* caption = m->GetCaption();
		if (ftext == nullptr || caption == nullptr)
			continue;

		Holder<Sprite2D> icon = m->GetMapIcon(worldmap->bam, OverrideIconPalette);
		const Region& icon_frame = icon->Frame;
		Point p = m->pos - icon_frame.Origin();
		Region r2 = Region(MapToScreen(p), icon_frame.Dimensions());
		
		Font::PrintColors colors;
		if (Area == m) {
			colors.fg = color_selected;
		} else if (!(m->GetAreaStatus() & WMP_ENTRY_VISITED)) {
			colors.fg = color_notvisited;
		} else {
			colors.fg = color_normal;
		}
		
		colors.bg = ColorBlack;

		Size ts = ftext->StringSize(*caption);
		ts.w += 10;
		ftext->Print(Region(Point(r2.x + (r2.w - ts.w)/2, r2.y + r2.h), ts),
					 *caption, 0, colors);
	}
}

void WorldMapControl::ScrollDelta(const Point& delta)
{
	ScrollTo(Pos + delta);
}

void WorldMapControl::ScrollTo(const Point& pos)
{
	Pos = pos;
	WorldMap* worldmap = core->GetWorldMap();
	Holder<Sprite2D> MapMOS = worldmap->GetMapMOS();

	if (pos.isnull()) {
		// center worldmap on current area
		unsigned entry;
		const WMPAreaEntry *areaEntry = worldmap->GetArea(currentArea, entry);
		if (areaEntry) {
			Pos.x = areaEntry->pos.x - frame.w / 2;
			Pos.y = areaEntry->pos.y - frame.h / 2;
		}
	}

	int maxx = MapMOS->Frame.w - frame.w;
	int maxy = MapMOS->Frame.h - frame.h;
	Pos.x = Clamp<int>(Pos.x, 0, maxx);
	Pos.y = Clamp<int>(Pos.y, 0, maxy);

	MarkDirty();
}

/** Mouse Over Event */
bool WorldMapControl::OnMouseOver(const MouseEvent& me)
{
	if (GetValue() != ieDword(-1)) {
		SetCursor(core->Cursors[IE_CURSOR_GRAB]);
		WorldMap* worldmap = core->GetWorldMap();
		Point p = ConvertPointFromScreen(me.Pos());
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

			Holder<Sprite2D> icon = ae->GetMapIcon(worldmap->bam, OverrideIconPalette);
			Region rgn(ae->pos, Size());
			if (icon) {
				rgn.x -= icon->Frame.x;
				rgn.y -= icon->Frame.y;
				rgn.w = icon->Frame.w;
				rgn.h = icon->Frame.h;
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
				String* str = core->GetString(DisplayMessage::GetStringReference(STR_TRAVEL_TIME));
				int hours = worldmap->GetDistance(Area->AreaName);
				if (str && !str->empty() && hours >= 0) {
					wchar_t dist[10];
					swprintf(dist, 10, L": %d", hours);
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
	switch (which) {
	case IE_GUI_WMAP_COLOR_NORMAL:
		color_normal = color;
		break;
	case IE_GUI_WMAP_COLOR_SELECTED:
		color_selected = color;
		break;
	case IE_GUI_WMAP_COLOR_NOTVISITED:
		color_notvisited = color;
		break;
	default:
		break;
	}

	MarkDirty();
}

}
