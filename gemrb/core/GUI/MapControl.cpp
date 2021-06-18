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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include "GUI/MapControl.h"

#include "ie_cursors.h"

#include "Game.h"
#include "GlobalTimer.h"
#include "Map.h"
#include "Sprite2D.h"
#include "GUI/GameControl.h"
#include "Scriptable/Actor.h"

namespace GemRB {

#define MAP_NO_NOTES   0
#define MAP_VIEW_NOTES 1
#define MAP_SET_NOTE   2
#define MAP_REVEAL     3
#define MAP_EDIT_NOTE  4

MapControl::MapControl(const Region& frame, AnimationFactory* af)
: Control(frame), mapFlags(af)
{
	ControlType = IE_GUI_MAP;	
	UpdateMap();
}

void MapControl::UpdateMap()
{
	Map* newMap = core->GetGame()->GetCurrentArea();
	if (newMap != MyMap) {
		MyMap = newMap;
		if (MyMap && MyMap->SmallMap) {
			MapMOS = MyMap->SmallMap;
		} else {
			MapMOS = nullptr;
		}

		MarkDirty();
	}
}

// Draw fog on the small bitmap
void MapControl::DrawFog(const Region& rgn) const
{
	Video *video = core->GetVideoDriver();
	const Size mapsize = MyMap->GetSize();
	Point p;
	Point gameP = p;

	std::vector<Point> points;
	points.reserve(rgn.w * rgn.h);

	for (; p.y < rgn.h; ++p.y) {
		gameP.y = p.y * double(mapsize.h) / mosRgn.h;

		for (p.x = 0; p.x < rgn.w; ++p.x) {
			gameP.x = p.x * double(mapsize.w) / mosRgn.w;
			
			bool visible = MyMap->IsExplored(gameP);
			if (!visible) {
				points.push_back(p + rgn.origin);
			}
		}
	}

	video->DrawPoints(points, ColorBlack);
}

void MapControl::UpdateState(unsigned int Sum)
{
	SetValue(Sum);
}
	
Point MapControl::ConvertPointToGame(Point p) const
{
	const Size mapsize = MyMap->GetSize();
	
	// mos is centered... first convert p to mos coordinates
	// mos is in win coordinates (to make things easy elsewhere)
	p = ConvertPointToSuper(p) - mosRgn.origin;
	
	p.x *= double(mapsize.w) / mosRgn.w;
	p.y *= double(mapsize.h) / mosRgn.h;
	
	return p;
}
	
Point MapControl::ConvertPointFromGame(Point p) const
{
	const Size mapsize = MyMap->GetSize();
	
	p.x *= double(mosRgn.w) / mapsize.w;
	p.y *= double(mosRgn.h) / mapsize.h;
	
	// mos is centered... convert p from mos coordinates
	return p + mosRgn.origin;
}
	
void MapControl::WillDraw(const Region& /*drawFrame*/, const Region& /*clip*/)
{
	UpdateMap();

	if (LinkedLabel) {
		if (GetValue() == MAP_EDIT_NOTE)
		{
			LinkedLabel->SetFlags(IgnoreEvents, OP_NAND);
			LinkedLabel->SetFocus();
		} else {
			LinkedLabel->SetFlags(IgnoreEvents, OP_OR);
		}
	}

	if (MapMOS) {
		const Size& mosSize = MapMOS->Frame.size;
		const Point center(frame.w/2 - mosSize.w/2, frame.h/2 - mosSize.h/2);
		mosRgn = Region(Origin() + center, mosSize);
	} else {
		mosRgn = Region(Point(), Dimensions());
	}
}

Region MapControl::GetViewport() const
{
	GameControl* gc = core->GetGameControl();
	Region vp = gc->Viewport();
	const Size& mapsize = MyMap->GetSize();

	vp.x *= double(mosRgn.w) / mapsize.w;
	vp.y *= double(mosRgn.h) / mapsize.h;
	vp.w *= double(mosRgn.w) / mapsize.w;
	vp.h *= double(mosRgn.h) / mapsize.h;

	vp.x += mosRgn.x;
	vp.y += mosRgn.y;
	return vp;
}

/** Draws the Control on the Output Display */
void MapControl::DrawSelf(Region rgn, const Region& /*clip*/)
{
	Video* video = core->GetVideoDriver();
	video->DrawRect(rgn, ColorBlack, true);
		
	if (MyMap == nullptr) {
		return;
	}

	if (MapMOS) {
		video->BlitSprite(MapMOS, mosRgn.origin);
	}

	if ((core->GetGameControl()->DebugFlags & DEBUG_SHOW_FOG_UNEXPLORED) == 0)
		DrawFog(mosRgn);

	Region vp = GetViewport();
	video->DrawRect(vp, ColorGreen, false );
	
	// Draw PCs' ellipses
	Game *game = core->GetGame();
	int i = game->GetPartySize(true);
	while (i--) {
		const Actor *actor = game->GetPC(i, true);
		if (MyMap->HasActor(actor) ) {
			Point pos = ConvertPointFromGame(actor->Pos);
			video->DrawEllipse( pos, 3, 2, actor->Selected ? ColorGreen : ColorGreenDark );
		}
	}
	// Draw Map notes, could be turned off in bg2
	// we use the common control value to handle it, because then we
	// don't need another interface
	if (GetValue()!=MAP_NO_NOTES) {
		i = MyMap -> GetMapNoteCount();
		while (i--) {
			const MapNote& mn = MyMap -> GetMapNote(i);
			
			// Skip unexplored map notes unless they are player added
			// FIXME: PST should include user notes regardless (!mn.readonly)
			bool visible = MyMap->IsExplored(mn.Pos);
			if (!visible)
				continue;

			Point pos = ConvertPointFromGame(mn.Pos);

			Holder<Sprite2D> anim = mapFlags ? mapFlags->GetFrame(0, mn.color) : nullptr;
			if (anim) {
				Point p(anim->Frame.w / 2, anim->Frame.h / 2);
				video->BlitSprite(anim, pos - p);
			} else {
				video->DrawEllipse( pos, 6, 5, mn.GetColor() );
			}
		}
	}
}

void MapControl::ClickHandle(const MouseEvent&)
{
	core->GetDictionary()->SetAt( "MapControlX", notePos.x );
	core->GetDictionary()->SetAt( "MapControlY", notePos.y );
}

void MapControl::UpdateViewport(Point vp)
{
	Region vp2 = GetViewport();
	vp.x -= vp2.w/2;
	vp.y -= vp2.h/2;
	vp = ConvertPointToGame(vp);
	
	// clear any previously scheduled moves and then do it asap, so it works while paused
	core->timer.SetMoveViewPort( vp, 0, false );

	MarkDirty();
}

void MapControl::UpdateCursor()
{
	ieDword val = GetValue();
	switch (val) {
		case MAP_REVEAL: //for farsee effect
			SetCursor(core->Cursors[IE_CURSOR_CAST]);
			break;
		case MAP_SET_NOTE:
			SetCursor(core->Cursors[IE_CURSOR_GRAB]);
			break;
		default:
			Holder<Sprite2D> cursor = EventMgr::MouseButtonState(GEM_MB_ACTION) ? core->Cursors[IE_CURSOR_PRESSED] : nullptr;
			SetCursor(cursor);
			break;
	}
}

const MapNote* MapControl::MapNoteAtPoint(const Point& p) const
{
	Point gamePoint = ConvertPointToGame(p);
	Size mapsize = MyMap->GetSize();

	float scalar = float(mapsize.w) / mosRgn.w;
	unsigned int radius = (mapFlags ? mapFlags->GetFrame(0)->Frame.w / 2 : 5) * scalar;

	return MyMap->MapNoteAtPoint(gamePoint, radius);
}

/** Mouse Button Down */
bool MapControl::OnMouseDown(const MouseEvent& me, unsigned short /*Mod*/)
{
	if (MyMap == NULL)
		return false;

	if (me.ButtonState(GEM_MB_ACTION)) {
		Point p = ConvertPointFromScreen(me.Pos());
		if (GetValue() == MAP_VIEW_NOTES) {
			const MapNote* mn = MapNoteAtPoint(p);
			if (!mn || mn->readonly) {
				UpdateViewport(p);
			}
		} else {
			UpdateViewport(p);
		}
	}

	UpdateCursor();
	return true;
}
	
/** Mouse Over Event */
bool MapControl::OnMouseOver(const MouseEvent& me)
{
	if (MyMap == NULL)
		return false;

	ieDword val = GetValue();
	if (val == MAP_VIEW_NOTES) {
		Point p = ConvertPointFromScreen(me.Pos());

		String* text = NULL;
		const MapNote* mn = MapNoteAtPoint(p);
		if (mn) {
			text = mn->text;
			notePos = mn->Pos;
		}

		if (LinkedLabel) {
			LinkedLabel->SetText(text);
		}
	}

	UpdateCursor();
	return true;
}
	
bool MapControl::OnMouseDrag(const MouseEvent& me)
{
	if (GetValue() == MAP_VIEW_NOTES) {
		if (me.ButtonState(GEM_MB_ACTION)) {
			UpdateViewport(ConvertPointFromScreen(me.Pos()));
		}
	}
	return true;
}

/** Mouse Button Up */
bool MapControl::OnMouseUp(const MouseEvent& me, unsigned short mod)
{
	Point p = ConvertPointFromScreen(me.Pos());

	switch(GetValue()) {
		case MAP_EDIT_NOTE:
			SetValue(MAP_VIEW_NOTES);
			break;
		case MAP_REVEAL:
			UpdateViewport(p);
			notePos = ConvertPointToGame(p);
			break;
		case MAP_SET_NOTE:
			notePos = ConvertPointToGame(p);
			SetValue(MAP_EDIT_NOTE);
			break;
		case MAP_NO_NOTES:
			UpdateViewport(p);
			break;
		case MAP_VIEW_NOTES:
			//left click allows setting only when in MAP_SET_NOTE mode
			if (me.ButtonState(GEM_MB_ACTION)) {
				UpdateViewport(p);
			} else {
				const MapNote* mn = MapNoteAtPoint(p);
				if (mn && !mn->readonly) {
					notePos = mn->Pos;
					SetValue(MAP_EDIT_NOTE);
				} else {
					notePos = ConvertPointToGame(p);
				}
			}
			break;
		default:
			break;
	}

	ClickHandle(me);
	Control::OnMouseUp(me, mod);
	UpdateCursor();
	return true;
}

bool MapControl::OnKeyPress(const KeyboardEvent& key, unsigned short mod)
{
	switch (key.keycode) {
		case GEM_LEFT:
		case GEM_RIGHT:
		case GEM_UP:
		case GEM_DOWN:
			GameControl* gc = core->GetGameControl();
			gc->KeyPress(key, mod);
			return true;
	}
	return Control::OnKeyPress(key, mod);
}

}
