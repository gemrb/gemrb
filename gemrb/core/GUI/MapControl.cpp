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
	NotePosX = 0;
	NotePosY = 0;
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

	for (; p.y < rgn.h; ++p.y) {
		for (; p.x < rgn.w; ++p.x) {
			Point gameP = p;
			gameP.x *= double(mapsize.w) / mosRgn.w;
			gameP.y *= double(mapsize.h) / mosRgn.h;
			
			bool visible = MyMap->IsVisible( gameP, true );
			if (!visible) {
				Region rgn = Region ( ConvertPointToScreen(p), Size(1, 1) );
				video->DrawRect( rgn, colors[black] );
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

/** Draws the Control on the Output Display */
void MapControl::DrawSelf(Region rgn, const Region& /*clip*/)
{
	Video* video = core->GetVideoDriver();

	if (MapMOS) {
		video->BlitSprite( MapMOS, mosRgn.x, mosRgn.y, NULL );
	}

	if (core->FogOfWar&FOG_DRAWFOG)
		DrawFog(mosRgn);

	GameControl* gc = core->GetGameControl();
	const Size mapsize = MyMap->GetSize();

	Region vp = gc->Viewport();
	vp.x *= double(mosRgn.w) / mapsize.w;
	vp.y *= double(mosRgn.h) / mapsize.h;
	vp.w *= double(mosRgn.w) / mapsize.w;
	vp.h *= double(mosRgn.h) / mapsize.h;
	
	vp.x += mosRgn.x;
	vp.y += mosRgn.y;
	video->DrawRect(vp, colors[green], false );
	
	// Draw PCs' ellipses
	Game *game = core->GetGame();
	int i = game->GetPartySize(true);
	while (i--) {
		Actor* actor = game->GetPC( i, true );
		if (MyMap->HasActor(actor) ) {
			Point pos = ConvertPointFromGame(actor->Pos);
			video->DrawEllipse( pos.x, pos.y, 3, 2, actor->Selected ? colors[green] : colors[darkgreen] );
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
				video->DrawEllipse( pos.x, pos.y, 6, 5, colors[mn.color&7] );
			}
		}
	}
}

void MapControl::ClickHandle()
{
	core->GetDictionary()->SetAt( "MapControlX", NotePosX );
	core->GetDictionary()->SetAt( "MapControlY", NotePosY );
}

void MapControl::UpdateViewport(Point vp)
{
	vp = ConvertPointToGame(vp);
	
	// clear any previously scheduled moves and then do it asap, so it works while paused
	core->timer->SetMoveViewPort( vp, 0, false );
}

/** Mouse Button Down */
void MapControl::OnMouseDown(const MouseEvent& me, unsigned short /*Mod*/)
{
	UpdateViewport(ConvertPointFromScreen(me.Pos()));
}
	
/** Mouse Over Event */
void MapControl::OnMouseOver(const MouseEvent& me)
{
	Point p = ConvertPointFromScreen(me.Pos());
	
	ieDword val = GetValue();
	// FIXME: implement cursor changing
	switch (val) {
		case MAP_REVEAL: //for farsee effect
			//Owner->Cursor = IE_CURSOR_CAST;
			break;
		case MAP_SET_NOTE:
			//Owner->Cursor = IE_CURSOR_GRAB;
			break;
		default:
			//Owner->Cursor = IE_CURSOR_NORMAL;
			break;
	}
	
	if (val) {
		unsigned int dist = 16;
		p = ConvertPointToGame(p);

		int i = MyMap -> GetMapNoteCount();
		while (i--) {
			const MapNote& mn = MyMap -> GetMapNote(i);
			if (Distance(p, mn.Pos)<dist) {
				if (LinkedLabel) {
					LinkedLabel->SetText( mn.text );
				}
				NotePosX = mn.Pos.x;
				NotePosY = mn.Pos.y;
				return;
			}
		}

		NotePosX = p.x;
		NotePosY = p.y;
	}
	if (LinkedLabel) {
		LinkedLabel->SetText( L"" );
	}
}
	
void MapControl::OnMouseDrag(const MouseEvent& me)
{
	if (me.ButtonState(GEM_MB_ACTION)) {
		UpdateViewport(ConvertPointFromScreen(me.Pos()));
	}
}

/** Mouse Button Up */
void MapControl::OnMouseUp(const MouseEvent& me, unsigned short /*Mod*/)
{
	if (me.button == GEM_MB_ACTION && me.repeats == 2) {
		window->Close();
	}

	/*
	switch(Value) {
		case MAP_REVEAL:
			ViewHandle(p.x, p.y);
			NotePosX = (short) SCREEN_TO_MAPX(p.x) * MAP_MULT / MAP_DIV;
			NotePosY = (short) SCREEN_TO_MAPY(p.y) * MAP_MULT / MAP_DIV;
			ClickHandle(Button);
			return;
		case MAP_NO_NOTES:
			ViewHandle(p.x, p.y);
			return;
		case MAP_VIEW_NOTES:
			//left click allows setting only when in MAP_SET_NOTE mode
			if (Button == GEM_MB_ACTION) {
				ViewHandle(p.x, p.y);
			}
			ClickHandle(Button);
			return;
		default:
			return Control::OnMouseUp(me, Mod);
	}
	*/
}

bool MapControl::OnKeyPress(const KeyboardEvent& key, unsigned short mod)
{
	switch (key.keycode) {
		case GEM_LEFT:
		case GEM_RIGHT:
		case GEM_UP:
		case GEM_DOWN:
			GameControl* gc = core->GetGameControl();
			return gc->OnKeyPress(key, mod);
	}
	return false;
}

}
