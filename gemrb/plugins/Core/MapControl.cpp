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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/MapControl.cpp,v 1.5 2004/09/04 12:08:55 avenger_teambg Exp $
 */

#include "../../includes/win32def.h"
#include "MapControl.h"
#include "Interface.h"

// Ratio between pixel sizes of an Area (Big map) and a Small map

static int MAP_DIV   = 3;
static int MAP_MULT  = 32;

static Color green = {
	0x00, 0xff, 0x00, 0xff
};
static Color darkgreen = {
	0x00, 0x80, 0x00, 0xff
};

#define MAP_TO_SCREENX(x) XPos + XCenter - ScrollX + (x)
#define MAP_TO_SCREENY(y) YPos + YCenter - ScrollY + (y)
// Omit [XY]Pos, since these macros are used in OnMouseDown(x, y), and x, y is 
//   already relative to control [XY]Pos there
#define SCREEN_TO_MAPX(x) (x) - XCenter + ScrollX
#define SCREEN_TO_MAPY(y) (y) - YCenter + ScrollY

#define GAME_TO_SCREENX(x) MAP_TO_SCREENX((int)((x) * MAP_DIV / MAP_MULT))
#define GAME_TO_SCREENY(y) MAP_TO_SCREENY((int)((y) * MAP_DIV / MAP_MULT))

MapControl::MapControl(void)
{
	if(core->HasFeature(GF_IWD_MAP_DIMENSIONS) ) {
		MAP_DIV=1;
		MAP_MULT=8;
	}
	else {
		MAP_DIV=3;
		MAP_MULT=32;
	}

	ScrollX = 0;
	ScrollY = 0;
	MouseIsDown = false;
	Changed = true;

	MapMOS = core->GetGame()->GetCurrentMap()->SmallMap->GetImage();
}


MapControl::~MapControl(void)
{
}

// To be called after changes in control's or screen geometry
void MapControl::Realize()
{
	Video* video = core->GetVideoDriver();

	// FIXME: ugly!! How to get area size in pixels?
	Map *map = core->GetGame()->GetCurrentMap();

	MapWidth = map->GetWidth();
	MapHeight = map->GetHeight();
printf("%d %d\n",MapWidth, MapHeight);
printf("%d %d\n",MapMOS->Width, MapMOS->Height);

	// FIXME: ugly hack! What is the actual viewport size?
	Region vp = video->GetViewport();

	ViewWidth = core->Width * MAP_DIV / MAP_MULT;
	ViewHeight = core->Height * MAP_DIV / MAP_MULT;

	XCenter = (Width - MapWidth ) / 2;
	YCenter = (Height - MapHeight ) / 2;
	if (XCenter < 0) XCenter = 0;
	if (YCenter < 0) YCenter = 0;
}

/** Draws the Control on the Output Display */
void MapControl::Draw(unsigned short /*x*/, unsigned short /*y*/)
{
	if (!Width || !Height) {
		return;
	}

	if (Changed) {
		Realize();
		Changed = false;
	}

	Video* video = core->GetVideoDriver();
	Region r( XPos, YPos, Width, Height );

	video->BlitSprite( MapMOS, MAP_TO_SCREENX(0), MAP_TO_SCREENY(0), true, &r );

	Region vp = video->GetViewport();

	vp.x = GAME_TO_SCREENX(vp.x);
	vp.y = GAME_TO_SCREENY(vp.y);
	vp.w = ViewWidth;
	vp.h = ViewHeight;

	video->DrawRect( vp, green, false, false );

	// Draw PCs' ellipses
	for (int i = 1; i < 9; i++) {
		Actor* actor = core->GetGame()->FindPC( i );
		if (actor) {
			video->DrawEllipse( GAME_TO_SCREENX(actor->XPos), GAME_TO_SCREENY(actor->YPos), 3, 2, i == 1 ? green : darkgreen, false );
		}
	}
}

/** Key Press Event */
void MapControl::OnKeyPress(unsigned char /*Key*/, unsigned short /*Mod*/)
{
}

/** Key Release Event */
void MapControl::OnKeyRelease(unsigned char Key, unsigned short Mod)
{
	switch (Key) {
		case '\t':
			//not GEM_TAB
			printf( "TAB released\n" );
			return;
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
	if (!core->CheatEnabled()) {
		return;
	}
	if (Mod & 64) //ctrl
	{
	}
}
/** Mouse Over Event */
void MapControl::OnMouseOver(unsigned short x, unsigned short y)
{
	if (MouseIsDown) {
		ScrollX -= x - lastMouseX;
		ScrollY -= y - lastMouseY;

		if (ScrollX > MapWidth - Width)
			ScrollX = MapWidth - Width;
		if (ScrollY > MapHeight - Height)
			ScrollY = MapHeight - Height;
		if (ScrollX < 0)
			ScrollX = 0;
		if (ScrollY < 0)
			ScrollY = 0;
	}

	lastMouseX = x;
	lastMouseY = y;


	x += ScrollX;
	y += ScrollY;

}

/** Mouse Button Down */
void MapControl::OnMouseDown(unsigned short x, unsigned short y,
	unsigned char Button, unsigned short /*Mod*/)
{
	if ((Button != GEM_MB_ACTION) ) {
		return;
	}
	MouseIsDown = true;
	lastMouseX = x;
	lastMouseY = y;

	short xp = SCREEN_TO_MAPX(x) - ViewWidth / 2;
	short yp = SCREEN_TO_MAPY(y) - ViewHeight / 2;

	if (xp + ViewWidth > MapWidth) xp = MapWidth - ViewWidth;
	if (yp + ViewHeight > MapHeight) yp = MapHeight - ViewHeight;
	if (xp < 0) xp = 0;
	if (yp < 0) yp = 0;

	core->GetVideoDriver()->SetViewport( xp * MAP_MULT / MAP_DIV, yp * MAP_MULT / MAP_DIV );

	// FIXME: play button sound here
}

/** Mouse Button Up */
void MapControl::OnMouseUp(unsigned short /*x*/, unsigned short /*y*/,
	unsigned char Button, unsigned short /*Mod*/)
{
	if (Button != GEM_MB_ACTION) {
		return;
	}

	MouseIsDown = false;
}

/** Special Key Press */
void MapControl::OnSpecialKeyPress(unsigned char Key)
{
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

	if (ScrollX > MapWidth - Width)
		ScrollX = MapWidth - Width;
	if (ScrollY > MapHeight - Height)
		ScrollY = MapHeight - Height;
	if (ScrollX < 0)
		ScrollX = 0;
	if (ScrollY < 0)
		ScrollY = 0;
}

