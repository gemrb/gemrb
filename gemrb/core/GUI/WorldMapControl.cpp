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

WorldMapControl::WorldMapControl(const Region& frame, Holder<Font> font, const Color &normal, const Color &selected, const Color &notvisited)
	: Control(frame), ftext(font)
{
	color_normal = normal;
	color_selected = selected;
	color_notvisited = notvisited;
	
	hoverAnim = ColorAnimation(displaymsg->GetColor(GUIColors::MAPICNBG), color_selected, true);
	
	ControlType = IE_GUI_WORLDMAP;
	SetCursor(core->Cursors[IE_CURSOR_GRAB]);
	const Game* game = core->GetGame();
	WorldMap* worldmap = core->GetWorldMap();
	currentArea = game->CurrentArea;
	int entry = gamedata->GetAreaAlias(currentArea);
	if (entry >= 0) {
		const WMPAreaEntry *m = worldmap->GetEntry(entry);
		currentArea = m->AreaResRef;
	}

	// ensure the current area and any variable triggered additions are
	// visible immediately and without the need for area travel
	worldmap->CalculateDistances(currentArea, WMPDirection::NONE);

	ControlEventHandler handler = [this](const Control* /*this*/) {
		//this also updates visible locations
		WorldMap* map = core->GetWorldMap();
		uint8_t dir = static_cast<uint8_t>(GetValue());
		map->CalculateDistances(currentArea, EnumIndex<WMPDirection>(dir));
	};
	
	SetAction(std::move(handler), Control::ValueChange);
}

WorldMapControl::WorldMapControl(const Region& frame, Holder<Font> font)
: WorldMapControl(frame, font,
				  Color(0xf0, 0xf0, 0xf0, 0xff),
				  Color(0xff, 0, 0, 0xff),
				  Color(0x80, 0x80, 0xf0, 0xff))
{}

void WorldMapControl::WillDraw(const Region& /*drawFrame*/, const Region& /*clip*/)
{
	if (hoverAnim) {
		hoverAnim.Next(GetMilliseconds());
	}
}

/** Draws the Control on the Output Display */
void WorldMapControl::DrawSelf(const Region& rgn, const Region& /*clip*/)
{
	auto MapToScreen = [&rgn, this](const Point& p) {
		return rgn.origin - Pos + p;
	};
	
	WorldMap* worldmap = core->GetWorldMap();

	VideoDriver->BlitSprite( worldmap->GetMapMOS(), MapToScreen(Point()));

	std::vector<Point> potentialIndicators;
	unsigned int ec = worldmap->GetEntryCount();
	ResRef current = currentArea;
	const auto nearest = worldmap->FindNearestEntry(currentArea);
	if (nearest) current = nearest->AreaName;

	for (unsigned int i = 0; i < ec; i++) {
		WMPAreaEntry *m = worldmap->GetEntry(i);
		if (! (m->GetAreaStatus() & WMP_ENTRY_VISIBLE)) continue;

		Point offset = MapToScreen(m->pos);
		Holder<Sprite2D> icon = m->GetMapIcon(worldmap->bam.get());
		if (icon) {
			BlitFlags flags =  core->HasFeature(GFFlags::AUTOMAP_INI) ? BlitFlags::BLENDED : (BlitFlags::BLENDED | BlitFlags::COLOR_MOD);
			if (m == Area && m->HighlightSelected()) {
				VideoDriver->BlitGameSprite(icon, offset, flags, hoverAnim.Current());
			} else if (!(m->GetAreaStatus() & WMP_ENTRY_VISITED)) {
				VideoDriver->BlitGameSprite(icon, offset, flags, color_notvisited);
			} else {
				VideoDriver->BlitGameSprite(icon, offset, flags, displaymsg->GetColor(GUIColors::MAPICNBG));
			}

			// intro and late-chapter candlekeep share the same entry, so we need to check both names
			// but bg2 ar1700 has bad data (ar1800), causing two indicators to be displayed, so defer
			if (areaIndicator && (m->AreaResRef == current || m->AreaName == current)) {
				Point indicatorPos = offset - icon->Frame.origin;
				indicatorPos.x += areaIndicator->Frame.x + icon->Frame.w / 2 - areaIndicator->Frame.w / 2;
				// bg2 centered also vertically, while the rest didn't
				if (core->HasFeature(GFFlags::JOURNAL_HAS_SECTIONS)) {
					indicatorPos.y += areaIndicator->Frame.y + icon->Frame.h / 2 - areaIndicator->Frame.h / 2;
				}
				potentialIndicators.push_back(indicatorPos);
			}
		}
	}

	// ensure good indicator placement, if any
	// so far only one bad example is known, hence the simple logic
	size_t indicatorCount = potentialIndicators.size();
	if (indicatorCount == 1) {
		VideoDriver->BlitSprite(areaIndicator, potentialIndicators[0]);
	} else if (indicatorCount > 1) {
		VideoDriver->BlitSprite(areaIndicator, potentialIndicators[1]);
	}

	// draw labels in separate pass, so icons don't overlap them
	for (unsigned int i = 0; i < ec; i++) {
		WMPAreaEntry *m = worldmap->GetEntry(i);
		if (! (m->GetAreaStatus() & WMP_ENTRY_VISIBLE)) continue;
		const String caption = m->GetCaption();
		if (ftext == nullptr || caption.empty())
			continue;

		const Holder<Sprite2D> icon = m->GetMapIcon(worldmap->bam.get());
		if (!icon) continue;
		const Region& icon_frame = icon->Frame;
		Point p = m->pos - icon_frame.origin;
		Region r2 = Region(MapToScreen(p), icon_frame.size);
		
		Font::PrintColors colors;
		if (Area == m) {
			colors.fg = hoverAnim.Current();
		} else if (!(m->GetAreaStatus() & WMP_ENTRY_VISITED)) {
			colors.fg = color_notvisited;
		} else {
			colors.fg = color_normal;
		}
		
		colors.bg = displaymsg->GetColor(GUIColors::MAPTXTBG);

		Size ts = ftext->StringSize(caption);
		ts.w += 10;
		ftext->Print(Region(Point(r2.x + (r2.w - ts.w)/2, r2.y + r2.h), ts),
					 caption, 0, colors);
	}
}

void WorldMapControl::ScrollDelta(const Point& delta)
{
	ScrollTo(Pos + delta);
}

void WorldMapControl::ScrollTo(const Point& pos)
{
	Pos = pos;
	const WorldMap* worldmap = core->GetWorldMap();
	const Holder<Sprite2D> MapMOS = worldmap->GetMapMOS();

	if (pos.IsZero()) {
		// center worldmap on current area
		const WMPAreaEntry* areaEntry = worldmap->GetArea(currentArea);
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
	if (GetValue() == INVALID_VALUE) {
		return true;
	}

	SetCursor(core->Cursors[IE_CURSOR_GRAB]);
	WorldMap* worldmap = core->GetWorldMap();
	Point p = ConvertPointFromScreen(me.Pos());
	Point mapOff = p + Pos;

	const WMPAreaEntry *oldArea = Area;
	Area = nullptr;

	unsigned int ec = worldmap->GetEntryCount();
	for (unsigned int i = 0; i < ec; i++) {
		WMPAreaEntry *ae = worldmap->GetEntry(i);

		if ((ae->GetAreaStatus() & WMP_ENTRY_WALKABLE) != WMP_ENTRY_WALKABLE) {
			continue; //invisible or inaccessible
		}

		const Holder<Sprite2D> icon = ae->GetMapIcon(worldmap->bam.get());
		Region rgn(ae->pos, Size());
		if (icon) {
			rgn.x -= icon->Frame.x;
			rgn.y -= icon->Frame.y;
			rgn.w = icon->Frame.w;
			rgn.h = icon->Frame.h;
		}
		if (ftext) {
			Size ts = ftext->StringSize(ae->GetCaption());
			ts.w += 10;
			if (rgn.h < ts.h)
				rgn.h = ts.h;
			if (rgn.w < ts.w)
				rgn.w = ts.w;
		}
		if (!rgn.PointInside(mapOff)) continue;

		SetCursor(core->Cursors[IE_CURSOR_NORMAL]);
		Area = ae;
		if (oldArea != ae) {
			const String str = core->GetString(DisplayMessage::GetStringReference(HCStrings::TravelTime));
			int hours = worldmap->GetDistance(Area->AreaName);
			if (!str.empty() && hours >= 0) {
				SetTooltip(fmt::format(u"{}: {}", str, hours));
			}
		}
		break;
	}
	if (Area == nullptr) {
		SetTooltip(u"");
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
	ieDword keyScrollSpd = core->GetVariable("Keyboard Scroll Speed", 64);

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

}
