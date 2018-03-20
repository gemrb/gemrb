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

#include "win32def.h"
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

typedef enum {black=0, gray, violet, green, orange, red, blue, darkblue, darkgreen} colorcode;

static const Color colors[]={
 ColorBlack,
 ColorGray,
 ColorViolet,
 ColorGreen,
 ColorOrange,
 ColorRed,
 ColorBlue,
 ColorBlueDark,
 ColorGreenDark
};

MapControl::MapControl(const Region& frame)
	: Control(frame)
{
	ControlType = IE_GUI_MAP;

	LinkedLabel = NULL;
	MapWidth = MapHeight = 0;

	memset(Flag,0,sizeof(Flag) );

	MyMap = core->GetGame()->GetCurrentArea();
	if (MyMap && MyMap->SmallMap) {
		MapMOS = MyMap->SmallMap;
		MapMOS->acquire();
	} else
		MapMOS = NULL;
}

MapControl::~MapControl(void)
{
	if (MapMOS) {
		Sprite2D::FreeSprite(MapMOS);
	}
	for(int i=0;i<8;i++) {
		if (Flag[i]) {
			Sprite2D::FreeSprite(Flag[i]);
		}
	}
}

// Draw fog on the small bitmap
void MapControl::DrawFog(const Region& rgn)
{
	Video *video = core->GetVideoDriver();
	const Size mapsize = MyMap->GetSize();
	Point p;
	Point gameP = p;

	for (; p.y < rgn.h; ++p.y) {
		gameP.y = p.y * double(mapsize.h) / mosRgn.h;

		for (p.x = 0; p.x < rgn.w; ++p.x) {
			gameP.x = p.x * double(mapsize.w) / mosRgn.w;
			
			bool visible = MyMap->IsVisible( gameP, true );
			if (!visible) {
				Region px = Region ( p + rgn.Origin(), Size(1, 1) );
				video->DrawRect( px, colors[black] );
			}
		}
	}
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
	p = ConvertPointToSuper(p) - mosRgn.Origin();
	
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
	return p + mosRgn.Origin();
}
	
void MapControl::WillDraw()
{
	if (MapMOS) {
		const Size mosSize(MapMOS->Width, MapMOS->Height);
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
	const Size mapsize = MyMap->GetSize();

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

	if (MapMOS) {
		video->BlitSprite( MapMOS, mosRgn.x, mosRgn.y, NULL );
	}

	if (core->FogOfWar&FOG_DRAWFOG)
		DrawFog(mosRgn);

	Region vp = GetViewport();
	video->DrawRect(vp, colors[green], false );
	
	// Draw PCs' ellipses
	Game *game = core->GetGame();
	int i = game->GetPartySize(true);
	while (i--) {
		Actor* actor = game->GetPC( i, true );
		if (MyMap->HasActor(actor) ) {
			Point pos = ConvertPointFromGame(actor->Pos);
			video->DrawEllipse( pos, 3, 2, actor->Selected ? colors[green] : colors[darkgreen] );
		}
	}
	// Draw Map notes, could be turned off in bg2
	// we use the common control value to handle it, because then we
	// don't need another interface
	if (GetValue()!=MAP_NO_NOTES) {
		i = MyMap -> GetMapNoteCount();
		while (i--) {
			const MapNote& mn = MyMap -> GetMapNote(i);
			Sprite2D *anim = Flag[mn.color&7];
			
			Point pos = ConvertPointFromGame(mn.Pos);
			
			//Skip unexplored map notes
			bool visible = MyMap->IsVisible( pos, true );
			if (!visible)
				continue;

			if (anim) {
				video->BlitSprite( anim, pos.x - anim->Width/2, pos.y - anim->Height/2, &rgn );
			} else {
				video->DrawEllipse( pos, 6, 5, colors[mn.color&7] );
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
	core->timer->SetMoveViewPort( vp, 0, false );
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
			Sprite2D* cursor = (EventMgr::MouseButtonState(GEM_MB_ACTION)) ? core->Cursors[IE_CURSOR_PRESSED] : NULL;
			SetCursor(cursor);
			break;
	}
}

/** Mouse Button Down */
bool MapControl::OnMouseDown(const MouseEvent& me, unsigned short /*Mod*/)
{
	if (me.ButtonState(GEM_MB_ACTION)) {
		UpdateViewport(ConvertPointFromScreen(me.Pos()));
	}
	UpdateCursor();
	return true;
}
	
/** Mouse Over Event */
bool MapControl::OnMouseOver(const MouseEvent& me)
{
	Point p = ConvertPointFromScreen(me.Pos());
	
	ieDword val = GetValue();
	if (val) {
		p = ConvertPointToGame(p);

		Size mapsize = MyMap->GetSize();
		int i = MyMap->GetMapNoteCount();
		while (i--) {
			const MapNote& mn = MyMap->GetMapNote(i);
			Sprite2D *anim = Flag[mn.color&7];
			
			if (Distance(p, mn.Pos) < anim->Width / 2 * double(mapsize.w) / mosRgn.w) {
				if (LinkedLabel) {
					LinkedLabel->SetText( mn.text );
				}
				notePos = mn.Pos;
				return true;
			}
		}

		notePos = p;
	}
	if (LinkedLabel) {
		LinkedLabel->SetText( L"" );
	}
	
	UpdateCursor();
	return true;
}
	
bool MapControl::OnMouseDrag(const MouseEvent& me)
{
	if (me.ButtonState(GEM_MB_ACTION)) {
		UpdateViewport(ConvertPointFromScreen(me.Pos()));
	}
	return true;
}

/** Mouse Button Up */
bool MapControl::OnMouseUp(const MouseEvent& me, unsigned short mod)
{
	if (me.button == GEM_MB_ACTION && me.repeats == 2) {
		window->Close();
	}
	Point p = ConvertPointFromScreen(me.Pos());

	switch(GetValue()) {
		case MAP_REVEAL:
			UpdateViewport(p);
			notePos = ConvertPointToGame(p);
			ClickHandle(me);
			break;
		case MAP_NO_NOTES:
			UpdateViewport(p);
			break;
		case MAP_VIEW_NOTES:
			//left click allows setting only when in MAP_SET_NOTE mode
			if (me.ButtonState(GEM_MB_ACTION)) {
				UpdateViewport(p);
			}
			ClickHandle(me);
			break;
		default:
			break;
	}

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
