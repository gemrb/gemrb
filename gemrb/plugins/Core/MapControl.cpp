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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/MapControl.cpp,v 1.1 2004/08/22 19:24:26 edheldil Exp $
 */

#include "../../includes/win32def.h"
#include "MapControl.h"
#include "Interface.h"

#define MAP_SCALE 10.67


extern Interface* core;
static Color green = {
	0x00, 0xff, 0x00, 0xff
};

MapControl::MapControl(void)
{
	ScrollX = 0;
	ScrollY = 0;
	MouseIsDown = false;
	Changed = true;

	MapMOS = core->GetGame()->GetCurrentMap()->SmallMap->GetImage();
}


MapControl::~MapControl(void)
{
}

/** Draws the Control on the Output Display */
void MapControl::Draw(unsigned short x, unsigned short y)
{
	if (!Width || !Height) {
		return;
	}
	Video* video = core->GetVideoDriver();
	Region r( XPos, YPos, Width, Height );
	short xc = (Width - MapMOS->Width) / 2;
	short yc = (Height - MapMOS->Height) / 2;

	if (xc < 0) xc = 0;
	if (yc < 0) yc = 0;

	video->BlitSprite( MapMOS, XPos + xc - ScrollX, YPos + yc - ScrollY, true, &r );

	// Draw PCs' ellipses
	int i;
	for (i = 1; i < 9; i++) {
		Actor* actor = core->GetGame()->FindPC( i );
		if (actor) {
			core->GetVideoDriver()->DrawEllipse( XPos + xc - ScrollX + (int)(actor->XPos/MAP_SCALE), YPos + yc - ScrollY + (int)(actor->YPos/MAP_SCALE), 3, 2, green, false );
		}
	}
}

/** Key Press Event */
void MapControl::OnKeyPress(unsigned char Key, unsigned short Mod)
{
}

/** Key Release Event */
void MapControl::OnKeyRelease(unsigned char Key, unsigned short Mod)
{
	unsigned int i;

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
	int nextCursor = 0;

	if (MouseIsDown) {
		ScrollX -= x - lastMouseX;
		ScrollY -= y - lastMouseY;

		if (ScrollX > MapMOS->Width - Width)
			ScrollX = MapMOS->Width - Width;
		if (ScrollY > MapMOS->Height - Height)
			ScrollY = MapMOS->Height - Height;
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
	unsigned char Button, unsigned short Mod)
{
	if ((Button != GEM_MB_ACTION) ) {
		return;
	}
	MouseIsDown = true;
	lastMouseX = x;
	lastMouseY = y;
}
/** Mouse Button Up */
void MapControl::OnMouseUp(unsigned short x, unsigned short y,
	unsigned char Button, unsigned short Mod)
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

	if (ScrollX > MapMOS->Width - Width)
		ScrollX = MapMOS->Width - Width;
	if (ScrollY > MapMOS->Height - Height)
		ScrollY = MapMOS->Height - Height;
	if (ScrollX < 0)
		ScrollX = 0;
	if (ScrollY < 0)
		ScrollY = 0;
}

