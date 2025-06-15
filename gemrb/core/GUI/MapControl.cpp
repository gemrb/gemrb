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
#include "Interface.h"
#include "Map.h"
#include "Sprite2D.h"

#include "GUI/GameControl.h"
#include "GUI/TextEdit.h"
#include "Scriptable/Actor.h"

namespace GemRB {

MapControl::MapControl(const Region& frame, std::shared_ptr<const AnimationFactory> af)
	: Control(frame), mapFlags(std::move(af))
{
	ControlType = IE_GUI_MAP;
	SetValueRange({ NO_NOTES, EDIT_NOTE });
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
	const Size mapsize = MyMap->GetSize();
	Point p;
	Point gameP = p;

	std::vector<BasePoint> points;
	points.reserve(rgn.w_get() * rgn.h_get());

	for (; p.y < rgn.h_get(); ++p.y) {
		gameP.y = int(p.y * double(mapsize.h) / mosRgn.h_get());

		for (p.x = 0; p.x < rgn.w_get(); ++p.x) {
			gameP.x = int(p.x * double(mapsize.w) / mosRgn.w_get());

			bool visible = MyMap->IsExplored(gameP);
			if (!visible) {
				points.push_back(p + rgn.origin);
			}
		}
	}

	VideoDriver->DrawPoints(points, ColorBlack);
}

Point MapControl::ConvertPointToGame(Point p) const
{
	const Size mapsize = MyMap->GetSize();

	// mos is centered... first convert p to mos coordinates
	// mos is in win coordinates (to make things easy elsewhere)
	p = ConvertPointToSuper(p) - mosRgn.origin;

	p.x = int(p.x * double(mapsize.w) / mosRgn.w_get());
	p.y = int(p.y * double(mapsize.h) / mosRgn.h_get());

	return p;
}

Point MapControl::ConvertPointFromGame(Point p) const
{
	const Size mapsize = MyMap->GetSize();

	p.x = int(p.x * double(mosRgn.w_get()) / mapsize.w);
	p.y = int(p.y * double(mosRgn.h_get()) / mapsize.h);

	// mos is centered... convert p from mos coordinates
	return p + mosRgn.origin;
}

void MapControl::UpdateState(value_t val)
{
	SetValue(val);
}

void MapControl::WillDraw(const Region& /*drawFrame*/, const Region& /*clip*/)
{
	UpdateMap();

	if (LinkedLabel) {
		if (GetValue() == EDIT_NOTE) {
			LinkedLabel->SetFlags(IgnoreEvents, BitOp::NAND);
			LinkedLabel->SetFocus();
		} else {
			LinkedLabel->SetFlags(IgnoreEvents, BitOp::OR);
		}
	}

	if (MapMOS) {
		const Size& mosSize = MapMOS->Frame.size;
		const Point center(frame.w_get() / 2 - mosSize.w / 2, frame.h_get() / 2 - mosSize.h / 2);
		mosRgn = Region(Origin() + center, mosSize);
	} else {
		mosRgn = Region(Point(), Dimensions());
	}
}

Region MapControl::GetViewport() const
{
	const GameControl* gc = core->GetGameControl();
	Region vp = gc->Viewport();
	const Size& mapsize = MyMap->GetSize();

	vp.x_get() = int(vp.x_get() * double(mosRgn.w_get()) / mapsize.w);
	vp.y_get() = int(vp.y_get() * double(mosRgn.h_get()) / mapsize.h);
	vp.w_get() = int(vp.w_get() * double(mosRgn.w_get()) / mapsize.w);
	vp.h_get() = int(vp.h_get() * double(mosRgn.h_get()) / mapsize.h);

	vp.x_get() += mosRgn.x_get();
	vp.y_get() += mosRgn.y_get();
	return vp;
}

/** Draws the Control on the Output Display */
void MapControl::DrawSelf(const Region& rgn, const Region& /*clip*/)
{
	VideoDriver->DrawRect(rgn, ColorBlack, true);

	if (MyMap == nullptr) {
		return;
	}

	if (MapMOS) {
		VideoDriver->BlitSprite(MapMOS, mosRgn.origin);
	}

	if ((GameControl::DebugFlags & DEBUG_SHOW_FOG_UNEXPLORED) == 0)
		DrawFog(mosRgn);

	Region vp = GetViewport();
	VideoDriver->DrawRect(vp, ColorGreen, false);

	// Draw PCs' ellipses
	const Game* game = core->GetGame();
	int i = game->GetPartySize(true);
	while (i--) {
		const Actor* actor = game->GetPC(i, true);
		if (MyMap->HasActor(actor)) {
			const Point pos = ConvertPointFromGame(actor->Pos);
			const Size s(6, 4);
			const Region r(pos - s.Center(), s);
			VideoDriver->DrawEllipse(r, actor->Selected ? ColorGreen : ColorGreenDark);
		}
	}
	// Draw Map notes, could be turned off in bg2
	// we use the common control value to handle it, because then we
	// don't need another interface
	if (GetValue() != NO_NOTES) {
		i = MyMap->GetMapNoteCount();
		while (i--) {
			const MapNote& mn = MyMap->GetMapNote(i);

			// Skip unexplored map notes unless they are player added
			// PST draws user notes regardless of visibility
			bool visible = MyMap->IsExplored(mn.Pos);
			if (!visible && (!core->HasFeature(GFFlags::PST_STATE_FLAGS) || mn.readonly)) {
				continue;
			}

			Point pos = ConvertPointFromGame(mn.Pos);

			Holder<Sprite2D> anim = mapFlags ? mapFlags->GetFrame(0, mn.color) : nullptr;
			if (anim) {
				Point p(anim->Frame.w_get() / 2, anim->Frame.h_get() / 2);
				VideoDriver->BlitSprite(anim, pos - p);
			} else {
				const Size s(12, 10);
				const Region r(pos - s.Center(), s);
				VideoDriver->DrawEllipse(r, mn.GetColor());
			}
		}
	}
}

void MapControl::ClickHandle(const MouseEvent&) const
{
	auto& vars = core->GetDictionary();
	vars.Set("MapControlX", notePos.x);
	vars.Set("MapControlY", notePos.y);

	if (LinkedLabel && LinkedLabel->ControlType == IE_GUI_EDIT && GetValue() == EDIT_NOTE) {
		static_cast<TextEdit*>(LinkedLabel)->SetBackground(TextEditBG::Editing);
	}
}

void MapControl::UpdateViewport(Point vp)
{
	Region vp2 = GetViewport();
	vp.x -= vp2.w_get() / 2;
	vp.y -= vp2.h_get() / 2;
	vp = ConvertPointToGame(vp);

	// clear any previously scheduled moves and then do it asap, so it works while paused
	core->timer.SetMoveViewPort(vp, 0, false);

	MarkDirty();
}

void MapControl::UpdateCursor()
{
	value_t val = GetValue();
	switch (val) {
		case REVEAL: //for farsee effect
			SetCursor(core->Cursors[IE_CURSOR_CAST]);
			break;
		case SET_NOTE:
			SetCursor(core->Cursors[IE_CURSOR_GRAB]);
			break;
		default:
			Holder<Sprite2D> cursor = EventMgr::MouseButtonState(GEM_MB_ACTION) ? core->Cursors[IE_CURSOR_PRESSED] : nullptr;
			SetCursor(std::move(cursor));
			break;
	}
}

const MapNote* MapControl::MapNoteAtPoint(const Point& p) const
{
	Point gamePoint = ConvertPointToGame(p);
	Size mapsize = MyMap->GetSize();

	float scalar = float(mapsize.w) / float(mosRgn.w_get());
	unsigned int radius = (unsigned int) ((mapFlags ? mapFlags->GetFrame(0)->Frame.w_get() / 2 : 5) * scalar);

	return MyMap->MapNoteAtPoint(gamePoint, radius);
}

/** Mouse Button Down */
bool MapControl::OnMouseDown(const MouseEvent& me, unsigned short /*Mod*/)
{
	if (MyMap == NULL)
		return false;

	if (me.ButtonState(GEM_MB_ACTION)) {
		Point p = ConvertPointFromScreen(me.Pos());
		if (GetValue() == VIEW_NOTES) {
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

	TextEdit* edit = nullptr;
	if (LinkedLabel && LinkedLabel->ControlType == IE_GUI_EDIT) {
		edit = static_cast<TextEdit*>(LinkedLabel);
	}

	value_t val = GetValue();
	if (val == VIEW_NOTES) {
		Point p = ConvertPointFromScreen(me.Pos());

		const MapNote* mn = MapNoteAtPoint(p);
		if (mn) {
			notePos = mn->Pos;

			if (LinkedLabel) {
				LinkedLabel->SetText(mn->text);
			}
			if (edit) {
				edit->SetBackground(TextEditBG::Over);
			}
		} else if (LinkedLabel) {
			LinkedLabel->SetText(u"");
			if (edit) {
				edit->SetBackground(TextEditBG::Normal);
			}
		}
	}

	UpdateCursor();
	return true;
}

bool MapControl::OnMouseDrag(const MouseEvent& me)
{
	if (GetValue() == VIEW_NOTES) {
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

	switch (GetValue()) {
		case EDIT_NOTE:
			SetValue(VIEW_NOTES);
			break;
		case REVEAL:
			UpdateViewport(p);
			notePos = ConvertPointToGame(p);
			break;
		case SET_NOTE:
			notePos = ConvertPointToGame(p);
			SetValue(EDIT_NOTE);
			break;
		case NO_NOTES:
			UpdateViewport(p);
			break;
		case VIEW_NOTES:
			//left click allows setting only when in MAP_SET_NOTE mode
			if (me.ButtonState(GEM_MB_ACTION)) {
				UpdateViewport(p);
			} else {
				const MapNote* mn = MapNoteAtPoint(p);
				if (mn && !mn->readonly) {
					notePos = mn->Pos;
					SetValue(EDIT_NOTE);
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
