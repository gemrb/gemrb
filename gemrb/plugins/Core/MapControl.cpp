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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/MapControl.cpp,v 1.15 2004/10/14 17:31:26 avenger_teambg Exp $
 */

#include "../../includes/win32def.h"
#include "MapControl.h"
#include "Interface.h"

// Ratio between pixel sizes of an Area (Big map) and a Small map

static int MAP_DIV   = 3;
static int MAP_MULT  = 32;

typedef enum colorcode {gray=0, violet, green, orange, red, blue, darkblue, darkgreen};

Color colors[8]={
  { 0x60, 0x60, 0x60, 0xff }, //gray
  { 0xa0, 0x00, 0xa0, 0xff }, //violet
  { 0x00, 0xff, 0x00, 0xff }, //green
  { 0xff, 0xff, 0x00, 0xff }, //orange
  { 0xff, 0x00, 0x00, 0xff }, //red
  { 0x00, 0x00, 0xff, 0xff }, //blue
  { 0x00, 0x00, 0x80, 0xff }, //darkblue
  { 0x00, 0x80, 0x00, 0xff }  //darkgreen
};

#define MAP_TO_SCREENX(x) (XWin + XPos + XCenter - ScrollX + (x))
#define MAP_TO_SCREENY(y) (YWin + YPos + YCenter - ScrollY + (y))
// Omit [XY]Pos, since these macros are used in OnMouseDown(x, y), and x, y is 
//   already relative to control [XY]Pos there
#define SCREEN_TO_MAPX(x) ((x) - XCenter + ScrollX)
#define SCREEN_TO_MAPY(y) ((y) - YCenter + ScrollY)

#define GAME_TO_SCREENX(x) MAP_TO_SCREENX((int)((x) * MAP_DIV / MAP_MULT))
#define GAME_TO_SCREENY(y) MAP_TO_SCREENY((int)((y) * MAP_DIV / MAP_MULT))

#define SCREEN_TO_GAMEX(x) (SCREEN_TO_MAPX(x) * MAP_MULT / MAP_DIV)
#define SCREEN_TO_GAMEY(y) (SCREEN_TO_MAPY(y) * MAP_MULT / MAP_DIV)

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

	LinkedLabel = NULL;
	ScrollX = 0;
	ScrollY = 0;
	MouseIsDown = false;
	Changed = true;
	memset(Flag,0,sizeof(Flag) );

	MyMap = core->GetGame()->GetCurrentMap();
	MapMOS = MyMap->SmallMap->GetImage();
}

MapControl::~MapControl(void)
{
	Video *video = core->GetVideoDriver();

	if(MapMOS) {
		video->FreeSprite(MapMOS);
	}
	for(int i=0;i<8;i++) {
		if(Flag[i]) {
			video->FreeSprite(Flag[i]);
		}
	}
}

// To be called after changes in control's or screen geometry
void MapControl::Realize()
{
	// FIXME: ugly!! How to get area size in pixels?
	//Map *map = core->GetGame()->GetCurrentMap();
	//MapWidth = map->GetWidth();
	//MapHeight = map->GetHeight();

	MapWidth = MapMOS->Width;
	MapHeight = MapMOS->Height;

	// FIXME: ugly hack! What is the actual viewport size?
	ViewWidth = core->Width * MAP_DIV / MAP_MULT;
	ViewHeight = core->Height * MAP_DIV / MAP_MULT;

	XCenter = (Width - MapMOS->Width ) / 2;
	YCenter = (Height - MapMOS->Height ) / 2;
	if (XCenter < 0) XCenter = 0;
	if (YCenter < 0) YCenter = 0;
}

void MapControl::RedrawMapControl(char *VariableName, unsigned int Sum)
{
	if (strnicmp( VarName, VariableName, MAX_VARIABLE_LENGTH )) {
		return;
	}
	Value = Sum;
	Changed = true;
}

/** Draws the Control on the Output Display */
void MapControl::Draw(unsigned short XWin, unsigned short YWin)
{
	if (!Width || !Height) {
		return;
	}

	if (Changed) {
		Realize();
		Changed = false;
	}

	Video* video = core->GetVideoDriver();
	Region r( XWin + XPos, YWin + YPos, Width, Height );

	video->BlitSprite( MapMOS, MAP_TO_SCREENX(0), MAP_TO_SCREENY(0), true, &r );

	Region vp = video->GetViewport();

	vp.x = GAME_TO_SCREENX(vp.x);
	vp.y = GAME_TO_SCREENY(vp.y);
	vp.w = ViewWidth;
	vp.h = ViewHeight;

	video->DrawRect( vp, colors[green], false, false );

	int i;
	// Draw PCs' ellipses
	i = core->GetGame()->GetPartySize(false);
	while (i--) {
		Actor* actor = core->GetGame()->GetPC( i );
		if (MyMap->HasActor(actor) ) {
			video->DrawEllipse( GAME_TO_SCREENX(actor->Pos.x), GAME_TO_SCREENY(actor->Pos.y), 3, 2, actor->Selected ? colors[green] : colors[darkgreen], false );
		}
	}
	// Draw Map notes, could be turned off in bg2
	// we use the common control value to handle it, because then we
	// don't need another interface
	if (Value&1) {
		i = MyMap -> GetMapNoteCount();
		while (i--) {
			MapNote * mn = MyMap -> GetMapNote(i);
			Sprite2D *anim = Flag[mn->color&7];
			vp.x = GAME_TO_SCREENX(mn->Pos.x);
			vp.y = GAME_TO_SCREENY(mn->Pos.y);
			if (anim) {
				video->BlitSprite( anim, vp.x, vp.y, true, &r );
			}
			else {
				video->DrawEllipse( vp.x, vp.y, 6, 5, colors[mn->color&7], false );
			}
/*i think none of the games print the notes directly
			vp.x = mn->Pos.x;
			vp.y = mn->Pos.y;
			Font *font = core->GetFont(9);
			font->Print(vp, (unsigned char *) mn->text, colors+(mn->color&7), IE_FONT_ALIGN_TOP, true);
*/
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

	if (Value&1) {
		Point mp = {SCREEN_TO_GAMEX(x),SCREEN_TO_GAMEY(y) };
		int i = MyMap -> GetMapNoteCount();
		while (i--) {
			MapNote * mn = MyMap -> GetMapNote(i);
			if (Distance(mp, mn->Pos)<64) {
				if (LinkedLabel) {
					LinkedLabel->SetText( mn->text );
					printf("%s\n",mn->text);
				}
				//if you ever need something else to do
				//change this to a goto
				return;
			}
		}
	}
	if (LinkedLabel) {
		LinkedLabel->SetText( "" );
        	( ( Window * ) Owner )->Invalidate();
	}
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

