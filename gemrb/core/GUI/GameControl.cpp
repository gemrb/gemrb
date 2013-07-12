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

#include "GUI/GameControl.h"

#include "strrefs.h"
#include "win32def.h"

#include "CharAnimations.h"
#include "DialogHandler.h"
#include "DisplayMessage.h"
#include "Effect.h"
#include "Font.h"
#include "Game.h"
#include "GameData.h"
#include "GlobalTimer.h"
#include "ImageMgr.h"
#include "Interface.h"
#include "Item.h"
#include "PathFinder.h"
#include "SaveGameIterator.h"
#include "ScriptEngine.h"
#include "TableMgr.h"
#include "TileMap.h"
#include "Video.h"
#include "damages.h"
#include "ie_cursors.h"
#include "opcode_params.h"
#include "GameScript/GSUtils.h"
#include "GUI/EventMgr.h"
#include "GUI/TextArea.h"
#include "GUI/Window.h"
#include "Scriptable/Container.h"
#include "Scriptable/Door.h"
#include "Scriptable/InfoPoint.h"

#include <cmath>

namespace GemRB {

#define DEBUG_SHOW_INFOPOINTS   0x01
#define DEBUG_SHOW_CONTAINERS   0x02
#define DEBUG_SHOW_DOORS	DEBUG_SHOW_CONTAINERS
#define DEBUG_SHOW_LIGHTMAP     0x08

static const Color cyan = {
	0x00, 0xff, 0xff, 0xff
};
static const Color red = {
	0xff, 0x00, 0x00, 0xff
};
static const Color magenta = {
	0xff, 0x00, 0xff, 0xff
};
static const Color green = {
	0x00, 0xff, 0x00, 0xff
};
static const Color darkgreen = {
	0x00, 0x78, 0x00, 0xff
};
static Color white = {
	0xff, 0xff, 0xff, 0xff
};

static const Color black = {
	0x00, 0x00, 0x00, 0xff
};
static const Color blue = {
	0x00, 0x00, 0xff, 0x80
};
static const Color gray = {
	0x80, 0x80, 0x80, 0xff
};

#define FORMATIONSIZE 10
typedef Point formation_type[FORMATIONSIZE];
ieDword formationcount;
static formation_type *formations=NULL;
static ieResRef TestSpell="SPWI207";

//If one of the actors has tracking on, the gamecontrol needs to display
//arrow markers on the edges to point at detected monsters
//tracterID is the tracker actor's global ID
//distance is the detection distance
void GameControl::SetTracker(Actor *actor, ieDword dist)
{
	trackerID = actor->GetGlobalID();
	distance = dist;
}

GameControl::GameControl(void)
{
	if (!formations) {
		ReadFormations();
	}
	//this is the default action, individual actors should have one too
	//at this moment we use only this
	//maybe we don't even need it
	Changed = true;
	spellCount = 0;
	user = NULL;
	lastActorID = 0;
	trackerID = 0;
	distance = 0;
	overDoor = NULL;
	overContainer = NULL;
	overInfoPoint = NULL;
	drawPath = NULL;
	pfs.null();
	lastCursor = IE_CURSOR_NORMAL;
	moveX = moveY = 0;
	scrolling = false;
	touchScrollAreasEnabled = false;
	numScrollCursor = 0;
	DebugFlags = 0;
	AIUpdateCounter = 1;
	AlwaysRun = false; //make this a game flag if you wish
	ieDword tmp=0;

	ClearMouseState();
	ResetTargetMode();

	core->GetDictionary()->Lookup("TouchScrollAreas",tmp);
	if (tmp) {
		touchScrollAreasEnabled = true;
		touched = false;
		scrollAreasWidth = 32;
	} else {
		scrollAreasWidth = 5;
	}

	tmp=0;
	core->GetDictionary()->Lookup("Center",tmp);
	if (tmp) {
		ScreenFlags=SF_ALWAYSCENTER|SF_CENTERONACTOR;
	} else {
		ScreenFlags = SF_CENTERONACTOR;
	}
	LeftCount = 0;
	BottomCount = 0;
	RightCount = 0;
	TopCount = 0;
	DialogueFlags = 0;
	dialoghandler = new DialogHandler();
	DisplayText = NULL;
}

//TODO:
//There could be a custom formation which is saved in the save game
//alternatively, all formations could be saved in some compatible way
//so it doesn't cause problems with the original engine
void GameControl::ReadFormations()
{
	unsigned int i,j;
	AutoTable tab("formatio");
	if (!tab) {
		// fallback
		formationcount = 1;
		formations = (formation_type *) calloc(1,sizeof(formation_type) );
		return;
	}
	formationcount = tab->GetRowCount();
	formations = (formation_type *) calloc(formationcount, sizeof(formation_type));
	for(i=0; i<formationcount; i++) {
		for(j=0;j<FORMATIONSIZE;j++) {
			short k=(short) atoi(tab->QueryField(i,j*2));
			formations[i][j].x=k;
			k=(short) atoi(tab->QueryField(i,j*2+1));
			formations[i][j].y=k;
		}
	}
}

//returns a single point offset for a formation
//formation: the formation type
//pos: the actor's slot ID
Point GameControl::GetFormationOffset(ieDword formation, ieDword pos)
{
	if (formation>=formationcount) formation = 0;
	if (pos>=FORMATIONSIZE) pos=FORMATIONSIZE-1;
	return formations[formation][pos];
}

//WARNING: don't pass p as a reference because it gets modified
Point GameControl::GetFormationPoint(Map *map, unsigned int pos, Point src, Point p)
{
	int formation=core->GetGame()->GetFormation();
	if (pos>=FORMATIONSIZE) pos=FORMATIONSIZE-1;

	// calculate angle
	double angle;
	double xdiff = src.x - p.x;
	double ydiff = src.y - p.y;
	if (ydiff == 0) {
		if (xdiff > 0) {
			angle = M_PI_2;
		} else {
			angle = -M_PI_2;
		}
	} else {
		angle = atan(xdiff/ydiff);
		if (ydiff < 0) angle += M_PI;
	}

	// calculate new coordinates by rotating formation around (0,0)
	double newx = -formations[formation][pos].x * cos(angle) + formations[formation][pos].y * sin(angle);
	double newy = formations[formation][pos].x * sin(angle) + formations[formation][pos].y * cos(angle);
	p.x += (int)newx;
	p.y += (int)newy;

	if (p.x < 0) p.x = 8;
	if (p.y < 0) p.y = 8;
	if (p.x > map->GetWidth()*16) p.x = map->GetWidth()*16 - 8;
	if (p.y > map->GetHeight()*12) p.y = map->GetHeight()*12 - 8;

	if(map->GetCursor(p) == IE_CURSOR_BLOCKED) {
		//we can't get there --> adjust position
		p.x/=16;
		p.y/=12;
		map->AdjustPosition(p);
		p.x*=16;
		p.y*=12;
	}
	return p;
}

void GameControl::Center(unsigned short x, unsigned short y)
{
	Video *video = core->GetVideoDriver();
	Region Viewport = video->GetViewport();
	Viewport.x += x - Viewport.w / 2;
	Viewport.y += y - Viewport.h / 2;
	core->timer->SetMoveViewPort( Viewport.x, Viewport.y, 0, false );
	video->MoveViewportTo( Viewport.x, Viewport.y );
}

void GameControl::ClearMouseState()
{
	MouseIsDown = false;
	DrawSelectionRect = false;
	FormationRotation = false;
}

// generate an action to do the actual movement
// only PST supports RunToPoint
void GameControl::CreateMovement(Actor *actor, const Point &p)
{
	char Tmp[256];
	Action *action = NULL;
	static bool CanRun = true;

	//try running (in PST) only if not encumbered
	ieDword speed = actor->CalculateSpeed(true);
	if ( (speed==actor->GetStat(IE_MOVEMENTRATE)) && CanRun && (DoubleClick || AlwaysRun)) {
		sprintf( Tmp, "RunToPoint([%d.%d])", p.x, p.y );
		action = GenerateAction( Tmp );
		//if it didn't work don't insist
		if (!action)
			CanRun = false;
	}
	if (!action) {
		sprintf( Tmp, "MoveToPoint([%d.%d])", p.x, p.y );
		action = GenerateAction( Tmp );
	}

	actor->AddAction( action );
	actor->CommandActor();
}

GameControl::~GameControl(void)
{
	//releasing the viewport of GameControl
	core->GetVideoDriver()->SetViewport( 0,0,0,0 );
	if (formations)	{
		free( formations );
		formations = NULL;
	}
	delete dialoghandler;
	if (DisplayText) {
		core->FreeString(DisplayText);
	}
}

// ArrowSprite cycles
//  321
//  4 0
//  567

#define D_LEFT   1
#define D_UP     2
#define D_RIGHT  4
#define D_BOTTOM 8
// Direction Bits
//  326
//  1 4
//  98c

static const int arrow_orientations[16]={
// 0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
	-1, 4, 2, 3, 0,-1, 1,-1, 6, 5,-1,-1, 7,-1,-1,-1
};

//Draws arrow markers along the edge of the game window
//WARNING:don't use reference for point, because it is altered
void GameControl::DrawArrowMarker(const Region &screen, Point p, const Region &viewport, const Color& color)
{
	Video* video = core->GetVideoDriver();

	//p.x-=viewport.x;
	//p.y-=viewport.y;
	ieDword draw = 0;
	if (p.x<viewport.x) {
		p.x=viewport.x;
		draw|= D_LEFT;
	}
	if (p.y<viewport.y) {
		p.y=viewport.y;
		draw |= D_UP;
	}
	int tmp;

	Sprite2D *spr = core->GetScrollCursorSprite(0,0);

	tmp = spr->Width;
	//tmp = core->ArrowSprites[0]->Width;

	if (p.x>viewport.x+viewport.w-tmp) {
		p.x=viewport.x+viewport.w;//-tmp;
		draw |= D_RIGHT;
	}

	tmp = spr->Height;
	//tmp = core->ArrowSprites[0]->Height;

	if (p.y>viewport.y+viewport.h-tmp) {
		p.y=viewport.y+viewport.h;//-tmp;
		draw |= D_BOTTOM;
	}
	if (arrow_orientations[draw]>=0) {
		Sprite2D *arrow = core->GetScrollCursorSprite(arrow_orientations[draw], 0);
		video->BlitGameSprite(arrow, p.x+screen.x, p.y+screen.y, BLIT_TINTED, color, NULL);
		arrow->release();
	}
	spr->release();
}

void GameControl::DrawTargetReticle(Point p, int size, bool animate, bool flash, bool actorSelected)
{
	// reticles are never drawn in cutscenes
	if (GetScreenFlags()&SF_CUTSCENE)
		return;

	unsigned short step = 0;
	if (animate) {
		// generates "step" from sequence 3 2 1 0 1 2 3 4
		// updated each 1/15 sec
		++step = tp_steps [(GetTickCount() >> 6) & 7];
	} else {
		step = 3;
	}
	if (size < 3) size = 3;

	/* segments should not go outside selection radius */
	unsigned short xradius = (size * 4) - 5;
	unsigned short yradius = (size * 3) - 5;

	Color color = green;
	if (flash) {
		if (step & 2) {
			color = white;
		} else {
			if (!actorSelected) color = darkgreen;
		}
	}

	Region viewport = core->GetVideoDriver()->GetViewport();
	// TODO: 0.5 and 0.7 are pretty much random values
	// right segment
	core->GetVideoDriver()->DrawEllipseSegment( p.x + step - viewport.x, p.y - viewport.y, xradius,
								yradius, color, -0.5, 0.5 );
	// top segment
	core->GetVideoDriver()->DrawEllipseSegment( p.x - viewport.x, p.y - step - viewport.y, xradius,
								yradius, color, -0.7 - M_PI_2, 0.7 - M_PI_2 );
	// left segment
	core->GetVideoDriver()->DrawEllipseSegment( p.x - step - viewport.x, p.y - viewport.y, xradius,
								yradius, color, -0.5 - M_PI, 0.5 - M_PI );
	// bottom segment
	core->GetVideoDriver()->DrawEllipseSegment( p.x - viewport.x, p.y + step - viewport.y, xradius,
								yradius, color, -0.7 - M_PI - M_PI_2, 0.7 - M_PI - M_PI_2 );
}

/** Draws the Control on the Output Display */
void GameControl::Draw(unsigned short x, unsigned short y)
{
	bool update_scripts = !(DialogueFlags & DF_FREEZE_SCRIPTS);

	Game* game = core->GetGame();
	if (!game)
		return;

	if (((short) Width) <=0 || ((short) Height) <= 0) {
		return;
	}

	if (Owner->Visible!=WINDOW_VISIBLE) {
		return;
	}

	Region screen( x + XPos, y + YPos, Width, Height );
	Map *area = core->GetGame()->GetCurrentArea();
	Video* video = core->GetVideoDriver();
	if (!area) {
		video->DrawRect( screen, blue, true );
		return;
	}

	Region viewport = video->GetViewport();
	// handle keeping the actor in the spotlight, but only when unpaused
	if ((ScreenFlags & SF_ALWAYSCENTER) && update_scripts) {
		Actor *star = core->GetFirstSelectedActor();
		moveX = star->Pos.x - viewport.x - viewport.w/2;
		moveY = star->Pos.y - viewport.y - viewport.h/2;
	}

	if (moveX || moveY) {
		viewport.x += moveX;
		viewport.y += moveY;
		Point mapsize = area->TMap->GetMapSize();
		if ( viewport.x < 0 )//if we are at the left of the map
			viewport.x = 0;
		else if ( (viewport.x + viewport.w) >= mapsize.x) //if we are at the right
			viewport.x = mapsize.x - viewport.w - 1;

		if ( viewport.y < 0 ) //if we are at the top of the map
			viewport.y = 0;
		else if ( (viewport.y + viewport.h ) >= mapsize.y ) //if we are at the bottom
			viewport.y = mapsize.y - viewport.h - 1;

		// override any existing viewport moves which may be in progress
		core->timer->SetMoveViewPort( viewport.x, viewport.y, 0, false );
		// move it directly ourselves, since we might be paused
		video->MoveViewportTo( viewport.x, viewport.y );
	}
	video->DrawRect( screen, black, true );

	// setup outlines
	InfoPoint *i;
	unsigned int idx;
	for (idx = 0; (i = area->TMap->GetInfoPoint( idx )); idx++) {
		i->Highlight = false;
		if (overInfoPoint == i && target_mode) {
			if (i->VisibleTrap(0)) {
				i->outlineColor = green;
				i->Highlight = true;
				continue;
			}
		}
		if (i->VisibleTrap(DebugFlags & DEBUG_SHOW_INFOPOINTS)) {
			i->outlineColor = red; // traps
		} else if (DebugFlags & DEBUG_SHOW_INFOPOINTS) {
			i->outlineColor = blue; // debug infopoints
		} else {
			continue;
		}
		i->Highlight = true;
	}

	Door *d;
	for (idx = 0; (d = area->TMap->GetDoor( idx )); idx++) {
		d->Highlight = false;
		if (d->Flags & DOOR_HIDDEN) {
			continue;
		}
		if (overDoor == d) {
			if (target_mode) {
				if (d->Visible() && (d->VisibleTrap(0) || (d->Flags & DOOR_LOCKED))) {
					// only highlight targettable doors
					d->outlineColor = green;
					d->Highlight = true;
					continue;
				}
			} else if (!(d->Flags & DOOR_SECRET)) {
				// mouse over, not in target mode, no secret door
				d->outlineColor = cyan;
				d->Highlight = true;
				continue;
			}
		}
		if (d->VisibleTrap(0)) {
			d->outlineColor = red; // traps
		} else if (d->Flags & DOOR_SECRET) {
			if (DebugFlags & DEBUG_SHOW_DOORS || d->Flags & DOOR_FOUND) {
				d->outlineColor = magenta; // found hidden door
			} else {
				// secret door is invisible
				continue;
			}
		} else if (DebugFlags & DEBUG_SHOW_DOORS) {
			d->outlineColor = cyan; // debug doors
		} else {
			continue;
		}
		d->Highlight = true;
	}

	Container *c;
	for (idx = 0; (c = area->TMap->GetContainer( idx )); idx++) {
		if (c->Flags & CONT_DISABLED) {
			continue;
		}

		c->Highlight = false;
		if (overContainer == c && target_mode) {
			if (c->VisibleTrap(0) || (c->Flags & CONT_LOCKED)) {
				// only highlight targettable containers
				c->outlineColor = green;
				c->Highlight = true;
				continue;
			}
		} else if (overContainer == c) {
			// mouse over, not in target mode
			c->outlineColor = cyan;
			c->Highlight = true;
			continue;
		}
		if (c->VisibleTrap(0)) {
			c->outlineColor = red; // traps
		} else if (DebugFlags & DEBUG_SHOW_CONTAINERS) {
			c->outlineColor = cyan; // debug containers
		} else {
			continue;
		}
		c->Highlight = true;
	}

	//drawmap should be here so it updates fog of war
	area->DrawMap( screen );
	game->DrawWeather(screen, update_scripts);

	if (trackerID) {
		Actor *actor = area->GetActorByGlobalID(trackerID);

		if (actor) {
			Actor **monsters = area->GetAllActorsInRadius(actor->Pos, GA_NO_DEAD|GA_NO_LOS|GA_NO_UNSCHEDULED, distance);

			int i = 0;
			while(monsters[i]) {
				Actor *target = monsters[i++];
				if (target->InParty) continue;
				if (target->GetStat(IE_NOTRACKING)) continue;
				DrawArrowMarker(screen, target->Pos, viewport, black);
			}
			free(monsters);
		} else {
			trackerID = 0;
		}
	}

	if (lastActorID) {
		Actor* actor = GetLastActor();
		if (actor) {
			DrawArrowMarker(screen, actor->Pos, viewport, green);
		}
	}

	if (ScreenFlags & SF_DISABLEMOUSE)
		return;
	Point p(lastMouseX, lastMouseY);
	video->ConvertToGame( p.x, p.y );

	// Draw selection rect
	if (DrawSelectionRect) {
		CalculateSelection( p );
		video->DrawRect( SelectionRect, green, false, true );
	}

	// draw reticles
	if (FormationRotation) {
		FormationApplicationPoint.x = lastMouseX;
		FormationApplicationPoint.y = lastMouseY;
		core->GetVideoDriver()->ConvertToGame( FormationApplicationPoint.x, FormationApplicationPoint.y );

		Actor *actor;
		int max = game->GetPartySize(false);
		// we only care about PCs and not summons for this. the summons will be included in
		// the final mouse up event.
		int formationPos = 0;
		for(int idx = 1; idx<=max; idx++) {
			actor = game->FindPC(idx);
			if(actor->IsSelected()) {
				// transform the formation point
				p = GetFormationPoint(actor->GetCurrentArea(), formationPos++, FormationApplicationPoint, FormationPivotPoint);
				DrawTargetReticle(p, 4, false);
			}
		}
	}

	// Show wallpolygons
	if (DebugFlags & DEBUG_SHOW_INFOPOINTS) {

		unsigned int count = area->GetWallCount();
		for (unsigned int i = 0; i < count; ++i) {
			Wall_Polygon* poly = area->GetWallGroup(i);
			if (!poly) continue;
			// yellow
			Color c;
			c.r = 0x7F;
			c.g = 0x7F;
			c.b = 0;
			c.a = 0;
			//if polygon is disabled, make it grey
			if (poly->wall_flag&WF_DISABLED) {
				c.b = 0x7F;
			}

			video->DrawPolyline( poly, c, true );
		}
	}

	// Draw path
	if (drawPath) {
		PathNode* node = drawPath;
		while (true) {
			Point p( ( node-> x*16) + 8, ( node->y*12 ) + 6 );
			if (!node->Parent) {
				video->DrawCircle( p.x, p.y, 2, red );
			} else {
				short oldX = ( node->Parent-> x*16) + 8, oldY = ( node->Parent->y*12 ) + 6;
				video->DrawLine( oldX, oldY, p.x, p.y, green );
			}
			if (!node->Next) {
				video->DrawCircle( p.x, p.y, 2, green );
				break;
			}
			node = node->Next;
		}
	}

	// Draw lightmap
	if (DebugFlags & DEBUG_SHOW_LIGHTMAP) {
		Sprite2D* spr = area->LightMap->GetSprite2D();
		video->BlitSprite( spr, 0, 0, true );
		video->FreeSprite( spr );
		Region point( p.x / 16, p.y / 12, 2, 2 );
		video->DrawRect( point, red );
	}

	if (core->HasFeature(GF_ONSCREEN_TEXT) && DisplayText) {
		core->GetFont(1)->Print(screen, (unsigned char *)DisplayText, core->InfoTextPalette, IE_FONT_ALIGN_CENTER | IE_FONT_ALIGN_MIDDLE, true);
		if (update_scripts) {
			// just replicating original engine behaviour
			if (DisplayTextTime == 0) {
				SetDisplayText((char *)NULL, 0);
			} else {
				DisplayTextTime--;
			}
		}
	}

	if (touchScrollAreasEnabled) {
		if (moveY < 0 && scrolling)
			video->DrawLine(screen.x+4, screen.y+scrollAreasWidth, screen.w+screen.x-4, screen.y+scrollAreasWidth, red);
		else
			video->DrawLine(screen.x+4, screen.y+scrollAreasWidth, screen.w+screen.x-4, screen.y+scrollAreasWidth, gray);
		if (moveY > 0 && scrolling)
			video->DrawLine(screen.x+4, screen.h-scrollAreasWidth, screen.w+screen.x-4, screen.h-scrollAreasWidth, red);
		else
			video->DrawLine(screen.x+4, screen.h-scrollAreasWidth, screen.w+screen.x-4, screen.h-scrollAreasWidth, gray);
		if (moveX < 0 && scrolling)
			video->DrawLine(screen.x+scrollAreasWidth, screen.y+4, screen.x+scrollAreasWidth, screen.h+screen.y-4, red);
		else
			video->DrawLine(screen.x+scrollAreasWidth, screen.y+4, screen.x+scrollAreasWidth, screen.h+screen.y-4, gray);
		if (moveX > 0 && scrolling)
			video->DrawLine(screen.w+screen.x-scrollAreasWidth, screen.y+4, screen.w+screen.x-scrollAreasWidth, screen.h-4, red);
		else
			video->DrawLine(screen.w+screen.x-scrollAreasWidth, screen.y+4, screen.w+screen.x-scrollAreasWidth, screen.h-4, gray);
	}
}

/** Key Press Event */
bool GameControl::OnKeyPress(unsigned char Key, unsigned short /*Mod*/)
{
	if (DialogueFlags&DF_IN_DIALOG) {
		return false;
	}
	unsigned int i, pc;
	Game* game = core->GetGame();
	if (!game) return false;

	switch (Key) {
		case '0':
			game->SelectActor( NULL, false, SELECT_NORMAL );
			i = game->GetPartySize(false)/2+1;
			while(i--) {
				SelectActor(i, true);
			}
			break;
		case '-':
			game->SelectActor( NULL, true, SELECT_NORMAL );
			i = game->GetPartySize(false)/2+1;
			while(i--) {
				SelectActor(i, false);
			}
			break;
		case '=':
			SelectActor(-1);
			break;
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
			SelectActor(Key-'0');
			break;
		case '7': // 1 & 2
		case '8': // 3 & 4
		case '9': // 5 & 6
			game->SelectActor( NULL, false, SELECT_NORMAL );
			i = game->GetPartySize(false);
			pc = 2*(Key - '6')-1;
			if (pc >= i) {
				SelectActor(i, true);
				break;
			}
			SelectActor(pc, true);
			SelectActor(pc+1, true);
			break;
#ifdef ANDROID
		case 'o':
		case 'p':
			Control::OnKeyPress(Key, 0);
			break;
		case 'c': // show containers in ANDROID, GEM_ALT is not possible to use
			DebugFlags |= DEBUG_SHOW_CONTAINERS;
			break;
#endif
	default:
			return false;
	}
	return true;
}

//Select (or deselect) a new actor (or actors)
void GameControl::SelectActor(int whom, int type)
{
	Game* game = core->GetGame();
	if (whom==-1) {
		game->SelectActor( NULL, true, SELECT_NORMAL );
		return;
	}

	/* doesn't fall through here */
	Actor* actor = game->FindPC( whom );
	if (!actor)
		return;

	if (type==0) {
		game->SelectActor( actor, false, SELECT_NORMAL );
		return;
	}
	if (type==1) {
		game->SelectActor( actor, true, SELECT_NORMAL );
		actor->PlaySelectionSound();
		return;
	}

	bool was_selected = actor->IsSelected();
	if (game->SelectActor( actor, true, SELECT_REPLACE )) {
		if (was_selected || (ScreenFlags & SF_ALWAYSCENTER)) {
			ScreenFlags |= SF_CENTERONACTOR;
		}
		actor->PlaySelectionSound();
	}
}

//Effect for the ctrl-r cheatkey (resurrect)
static EffectRef heal_ref = { "CurrentHPModifier", -1 };
static EffectRef damage_ref = { "Damage", -1 };

/** Key Release Event */
bool GameControl::OnKeyRelease(unsigned char Key, unsigned short Mod)
{
	unsigned int i;
	Game* game = core->GetGame();

	if (!game)
		return false;

	if (DialogueFlags&DF_IN_DIALOG) {
		if (Mod) return false;
		switch(Key) {
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				{
					TextArea *ta = core->GetMessageTextArea();
					if (ta) {
						return ta->OnKeyPress(Key,Mod);
					}
				}
				break;
		}
		return false;
	}
	if (Mod & GEM_MOD_SHIFT) {
		Key = toupper(Key);
	}

	//cheatkeys with ctrl-
	if (Mod & GEM_MOD_CTRL) {
		if (!core->CheatEnabled()) {
			return false;
		}
		Map* area = game->GetCurrentArea( );
		if (!area)
			return false;
		Actor *lastActor = area->GetActorByGlobalID(lastActorID);
		Point p(lastMouseX, lastMouseY);
		core->GetVideoDriver()->ConvertToGame( p.x, p.y );
		switch (Key) {
			case 'd': //detect a trap or door
				if (overInfoPoint) {
					overInfoPoint->DetectTrap(256, lastActorID);
				}
				if (overContainer) {
					overContainer->DetectTrap(256, lastActorID);
				}
				if (overDoor) {
					overDoor->TryDetectSecret(256, lastActorID);
					overDoor->DetectTrap(256, lastActorID);
				}
				break;
			case 'l': //play an animation (vvc/bam) over an actor
				//the original engine was able to swap through all animations
				if (lastActor) {
					lastActor->AddAnimation("S056ICBL", 0, 0, 0);
				}
				break;

			case 'c': //force cast a hardcoded spell
				//caster is the last selected actor
				//target is the door/actor currently under the pointer
				if (game->selected.size() > 0) {
					Actor *src = game->selected[0];
					Scriptable *target = lastActor;
					if (overDoor) {
						target = overDoor;
					}
					if (target) {
						src->SetSpellResRef(TestSpell);
						src->CastSpell(target, false);
						if (src->LastTarget) {
							src->CastSpellEnd(0, 0);
						} else {
							src->CastSpellPointEnd(0, 0);
						}
					}
				}
				break;

			case 'b': //draw a path to the target (pathfinder debug)
				//You need to select an origin with ctrl-o first
				if (drawPath) {
					PathNode* nextNode = drawPath->Next;
					PathNode* thisNode = drawPath;
					while (true) {
						delete( thisNode );
						thisNode = nextNode;
						if (!thisNode)
							break;
						nextNode = thisNode->Next;
					}
				}
				drawPath = core->GetGame()->GetCurrentArea()->FindPath( pfs, p, lastActor?lastActor->size:1 );

				break;

			case 'o': //set up the origin for the pathfinder
				// origin
				pfs.x = lastMouseX;
				pfs.y = lastMouseY;
				core->GetVideoDriver()->ConvertToGame( pfs.x, pfs.y );
				break;
			case 'a': //switches through the avatar animations
				if (lastActor) {
					lastActor->GetNextAnimation();
				}
				break;
			case 's': //switches through the stance animations
				if (lastActor) {
					lastActor->GetNextStance();
				}
				break;
			case 'j': //teleports the selected actors
				for (i = 0; i < game->selected.size(); i++) {
					Actor* actor = game->selected[i];
					actor->ClearActions();
					MoveBetweenAreasCore(actor, core->GetGame()->CurrentArea, p, -1, true);
				}
				break;

			case 'M':
				if (!lastActor) {
					lastActor = area->GetActor( p, GA_DEFAULT);
				}
				if (!lastActor) {
					// ValidTarget never returns immobile targets, making debugging a nightmare
					// so if we don't have an actor, we make really really sure by checking manually
					unsigned int count = area->GetActorCount(true);
					while (count--) {
						Actor *actor = area->GetActor(count, true);
						if (actor->IsOver(p)) {
							actor->GetAnims()->DebugDump();
						}
					}
				}
				if (lastActor) {
					lastActor->GetAnims()->DebugDump();
					break;
				}
				break;
			case 'm': //prints a debug dump (ctrl-m in the original game too)
				if (!lastActor) {
					lastActor = area->GetActor( p, GA_DEFAULT);
				}
				if (!lastActor) {
					// ValidTarget never returns immobile targets, making debugging a nightmare
					// so if we don't have an actor, we make really really sure by checking manually
					unsigned int count = area->GetActorCount(true);
					while (count--) {
						Actor *actor = area->GetActor(count, true);
						if (actor->IsOver(p)) {
							actor->dump();
						}
					}
				}
				if (lastActor) {
					lastActor->dump();
					break;
				}
				if (overDoor) {
					overDoor->dump();
					break;
				}
				if (overContainer) {
					overContainer->dump();
					break;
				}
				if (overInfoPoint) {
					overInfoPoint->dump();
					break;
				}
				core->GetGame()->GetCurrentArea()->dump(false);
				break;
			case 'N': //prints a list of all the live actors in the area
				core->GetGame()->GetCurrentArea()->dump(true);
				break;
			case 'v': //marks some of the map visited (random vision distance)
				area->ExploreMapChunk( p, rand()%30, 1 );
				break;
			case 'V': //
				core->GetDictionary()->DebugDump();
				break;
			case 'w': // consolidates found ground piles under the pointed pc
				area->MoveVisibleGroundPiles(p);
				break;
			case 'x': // shows coordinates on the map
				Log(MESSAGE, "GameControl", "Position: %s [%d.%d]", area->GetScriptName(), p.x, p.y );
				break;
			case 'g'://shows loaded areas and other game information
				game->dump();
				break;
			case 'i'://interact trigger (from the original game)
				if (!lastActor) {
					lastActor = area->GetActor( p, GA_DEFAULT);
				}
				if (lastActor && !(lastActor->GetStat(IE_MC_FLAGS)&MC_EXPORTABLE)) {
					Actor *target;
					int i = game->GetPartySize(true);
					if(i<2) break;
					i=rand()%i;
					do
					{
						target = game->GetPC(i, true);
						if(target==lastActor) continue;
						if(target->GetStat(IE_MC_FLAGS)&MC_EXPORTABLE) continue;

						char Tmp[40];
						snprintf(Tmp,sizeof(Tmp),"Interact(\"%s\")",target->GetScriptName() );
						lastActor->AddAction(GenerateAction(Tmp));
						break;
					}
					while(i--);
				}
				break;
			case 'r'://resurrects actor
				if (!lastActor) {
					lastActor = area->GetActor( p, GA_DEFAULT);
				}
				if (lastActor) {
					Effect *fx = EffectQueue::CreateEffect(heal_ref, lastActor->GetStat(IE_MAXHITPOINTS), 0x30001, FX_DURATION_INSTANT_PERMANENT);
					if (fx) {
						core->ApplyEffect(fx, lastActor, lastActor);
					}
					delete fx;
				}
				break;
			case 't'://advances time
				// 7200 (one day) /24 (hours) == 300
				game->AdvanceTime(300*AI_UPDATE_TIME);
				//refresh gui here once we got it
				break;

			case 'q': //joins actor to the party
				if (lastActor && !lastActor->InParty) {
					lastActor->ClearActions();
					lastActor->ClearPath();
					char Tmp[40];
					strlcpy(Tmp, "JoinParty()", sizeof(Tmp));
					lastActor->AddAction( GenerateAction(Tmp) );
				}
				break;
			case 'p': //center on actor
				ScreenFlags|=SF_CENTERONACTOR;
				ScreenFlags^=SF_ALWAYSCENTER;
				break;
			case 'k': //kicks out actor
				if (lastActor && lastActor->InParty) {
					lastActor->ClearActions();
					lastActor->ClearPath();
					char Tmp[40];
					strlcpy(Tmp, "LeaveParty()", sizeof(Tmp));
					lastActor->AddAction( GenerateAction(Tmp) );
				}
				break;
			case 'Y': // damages all enemies by 300 (resistances apply)
				// mwahaha!
				Effect *newfx;
				newfx = EffectQueue::CreateEffect(damage_ref, 300, DAMAGE_MAGIC<<16, FX_DURATION_INSTANT_PERMANENT);
				Actor *victim;
				i = area->GetActorCount(0);
				while(i--) {
					victim = area->GetActor(i, 0);
					if (victim->Modified[IE_EA] == EA_ENEMY) {
						core->ApplyEffect(newfx, victim, victim);
					}
				}
				delete newfx;
			case 'y': //kills actor
				if (lastActor) {
					//using action so the actor is killed
					//correctly (synchronisation)
					lastActor->ClearActions();
					lastActor->ClearPath();

					Effect *newfx;
					newfx = EffectQueue::CreateEffect(damage_ref, 300, DAMAGE_MAGIC<<16, FX_DURATION_INSTANT_PERMANENT);
					core->ApplyEffect(newfx, lastActor, lastActor);
					delete newfx;
					if (! (lastActor->GetInternalFlag() & IF_REALLYDIED)) {
						newfx = EffectQueue::CreateEffect(damage_ref, 300, DAMAGE_ACID<<16, FX_DURATION_INSTANT_PERMANENT);
						core->ApplyEffect(newfx, lastActor, lastActor);
						delete newfx;
						newfx = EffectQueue::CreateEffect(damage_ref, 300, DAMAGE_CRUSHING<<16, FX_DURATION_INSTANT_PERMANENT);
						core->ApplyEffect(newfx, lastActor, lastActor);
						delete newfx;
					}
				} else if (overContainer) {
					overContainer->SetContainerLocked(0);
				} else if (overDoor) {
					overDoor->SetDoorLocked(0,0);
				}
				break;
			case 'z': //shift through the avatar animations backward
				if (lastActor) {
					lastActor->GetPrevAnimation();
				}
				break;
			case '1': //change paperdoll armour level
				if (! lastActor)
					break;
				lastActor->NewStat(IE_ARMOR_TYPE,1,MOD_ADDITIVE);
				break;
			case '4': //show all traps and infopoints
				DebugFlags ^= DEBUG_SHOW_INFOPOINTS;
				Log(MESSAGE, "GameControl", "Show traps and infopoints %s", DebugFlags & DEBUG_SHOW_INFOPOINTS ? "ON" : "OFF");
				break;
			case '6': //show the lightmap
				DebugFlags ^= DEBUG_SHOW_LIGHTMAP;
				Log(MESSAGE, "GameControl", "Show lightmap %s", DebugFlags & DEBUG_SHOW_LIGHTMAP ? "ON" : "OFF");
				break;
			case '7': //toggles fog of war
				core->FogOfWar ^= FOG_DRAWFOG;
				Log(MESSAGE, "GameControl", "Show Fog-Of-War: %s", core->FogOfWar & FOG_DRAWFOG ? "ON" : "OFF");
				break;
			case '8': //show searchmap over area
				core->FogOfWar ^= FOG_DRAWSEARCHMAP;
				Log(MESSAGE, "GameControl", "Show searchmap %s", core->FogOfWar & FOG_DRAWSEARCHMAP ? "ON" : "OFF");
				break;
			default:
				Log(MESSAGE, "GameControl", "KeyRelease:%d - %d", Key, Mod );
				break;
		}
		return true; //return from cheatkeys
	}
	switch (Key) {
//FIXME: move these to guiscript
		case 'h': //hard pause
			core->SetPause(PAUSE_ON);
			break;
		case ' ': //soft pause
			core->TogglePause();
			break;
		case GEM_ALT: //alt key (shows containers)
#ifdef ANDROID
		case 'c': // show containers in ANDROID, GEM_ALT is not possible to use
#endif
			DebugFlags &= ~DEBUG_SHOW_CONTAINERS;
			break;
		default:
			return false;
	}
	return true;
}

void GameControl::DisplayTooltip() {
	Game* game = core->GetGame();
	if (game && !(ScreenFlags & SF_DISABLEMOUSE)) {
		Map* area = game->GetCurrentArea( );
		if (area) {
			Actor *actor = area->GetActorByGlobalID(lastActorID);
			if (actor && (actor->GetStat(IE_STATE_ID)&STATE_DEAD || actor->GetInternalFlag()&IF_REALLYDIED)) {
				// no tooltips for dead actors!
				actor->SetOver( false );
				lastActorID = 0;
				actor = NULL;
			}

			if (actor) {
				char *name = actor->GetName(-1);
				int hp = actor->GetStat(IE_HITPOINTS);
				int maxhp = actor->GetStat(IE_MAXHITPOINTS);

				char buffer[100];
				if (!core->TooltipBack) {
					// single-line tooltips without background (PS:T)
					if (actor->InParty) {
						snprintf(buffer, 100, "%s: %d/%d", name, hp, maxhp);
					} else {
						snprintf(buffer, 100, "%s", name);
					}
				} else {
					// a guess at a neutral check
					bool neutral = actor->GetStat(IE_EA) == EA_NEUTRAL;
					// test for an injured string being present for this game
					int strindex = displaymsg->GetStringReference(STR_UNINJURED);
					// normal tooltips
					if (actor->InParty) {
						// in party: display hp
						snprintf(buffer, 100, "%s\n%d/%d", name, hp, maxhp);
					} else if (neutral) {
						// neutral: display name only
						snprintf(buffer, 100, "%s", name);
					} else if (strindex == -1) {
						// non-neutral, not in party, no injured strings: display name
						// this case is mostly hit in bg1
						snprintf(buffer, 100, "%s", name);
					} else {
						// non-neutral, not in party: display injured string
						int strindex;
						char *injuredstring = NULL;
						// these boundaries are just a guess
						if (hp == maxhp) {
							strindex = STR_UNINJURED;
						} else if (hp > (maxhp*3)/4) {
							strindex = STR_INJURED1;
						} else if (hp > maxhp/2) {
							strindex = STR_INJURED2;
						} else if (hp > maxhp/3) {
							strindex = STR_INJURED3;
						} else {
							strindex = STR_INJURED4;
						}
						strindex = displaymsg->GetStringReference(strindex);
						if (strindex != -1) {
							injuredstring = core->GetString(strindex, 0);
						}

						if (!injuredstring) {
							// eek, where did the string go?
							snprintf(buffer, 100, "%s\n%d/%d", name, hp, maxhp);
						} else {
							snprintf(buffer, 100, "%s\n%s", name, injuredstring);
							free(injuredstring);
						}
					}
				}

				Point p = actor->Pos;
				core->GetVideoDriver()->ConvertToScreen( p.x, p.y );
				p.x += Owner->XPos + XPos;
				p.y += Owner->YPos + YPos;

				// hack to position text above PS:T actors
				if (!core->TooltipBack) p.y -= actor->size*50;

				// we should probably cope better with moving actors
				SetTooltip(buffer);
				core->DisplayTooltip(p.x, p.y, this);
				return;
			}
		}
	}

	SetTooltip(NULL);
	core->DisplayTooltip(0, 0, NULL);
	return;
}

//returns the appropriate cursor over an active region (trap, infopoint, travel region)
int GameControl::GetCursorOverInfoPoint(InfoPoint *overInfoPoint) const
{
	if (target_mode == TARGET_MODE_PICK) {
		if (overInfoPoint->VisibleTrap(0)) {
			return IE_CURSOR_TRAP;
		}

		return IE_CURSOR_STEALTH|IE_CURSOR_GRAY;
	}
	// traps always display a walk cursor?
	if (overInfoPoint->Type == ST_PROXIMITY) {
		return IE_CURSOR_WALK;
	}
	return overInfoPoint->Cursor;
}

//returns the appropriate cursor over a door
int GameControl::GetCursorOverDoor(Door *overDoor) const
{
	if (!overDoor->Visible()) {
		if (target_mode == TARGET_MODE_NONE) {
			return IE_CURSOR_BLOCKED;
		} else {
			return lastCursor|IE_CURSOR_GRAY;
		}
	}
	if (target_mode == TARGET_MODE_PICK) {
		if (overDoor->VisibleTrap(0)) {
			return IE_CURSOR_TRAP;
		}
		if (overDoor->Flags & DOOR_LOCKED) {
			return IE_CURSOR_LOCK;
		}

		return IE_CURSOR_STEALTH|IE_CURSOR_GRAY;
	}
	return overDoor->Cursor;
}

//returns the appropriate cursor over a container (or pile)
int GameControl::GetCursorOverContainer(Container *overContainer) const
{
	if (overContainer->Flags & CONT_DISABLED) {
		return lastCursor;
	}

	if (target_mode == TARGET_MODE_PICK) {
		if (overContainer->VisibleTrap(0)) {
			return IE_CURSOR_TRAP;
		}
		if (overContainer->Flags & CONT_LOCKED) {
			return IE_CURSOR_LOCK2;
		}

		return IE_CURSOR_STEALTH|IE_CURSOR_GRAY;
	}
	return IE_CURSOR_TAKE;
}

int GameControl::GetDefaultCursor() const
{
	switch(target_mode) {
	case TARGET_MODE_TALK:
			return IE_CURSOR_TALK;
		case TARGET_MODE_ATTACK:
			return IE_CURSOR_ATTACK;
		case TARGET_MODE_CAST:
			return IE_CURSOR_CAST;
		case TARGET_MODE_DEFEND:
			return IE_CURSOR_DEFEND;
		case TARGET_MODE_PICK:
			return IE_CURSOR_PICK;
		}
	return IE_CURSOR_NORMAL;
}

/** Mouse Over Event */
void GameControl::OnMouseOver(unsigned short x, unsigned short y)
{
	if (ScreenFlags & SF_DISABLEMOUSE) {
		return;
	}

	Video *video = core->GetVideoDriver();

	if (touchScrollAreasEnabled) {
		int mousescrollspd = core->GetMouseScrollSpeed();
		Region region;
		Map* map;
		Point mapsize;
		Region viewport = video->GetViewport();
		moveX = 0;
		moveY = 0;
		// Top scroll area
		region=Region(XPos, YPos, Width, YPos+scrollAreasWidth);
		if (region.PointInside(x, y)) {
			// Check for end of map area
			if (viewport.y > 0)
				moveY = -mousescrollspd;
		}
		// Bottom scroll area
		region=Region(XPos, Height-scrollAreasWidth, Width, Height);
		if (region.PointInside(x, y)) {
			// Check for end of map area
			map = core->GetGame()->GetCurrentArea();
			if (map != NULL) {
				mapsize = map->TMap->GetMapSize();
				if((viewport.y + viewport.h) < mapsize.y)
					moveY = mousescrollspd;
			}
		}
		// Left scroll area
		region=Region(XPos, YPos, XPos+scrollAreasWidth, Height);
		if (region.PointInside(x, y)) {
			// Check for end of map area
			if(viewport.x > 0)
				moveX = -mousescrollspd;
		}
		// Right scroll area
		region=Region(Width-scrollAreasWidth, YPos, Width, Height);
		if (region.PointInside(x, y)) {
			// Check for end of map area
			map = core->GetGame()->GetCurrentArea();
			if (map != NULL) {
				mapsize = map->TMap->GetMapSize();
				if((viewport.x + viewport.w) < mapsize.x)
					moveX = mousescrollspd;
			}
		}
		if ((moveX != 0 || moveY != 0) && touched) {
			SetScrolling(true);
			return;
		} else {
			SetScrolling(false);
		}
	}

	lastMouseX = x;
	lastMouseY = y;
	Point p( x,y );
	video->ConvertToGame( p.x, p.y );
	if (MouseIsDown && ( !DrawSelectionRect )) {
		if (( abs( p.x - StartX ) > 5 ) || ( abs( p.y - StartY ) > 5 )) {
			DrawSelectionRect = true;
		}
	}
	Game* game = core->GetGame();
	if (!game) return;
	Map* area = game->GetCurrentArea( );
	if (!area) return;
	int nextCursor = area->GetCursor( p );
	//make the invisible area really invisible
	if (nextCursor == IE_CURSOR_INVALID) {
		Owner->Cursor = IE_CURSOR_BLOCKED;
		lastCursor = IE_CURSOR_BLOCKED;
		return;
	}

	overInfoPoint = area->TMap->GetInfoPoint( p, true );
	if (overInfoPoint) {
		//nextCursor = overInfoPoint->Cursor;
		nextCursor = GetCursorOverInfoPoint(overInfoPoint);
	}

	if (overDoor) {
		overDoor->Highlight = false;
	}
	if (overContainer) {
		overContainer->Highlight = false;
	}
	Actor *lastActor = area->GetActorByGlobalID(lastActorID);
	if (lastActor) {
		lastActor->SetOver( false );
	}

	overDoor = area->TMap->GetDoor( p );
	overContainer = area->TMap->GetContainer( p );

	if (!DrawSelectionRect) {
		if (FormationRotation) {
			nextCursor = IE_CURSOR_USE;
			goto end_function;
		}
		if (overDoor) {
			nextCursor = GetCursorOverDoor(overDoor);
		}

		if (overContainer) {
			nextCursor = GetCursorOverContainer(overContainer);
		}

		Actor *prevActor = lastActor;
		// let us target party members even if they are invisible
		lastActor = area->GetActor(p, GA_NO_DEAD|GA_NO_UNSCHEDULED);
		if (lastActor && lastActor->Modified[IE_EA]>=EA_CONTROLLED) {
			if (!lastActor->ValidTarget(target_types)) {
				lastActor = NULL;
			}
		}
		if (lastActor != prevActor) {
			// we store prevActor so we can remove the tooltip on actor change
			// (maybe we should be checking this and actor movements every frame?)
			SetTooltip(NULL);
			core->DisplayTooltip(0, 0, this);
		}

		if ((target_types & GA_NO_SELF) && lastActor ) {
			if (lastActor == core->GetFirstSelectedActor()) {
				lastActor=NULL;
			}
		}

		if (lastActor) {
			lastActorID = lastActor->GetGlobalID();
			lastActor->SetOver( true );
			ieDword type = lastActor->GetStat(IE_EA);
			if (type >= EA_EVILCUTOFF || type == EA_GOODBUTRED) {
				nextCursor = IE_CURSOR_ATTACK;
			} else if ( type > EA_CHARMED ) {
				nextCursor = IE_CURSOR_TALK;
				//don't let the pc to talk to frozen/stoned creatures
				ieDword state = lastActor->GetStat(IE_STATE_ID);
				if (state & (STATE_CANTMOVE^STATE_SLEEP)) {
					nextCursor |= IE_CURSOR_GRAY;
				}
			} else {
				nextCursor = IE_CURSOR_NORMAL;
			}
		} else {
			lastActorID = 0;
		}

		if (target_mode == TARGET_MODE_TALK) {
			nextCursor = IE_CURSOR_TALK;
			if (!lastActor) {
				nextCursor |= IE_CURSOR_GRAY;
			} else {
				//don't let the pc to talk to frozen/stoned creatures
				ieDword state = lastActor->GetStat(IE_STATE_ID);
				if (state & (STATE_CANTMOVE^STATE_SLEEP)) {
					nextCursor |= IE_CURSOR_GRAY;
				}
			}
		} else if (target_mode == TARGET_MODE_ATTACK) {
			nextCursor = IE_CURSOR_ATTACK;
			if (overDoor) {
				if (!overDoor->Visible()) {
					nextCursor |= IE_CURSOR_GRAY;
				}
			} else if (!lastActor && !overContainer) {
				nextCursor |= IE_CURSOR_GRAY;
			}
		} else if (target_mode == TARGET_MODE_CAST) {
			nextCursor = IE_CURSOR_CAST;
			//point is always valid
			if (!(target_types & GA_POINT)) {
				if(!lastActor) {
					nextCursor |= IE_CURSOR_GRAY;
				}
			}
		} else if (target_mode == TARGET_MODE_DEFEND) {
			nextCursor = IE_CURSOR_DEFEND;
			if(!lastActor) {
				nextCursor |= IE_CURSOR_GRAY;
			}
		} else if (target_mode == TARGET_MODE_PICK) {
			if (lastActor) {
				nextCursor = IE_CURSOR_PICK;
			} else {
				if (!overContainer && !overDoor && !overInfoPoint) {
					nextCursor = IE_CURSOR_STEALTH|IE_CURSOR_GRAY;
				}
			}
			goto end_function;
		}

		if (lastActor) {
			switch (lastActor->GetStat(IE_EA)) {
				case EA_EVILCUTOFF:
				case EA_GOODCUTOFF:
					break;

				case EA_PC:
				case EA_FAMILIAR:
				case EA_ALLY:
				case EA_CONTROLLED:
				case EA_CHARMED:
				case EA_EVILBUTGREEN:
					if (target_types & GA_NO_ENEMY)
						nextCursor^=1;
					break;

				case EA_ENEMY:
				case EA_GOODBUTRED:
					if (target_types & GA_NO_ALLY)
						nextCursor^=1;
					break;
				default:
					if (!(target_types & GA_NO_NEUTRAL))
						nextCursor^=1;
					break;
			}
		}
	}
end_function:
	if (lastCursor != nextCursor) {
		Owner->Cursor = nextCursor;
		lastCursor = (unsigned char) nextCursor;
	}
}

/** Global Mouse Move Event */
#if TARGET_OS_IPHONE
// iOS will never have a mouse.
void GameControl::OnGlobalMouseMove(unsigned short /*x*/, unsigned short /*y*/) {}
#else
void GameControl::OnGlobalMouseMove(unsigned short x, unsigned short y)
{
	if (ScreenFlags & SF_DISABLEMOUSE) {
		return;
	}

	if (Owner->Visible!=WINDOW_VISIBLE) {
		return;
	}

	if (!touchScrollAreasEnabled) {
		int mousescrollspd = core->GetMouseScrollSpeed();

		if (x <= scrollAreasWidth)
			moveX = -mousescrollspd;
		else {
			if (x >= ( core->Width - scrollAreasWidth ))
				moveX = mousescrollspd;
			else
				moveX = 0;
		}
		if (y <= scrollAreasWidth)
			moveY = -mousescrollspd;
		else {
			if (y >= ( core->Height - scrollAreasWidth ))
				moveY = mousescrollspd;
			else
				moveY = 0;
		}

		SetScrolling(moveX != 0 || moveY != 0);
	}
}
#endif

void GameControl::UpdateScrolling() {
	// mouse scroll speed is checked because scrolling is not always done by the mouse (ie cutscenes/keyboard/etc)
	if (!scrolling || !core->GetMouseScrollSpeed() || (moveX == 0 && moveY == 0)) return;

	int cursorFrame = 0; // right
	if (moveY < 0) {
		cursorFrame = 2; // up
		if (moveX > 0) cursorFrame--; // +right
		else if (moveX < 0) cursorFrame++; // +left
	} else if (moveY > 0) {
		cursorFrame = 6; // down
		if (moveX > 0) cursorFrame++; // +right
		else if (moveX < 0) cursorFrame--; // +left
	} else if (moveX < 0) {
		cursorFrame = 4; // left
	}

	Sprite2D* cursor = core->GetScrollCursorSprite(cursorFrame, numScrollCursor);
	Video* video = core->GetVideoDriver();
	video->SetCursor(cursor, VID_CUR_DRAG);
	video->FreeSprite(cursor);

	numScrollCursor = (numScrollCursor+1) % 15;
}

void GameControl::SetScrolling(bool scroll) {
	if (scrolling != scroll) {
		scrolling = scroll;
		if (!scrolling) {
			moveX = 0;
			moveY = 0;

			// only clear the drag cursor when changing scrolling to false!
			// clearing on every move kills drag operations such as dragging portraits
			core->GetVideoDriver()->SetCursor(NULL, VID_CUR_DRAG);
		}
	}
}

//generate action code for source actor to try to attack a target
void GameControl::TryToAttack(Actor *source, Actor *tgt)
{
	char Tmp[40];

	source->ClearPath();
	source->ClearActions();
	strlcpy(Tmp, "NIDSpecial3()", sizeof(Tmp));
	source->AddAction( GenerateActionDirect( Tmp, tgt) );
	source->CommandActor();
}

//generate action code for source actor to try to defend a target
void GameControl::TryToDefend(Actor *source, Actor *tgt)
{
	char Tmp[40];

	source->ClearPath();
	source->ClearActions();
	source->SetModal(MS_NONE);
	strlcpy(Tmp, "NIDSpecial4()", sizeof(Tmp));
	source->AddAction( GenerateActionDirect( Tmp, tgt) );
	source->CommandActor();
}

// generate action code for source actor to try to pick pockets of a target (if an actor)
// else if door/container try to pick a lock/disable trap
// The -1 flag is a placeholder for dynamic target IDs
void GameControl::TryToPick(Actor *source, Scriptable *tgt)
{
	char Tmp[40];

	source->ClearPath();
	source->ClearActions();
	source->SetModal(MS_NONE);
	if (tgt->Type == ST_ACTOR) {
		strlcpy(Tmp, "PickPockets([-1])", sizeof(Tmp));
	} else if (tgt->Type == ST_DOOR || tgt->Type == ST_CONTAINER) {
		if (((Highlightable*)tgt)->Trapped && ((Highlightable*)tgt)->TrapDetected) {
			strlcpy(Tmp, "RemoveTraps([-1])", sizeof(Tmp));
		} else {
			strlcpy(Tmp, "PickLock([-1])", sizeof(Tmp));
		}
	} else {
		Log(ERROR, "GameControl", "Invalid pick target of type %d", tgt->Type);
		return;
	}
	source->AddAction( GenerateActionDirect( Tmp, tgt) );
	source->CommandActor();
}

//generate action code for source actor to try to disable trap (only trap type active regions)
void GameControl::TryToDisarm(Actor *source, InfoPoint *tgt)
{
	if (tgt->Type!=ST_PROXIMITY) return;

	char Tmp[40];

	source->ClearPath();
	source->ClearActions();
	source->SetModal(MS_NONE);
	strlcpy(Tmp, "RemoveTraps([-1])", sizeof(Tmp));
	source->AddAction( GenerateActionDirect( Tmp, tgt ) );
	source->CommandActor();
}

//generate action code for source actor to use item/cast spell on a point
void GameControl::TryToCast(Actor *source, const Point &tgt)
{
	char Tmp[40];

	if (!spellCount) {
		ResetTargetMode();
		return; //not casting or using an own item
	}
	source->ClearPath();
	source->ClearActions();

	spellCount--;
	if (spellOrItem>=0) {
		if (spellIndex<0) {
			strlcpy(Tmp, "SpellPointNoDec(\"\",[0.0])", sizeof(Tmp));
		} else {
			strlcpy(Tmp, "SpellPoint(\"\",[0.0])", sizeof(Tmp));
		}
	} else {
		//using item on target
		strlcpy(Tmp, "UseItemPoint(\"\",[0,0],0)", sizeof(Tmp));
	}
	Action* action = GenerateAction( Tmp );
	action->pointParameter=tgt;
	if (spellOrItem>=0) {
		if (spellIndex<0) {
			sprintf(action->string0Parameter,"%.8s",spellName);
		} else {
			CREMemorizedSpell *si;
			//spell casting at target
			si = source->spellbook.GetMemorizedSpell(spellOrItem, spellSlot, spellIndex);
			if (!si) {
				ResetTargetMode();
				return;
			}
			sprintf(action->string0Parameter,"%.8s",si->SpellResRef);
		}
	} else {
		action->int0Parameter = spellSlot;
		action->int1Parameter = spellIndex;
		action->int2Parameter = UI_SILENT;
	}
	source->AddAction( action );
	if (!spellCount) {
		ResetTargetMode();
	}
}

//generate action code for source actor to use item/cast spell on another actor
void GameControl::TryToCast(Actor *source, Actor *tgt)
{
	char Tmp[40];

	if (!spellCount) {
		ResetTargetMode();
		return; //not casting or using an own item
	}
	source->ClearPath();
	source->ClearActions();

	// cannot target spells on invisible or sanctuaried creatures
	// invisible actors are invisible, so this is usually impossible by itself, but improved invisibility changes that
	if (source != tgt && tgt->Untargetable() ) {
		displaymsg->DisplayConstantStringName(STR_NOSEE_NOCAST, DMC_RED, source);
		ResetTargetMode();
		return;
	}

	spellCount--;
	if (spellOrItem>=0) {
		if (spellIndex<0) {
			sprintf(Tmp, "NIDSpecial7()");
		} else {
			sprintf(Tmp, "NIDSpecial6()");
		}
	} else {
		//using item on target
		sprintf(Tmp, "NIDSpecial5()");
	}
	Action* action = GenerateActionDirect( Tmp, tgt);
	if (spellOrItem>=0) {
		if (spellIndex<0) {
			sprintf(action->string0Parameter,"%.8s",spellName);
		} else {
			CREMemorizedSpell *si;
			//spell casting at target
			si = source->spellbook.GetMemorizedSpell(spellOrItem, spellSlot, spellIndex);
			if (!si) {
				ResetTargetMode();
				return;
			}
			sprintf(action->string0Parameter,"%.8s",si->SpellResRef);
		}
	} else {
		action->int0Parameter = spellSlot;
		action->int1Parameter = spellIndex;
		action->int2Parameter = UI_SILENT;
	}
	source->AddAction( action );
	if (!spellCount) {
		ResetTargetMode();
	}
}

//generate action code for source actor to use talk to target actor
void GameControl::TryToTalk(Actor *source, Actor *tgt)
{
	char Tmp[40];

	//Nidspecial1 is just an unused action existing in all games
	//(non interactive demo)
	//i found no fitting action which would emulate this kind of
	//dialog initation
	source->ClearPath();
	source->ClearActions();
	source->SetModal(MS_NONE);
	strlcpy(Tmp, "NIDSpecial1()", sizeof(Tmp));
	dialoghandler->targetID = tgt->GetGlobalID(); //this is a hack, but not so deadly
	source->AddAction( GenerateActionDirect( Tmp, tgt) );
	source->CommandActor();
}

//generate action code for actor appropriate for the target mode when the target is a container
void GameControl::HandleContainer(Container *container, Actor *actor)
{
	char Tmp[256];

	//container is disabled, it should not react
	if (container->Flags & CONT_DISABLED) {
		return;
	}

	if ((target_mode == TARGET_MODE_CAST) && spellCount) {
		//we'll get the container back from the coordinates
		TryToCast(actor, container->Pos);
		//Do not reset target_mode, TryToCast does it for us!!
		return;
	}

	core->SetEventFlag(EF_RESETTARGET);

	if (target_mode == TARGET_MODE_ATTACK) {
		actor->ClearPath();
		actor->ClearActions();
		snprintf(Tmp, sizeof(Tmp), "BashDoor(\"%s\")", container->GetScriptName());
		actor->AddAction(GenerateAction(Tmp));
		actor->CommandActor();
		return;
	}

	if (target_mode == TARGET_MODE_PICK) {
		TryToPick(actor, container);
		return;
	}

	container->AddTrigger(TriggerEntry(trigger_clicked, actor->GetGlobalID()));
	actor->ClearPath();
	actor->ClearActions();
	strlcpy(Tmp, "UseContainer()", sizeof(Tmp));
	core->SetCurrentContainer( actor, container);
	actor->AddAction( GenerateAction( Tmp) );
	actor->CommandActor();
}

//generate action code for actor appropriate for the target mode when the target is a door
void GameControl::HandleDoor(Door *door, Actor *actor)
{
	char Tmp[256];

	if ((target_mode == TARGET_MODE_CAST) && spellCount) {
		//we'll get the door back from the coordinates
		Point *p = door->toOpen;
		Point *otherp = door->toOpen+1;
		if (Distance(*p,actor)>Distance(*otherp,actor)) {
			p=otherp;
		}
		TryToCast(actor, *p);
		return;
	}

	core->SetEventFlag(EF_RESETTARGET);

	if (target_mode == TARGET_MODE_ATTACK) {
		actor->ClearPath();
		actor->ClearActions();
		snprintf(Tmp, sizeof(Tmp), "BashDoor(\"%s\")", door->GetScriptName());
		actor->AddAction(GenerateAction(Tmp));
		actor->CommandActor();
		return;
	}

	if (target_mode == TARGET_MODE_PICK) {
		TryToPick(actor, door);
		return;
	}

	door->AddTrigger(TriggerEntry(trigger_clicked, actor->GetGlobalID()));
	actor->ClearPath();
	actor->ClearActions();
	actor->TargetDoor = door->GetGlobalID();
	// internal gemrb toggle door action hack - should we use UseDoor instead?
	sprintf( Tmp, "NIDSpecial9()" );
	actor->AddAction( GenerateAction( Tmp) );
	actor->CommandActor();
}

//generate action code for actor appropriate for the target mode when the target is an active region (infopoint, trap or travel)
bool GameControl::HandleActiveRegion(InfoPoint *trap, Actor * actor, Point &p)
{
	if ((target_mode == TARGET_MODE_CAST) && spellCount) {
		//we'll get the active region from the coordinates (if needed)
		TryToCast(actor, p);
		//don't bother with this region further
		return true;
	}
	if (target_mode == TARGET_MODE_PICK) {
		TryToDisarm(actor, trap);
		return true;
	}

	switch(trap->Type) {
		case ST_TRAVEL:
			trap->AddTrigger(TriggerEntry(trigger_clicked, actor->GetGlobalID()));
			actor->LastMarked = trap->GetGlobalID();
			//clear the go closer flag
			trap->GetCurrentArea()->LastGoCloser = 0;
			return false;
		case ST_TRIGGER:
			// always display overhead text; totsc's ar0511 library relies on it
			if (trap->overHeadText) {
				if (trap->textDisplaying != 1) {
					trap->textDisplaying = 1;
					trap->timeStartDisplaying = core->GetGame()->Ticks;
					DisplayString( trap );
				}
			}
			//the importer shouldn't load the script
			//if it is unallowed anyway (though
			//deactivated scripts could be reactivated)
			//only the 'trapped' flag should be honoured
			//there. Here we have to check on the
			//reset trap and deactivated flags
			if (trap->Scripts[0]) {
				if (!(trap->Flags&TRAP_DEACTIVATED) ) {
					trap->AddTrigger(TriggerEntry(trigger_clicked, actor->GetGlobalID()));
					actor->LastMarked = trap->GetGlobalID();
					//directly feeding the event, even if there are actions in the queue
					//trap->Scripts[0]->Update();
					// FIXME
					trap->ExecuteScript(1);
					trap->ProcessActions();
				}
			}
			if (trap->GetUsePoint() ) {
				char Tmp[256];
				sprintf(Tmp, "TriggerWalkTo(\"%s\")", trap->GetScriptName());
				actor->AddAction(GenerateAction(Tmp));
				actor->CommandActor();
				return true;
			}
			return true;
		default:;
	}
	return false;
}
/** Mouse Button Down */
void GameControl::OnMouseDown(unsigned short x, unsigned short y, unsigned short Button,
	unsigned short Mod)
{
	if (ScreenFlags&SF_DISABLEMOUSE)
		return;

	short px=x;
	short py=y;

	core->GetVideoDriver()->ConvertToGame( px, py );
	FormationRotation = false;

	DoubleClick = false;
	switch(Button)
	{
	case GEM_MB_SCRLUP:
		OnSpecialKeyPress(GEM_UP);
		break;
	case GEM_MB_SCRLDOWN:
		OnSpecialKeyPress(GEM_DOWN);
		break;
	case GEM_MB_MENU: //right click.
		if (core->HasFeature(GF_HAS_FLOAT_MENU) && !Mod) {
			core->GetGUIScriptEngine()->RunFunction( "GUICommon", "OpenFloatMenuWindow", false, Point (x, y));
		}
		else if (target_mode == TARGET_MODE_NONE) {
			ClearMouseState();
			if (core->GetGame()->selected.size() > 1) {
				FormationRotation = true;
				FormationPivotPoint.x = px;
				FormationPivotPoint.y = py;
			}
		}
		break;
	case GEM_MB_ACTION|GEM_MB_DOUBLECLICK:
		DoubleClick = true;
	case GEM_MB_ACTION:
		MouseIsDown = true;
		SelectionRect.x = px;
		SelectionRect.y = py;
		StartX = px;
		StartY = py;
		SelectionRect.w = 0;
		SelectionRect.h = 0;
		if (touchScrollAreasEnabled) {
			touched=true;
		}
	}
}

/** Mouse Button Up */
void GameControl::OnMouseUp(unsigned short x, unsigned short y, unsigned short Button,
	unsigned short Mod)
{
	unsigned int i;
	char Tmp[256];

	if (ScreenFlags & SF_DISABLEMOUSE) {
		return;
	}
	//heh, i found no better place
	core->CloseCurrentContainer();

	MouseIsDown = false;
	Point p(x,y);
	core->GetVideoDriver()->ConvertToGame( p.x, p.y );
	Game* game = core->GetGame();
	if (!game) return;
	Map* area = game->GetCurrentArea( );
	if (!area) return;

	if (touchScrollAreasEnabled) {
		touched=false;
		if (scrolling) {
			SetScrolling(false);
			if (DrawSelectionRect) {
				Actor** ab;
				unsigned int count = area->GetActorInRect( ab, SelectionRect,true );
				if (count != 0) {
					for (i = 0; i < highlighted.size(); i++)
						highlighted[i]->SetOver( false );
					highlighted.clear();
					game->SelectActor( NULL, false, SELECT_NORMAL );
					for (i = 0; i < count; i++) {
						// FIXME: should call handler only once
						game->SelectActor( ab[i], true, SELECT_NORMAL );
					}
				}
				free( ab );
				DrawSelectionRect = false;
			}
			return;
		}
	}

	if (DrawSelectionRect) {
		Actor** ab;
		unsigned int count = area->GetActorInRect( ab, SelectionRect,true );
		if (count != 0) {
			for (i = 0; i < highlighted.size(); i++)
				highlighted[i]->SetOver( false );
			highlighted.clear();
			game->SelectActor( NULL, false, SELECT_NORMAL );
			for (i = 0; i < count; i++) {
				// FIXME: should call handler only once
				game->SelectActor( ab[i], true, SELECT_NORMAL );
			}
		}
		free( ab );
		DrawSelectionRect = false;
		return;
	}

	//hidden actors are not selectable by clicking on them unless they're party members
	Actor* actor = NULL;
	if (!FormationRotation) {
		actor = area->GetActor(p, target_types & ~GA_NO_HIDDEN);
		if (actor && actor->Modified[IE_EA]>=EA_CONTROLLED) {
			if (!actor->ValidTarget(GA_NO_HIDDEN)) {
				actor = NULL;
			}
		}
	}
	if (Button == GEM_MB_MENU && (!core->HasFeature(GF_HAS_FLOAT_MENU) || Mod)) {
		if (actor) {
			//play select sound on right click on actor
			actor->PlaySelectionSound();
			return;
		}
		// reset the action bar
		core->GetGUIScriptEngine()->RunFunction("GUICommonWindows", "EmptyControls");
		core->SetEventFlag(EF_ACTION);
		if (!FormationRotation) {
			return;
		}
		FormationRotation = false;
		//refresh the mouse cursor
		core->GetEventMgr()->FakeMouseMove();
	}

	if (Button > GEM_MB_MENU) return;

	if (!game->selected.size()) {
		//TODO: this is a hack, we need some restructuring here
		//handling the special case when no one was selected, and
		//the player clicks on a partymember
		if (actor && (actor->GetStat(IE_EA)<EA_CHARMED)) {
			if (target_mode==TARGET_MODE_NONE) {
				PerformActionOn(actor);
			}
		}
		return;
	}

	Actor *pc = core->GetFirstSelectedPC(false);
	if (!pc) {
		//this could be a non-PC
		pc = game->selected[0];
	}
	if (!actor) {
		if (Button == GEM_MB_ACTION) {
			//add a check if you don't want some random monster handle doors and such
			if (overDoor) {
				HandleDoor(overDoor, pc);
				return;
			}
			if (overContainer) {
				HandleContainer(overContainer, pc);
				return;
			}
			if (overInfoPoint) {
				if (overInfoPoint->Type==ST_TRAVEL) {
					int i = game->selected.size();
					ieDword exitID = overInfoPoint->GetGlobalID();
					while(i--) {
						game->selected[i]->UseExit(exitID);
					}
				}
				if (HandleActiveRegion(overInfoPoint, pc, p)) {
					core->SetEventFlag(EF_RESETTARGET);
					return;
				}
			}
		}
		//just a single actor, no formation
		if (game->selected.size()==1) {
			//the player is using an item or spell on the ground
			if ((target_mode == TARGET_MODE_CAST) && spellCount) {
				if (target_types & GA_POINT) {
					TryToCast(pc, p);
				}
				return;
			}

			pc->ClearPath();
			pc->ClearActions();
			CreateMovement(pc, p);
			if (DoubleClick) Center(x,y);
			//p is a searchmap travel region
			if ( pc->GetCurrentArea()->GetCursor(p) == IE_CURSOR_TRAVEL) {
				sprintf( Tmp, "NIDSpecial2()" );
				pc->AddAction( GenerateAction( Tmp) );
			}
			return;
		}

		// construct a sorted party
		// TODO: this is still ugly, help?
		std::vector<Actor *> party;
		// first, from the actual party
		int max = game->GetPartySize(false);
		for(int idx = 1; idx<=max; idx++) {
			Actor *act = game->FindPC(idx);
			if(act->IsSelected()) {
				party.push_back(act);
			}
		}
		//summons etc
		for (i = 0; i < game->selected.size(); i++) {
			Actor *act = game->selected[i];
			if (!act->InParty) {
				party.push_back(act);
			}
		}

		//party formation movement
		Point src;
		//p = FormationPivotPoint;
		if (Button == GEM_MB_MENU) {
			p = FormationPivotPoint;
			src = FormationApplicationPoint;
		} else {
			src = party[0]->Pos;
		}
		Point move;

		for(i = 0; i < party.size(); i++) {
			actor = party[i];
			actor->ClearPath();
			actor->ClearActions();

			Map* map = actor->GetCurrentArea();
			move = GetFormationPoint(map, i, src, p);
			CreateMovement(actor, move);
		}
		if (DoubleClick) Center(x,y);

		//p is a searchmap travel region
		if ( party[0]->GetCurrentArea()->GetCursor(p) == IE_CURSOR_TRAVEL) {
			sprintf( Tmp, "NIDSpecial2()" );
			party[0]->AddAction( GenerateAction( Tmp) );
		}
		return;
	}
	if (!actor) return;

	//we got an actor past this point
	if (target_mode == TARGET_MODE_NONE) {
		//play select sound
		actor->PlaySelectionSound();
	}

	PerformActionOn(actor);
}

void GameControl::OnMouseWheelScroll(short x, short y)
{
	Region Viewport = core->GetVideoDriver()->GetViewport();
	Game* game = core->GetGame();
	Map *map = game->GetCurrentArea();
	if (!map) return;
	
	Point mapsize = map->TMap->GetMapSize();
	
	Viewport.x += x;
	if (Viewport.x <= 0) {
		Viewport.x = 0;
	} else if (Viewport.x + Viewport.w >= mapsize.x) {
		Viewport.x = mapsize.x - Viewport.w - 1;
	}

	Viewport.y += y;
	if (Viewport.y <= 0) {
		Viewport.y = 0;
	} else if (Viewport.y + Viewport.h >= mapsize.y) {
		Viewport.y = mapsize.y - Viewport.h - 1;
	}

	if (ScreenFlags & SF_LOCKSCROLL) {
		moveX = 0;
		moveY = 0;
	} else {
		// override any existing viewport moves which may be in progress
		core->timer->SetMoveViewPort( Viewport.x, Viewport.y, 0, false );
		// move it directly ourselves, since we might be paused
		core->GetVideoDriver()->MoveViewportTo( Viewport.x, Viewport.y );
	}
	//update cursor
	core->GetEventMgr()->FakeMouseMove();
}

void GameControl::PerformActionOn(Actor *actor)
{
	Game* game = core->GetGame();
	unsigned int i;

	//determining the type of the clicked actor
	ieDword type;

	type = actor->GetStat(IE_EA);
	if ( type >= EA_EVILCUTOFF || type == EA_GOODBUTRED ) {
		type = ACT_ATTACK; //hostile
	} else if ( type > EA_CHARMED ) {
		type = ACT_TALK; //neutral
	} else {
		type = ACT_NONE; //party
	}

	if (target_mode == TARGET_MODE_ATTACK) {
		type = ACT_ATTACK;
	} else if (target_mode == TARGET_MODE_TALK) {
		type = ACT_TALK;
	} else if (target_mode == TARGET_MODE_CAST) {
		type = ACT_CAST;
	} else if (target_mode == TARGET_MODE_DEFEND) {
		type = ACT_DEFEND;
	} else if (target_mode == TARGET_MODE_PICK) {
		type = ACT_THIEVING;
	}

	if (type != ACT_NONE) {
		if(!actor->ValidTarget(target_types)) {
			return;
		}
	}

	//we shouldn't zero this for two reasons in case of spell or item
	//1. there could be multiple targets
	//2. the target mode is important
	if (!(target_mode == TARGET_MODE_CAST) || !spellCount) {
		ResetTargetMode();
	}

	switch (type) {
		case ACT_NONE: //none
			if (!actor->ValidTarget(GA_SELECT)) {
				return;
			}

			if (actor->InParty)
				SelectActor( actor->InParty );
			else if (actor->GetStat(IE_EA) <= EA_CHARMED) {
				/*let's select charmed/summoned creatures
				EA_CHARMED is the maximum value known atm*/
				core->GetGame()->SelectActor(actor, true, SELECT_REPLACE);
			}
			break;
		case ACT_TALK:
			if (!actor->ValidTarget(GA_TALK)) {
				return;
			}

			//talk (first selected talks)
			if (game->selected.size()) {
				//if we are in PST modify this to NO!
				Actor *source;
				if (core->HasFeature(GF_PROTAGONIST_TALKS) ) {
					source = game->GetPC(0, false); //protagonist
				} else {
					source = core->GetFirstSelectedPC(false);
				}
				// only party members can start conversations
				if (source) {
					TryToTalk(source, actor);
				}
			}
			break;
		case ACT_ATTACK:
			//all of them attacks the red circled actor
			for(i=0;i<game->selected.size();i++) {
				TryToAttack(game->selected[i], actor);
			}
			break;
		case ACT_CAST: //cast on target or use item on target
			if (game->selected.size()==1) {
				Actor *source;
				source = core->GetFirstSelectedActor();
				if(source) {
					TryToCast(source, actor);
				}
			}
			break;
		case ACT_DEFEND:
			for(i=0;i<game->selected.size();i++) {
				TryToDefend(game->selected[i], actor);
			}
			break;
		case ACT_THIEVING:
			if (game->selected.size()==1) {
				Actor *source;
				source = core->GetFirstSelectedActor();
				if(source) {
					TryToPick(source, actor);
				}
			}
			break;
	}
}

//sets target mode, and resets the cursor
void GameControl::SetTargetMode(int mode) {
	target_mode = mode;
	//refresh the mouse cursor
	core->GetEventMgr()->FakeMouseMove();
}

void GameControl::ResetTargetMode() {
	target_types = GA_NO_DEAD|GA_NO_HIDDEN|GA_NO_UNSCHEDULED;
	SetTargetMode(TARGET_MODE_NONE);
}

void GameControl::UpdateTargetMode() {
	SetTargetMode(target_mode);
}

/** Special Key Press */
bool GameControl::OnSpecialKeyPress(unsigned char Key)
{
	if (DialogueFlags&DF_IN_DIALOG) {
		switch(Key) {
			case GEM_RETURN:
				//simulating the continue/end button pressed
				core->GetGUIScriptEngine()->RunFunction("GUIWORLD", "CloseContinueWindow");
				break;
		}
		return false; //don't accept keys in dialog
	}
	Game *game = core->GetGame();
	if (!game) return false;
	int partysize = game->GetPartySize(false);
	
	int pm;
	ieDword keyScrollSpd = 64;
	core->GetDictionary()->Lookup("Keyboard Scroll Speed", keyScrollSpd);
	switch (Key) {
		case GEM_LEFT:
			OnMouseWheelScroll(keyScrollSpd * -1, 0);
			break;
		case GEM_UP:
			OnMouseWheelScroll(0, keyScrollSpd * -1);
			break;
		case GEM_DOWN:
			OnMouseWheelScroll(0, keyScrollSpd);
			break;
		case GEM_RIGHT:
			OnMouseWheelScroll(keyScrollSpd, 0);
			break;
		case GEM_ALT:
			DebugFlags |= DEBUG_SHOW_CONTAINERS;
			break;
		case GEM_TAB:
			// show partymember hp/maxhp as overhead text
			for (pm=0; pm < partysize; pm++) {
				Actor *pc = game->GetPC(pm, true);
				if (!pc) continue;
				pc->DisplayHeadHPRatio();
			}
			break;
		case GEM_MOUSEOUT:
			moveX = 0;
			moveY = 0;
			break;
		case GEM_ESCAPE:
			core->GetGUIScriptEngine()->RunFunction("GUICommonWindows", "EmptyControls");
			core->SetEventFlag(EF_ACTION|EF_RESETTARGET);
			break;
		case GEM_PGUP:
			core->GetGUIScriptEngine()->RunFunction("CommonWindow","OnIncreaseSize");
			break;
		case GEM_PGDOWN:
			core->GetGUIScriptEngine()->RunFunction("CommonWindow","OnDecreaseSize");
			break;
		default:
			return false;
	}
	return true;
}

void GameControl::CalculateSelection(const Point &p)
{
	unsigned int i;

	Game* game = core->GetGame();
	Map* area = game->GetCurrentArea( );
	if (DrawSelectionRect) {
		if (p.x < StartX) {
			SelectionRect.w = StartX - p.x;
			SelectionRect.x = p.x;
		} else {
			SelectionRect.x = StartX;
			SelectionRect.w = p.x - StartX;
		}
		if (p.y < StartY) {
			SelectionRect.h = StartY - p.y;
			SelectionRect.y = p.y;
		} else {
			SelectionRect.y = StartY;
			SelectionRect.h = p.y - StartY;
		}
		Actor** ab;
		unsigned int count = area->GetActorInRect( ab, SelectionRect,true );
		for (i = 0; i < highlighted.size(); i++)
			highlighted[i]->SetOver( false );
		highlighted.clear();
		if (count != 0) {
			for (i = 0; i < count; i++) {
				ab[i]->SetOver( true );
				highlighted.push_back( ab[i] );
			}
		}
		free( ab );
	} else {
		Actor* actor = area->GetActor( p, GA_DEFAULT | GA_SELECT | GA_NO_DEAD | GA_NO_ENEMY);
		SetLastActor( actor, area->GetActorByGlobalID(lastActorID) );
/*
		Actor *lastActor = area->GetActorByGlobalID(lastActorID);
		if (lastActor)
			lastActor->SetOver( false );
		if (!actor) {
			lastActorID = 0;
		} else {
			lastActorID = actor->globalID;
			actor->SetOver( true );
		}
*/
	}
}

void GameControl::SetLastActor(Actor *actor, Actor *prevActor)
{
	if (prevActor)
		prevActor->SetOver( false );
	if (!actor) {
		lastActorID = 0;
	} else {
		lastActorID = actor->GetGlobalID();
		actor->SetOver( true );
	}
}

void GameControl::SetCutSceneMode(bool active)
{
	if (active) {
		ScreenFlags |= (SF_DISABLEMOUSE | SF_LOCKSCROLL | SF_CUTSCENE);
		moveX = 0;
		moveY = 0;
	} else {
		ScreenFlags &= ~(SF_DISABLEMOUSE | SF_LOCKSCROLL | SF_CUTSCENE);
	}
}

//Change game window geometries when a new window gets deactivated
void GameControl::HandleWindowHide(const char *WindowName, const char *WindowPosition)
{
	Variables* dict = core->GetDictionary();
	ieDword index;

	if (dict->Lookup( WindowName, index )) {
		if (index != (ieDword) -1) {
			Window* w = core->GetWindow( (unsigned short) index );
			if (w) {
				core->SetVisible( (unsigned short) index, WINDOW_INVISIBLE );
				if (dict->Lookup( WindowPosition, index )) {
					ResizeDel( w, index );
				}
				return;
			}
			Log(ERROR, "GameControl", "Invalid Window Index: %s:%u",
				WindowName, index);
		}
	}
}

//Hide all other windows on the GUI (gamecontrol is not hidden by this)
int GameControl::HideGUI()
{
	//hidegui is in effect
	if (!(ScreenFlags&SF_GUIENABLED) ) {
		return 0;
	}
	//no gamecontrol visible
	if (Owner->Visible == WINDOW_INVISIBLE ) {
		return 0;
	}
	ScreenFlags &=~SF_GUIENABLED;
	HandleWindowHide("PortraitWindow", "PortraitPosition");
	HandleWindowHide("OtherWindow", "OtherPosition");
	HandleWindowHide("TopWindow", "TopPosition");
	HandleWindowHide("OptionsWindow", "OptionsPosition");
	HandleWindowHide("MessageWindow", "MessagePosition");
	HandleWindowHide("ActionsWindow", "ActionsPosition");
	//FloatWindow doesn't affect gamecontrol, so it is special
	Variables* dict = core->GetDictionary();
	ieDword index;

	if (dict->Lookup( "FloatWindow", index )) {
		if (index != (ieDword) -1) {
			core->SetVisible( (unsigned short) index, WINDOW_INVISIBLE );
		}
	}
	core->GetVideoDriver()->SetViewport( Owner->XPos, Owner->YPos, Width, Height );
	return 1;
}

//Change game window geometries when a new window gets activated
void GameControl::HandleWindowReveal(const char *WindowName, const char *WindowPosition)
{
	Variables* dict = core->GetDictionary();
	ieDword index;

	if (dict->Lookup( WindowName, index )) {
		if (index != (ieDword) -1) {
			Window* w = core->GetWindow( (unsigned short) index );
			if (w) {
				core->SetVisible( (unsigned short) index, WINDOW_VISIBLE );
				if (dict->Lookup( WindowPosition, index )) {
					ResizeAdd( w, index );
				}
				return;
			}
			Log(ERROR, "GameControl", "Invalid Window Index %s:%u",
				WindowName, index);
		}
	}
}

//Reveal all windows on the GUI (including this one)
int GameControl::UnhideGUI()
{
	if (ScreenFlags&SF_GUIENABLED) {
		return 0;
	}

	ScreenFlags |= SF_GUIENABLED;
	// Unhide the gamecontrol window
	core->SetVisible( 0, WINDOW_VISIBLE );

	HandleWindowReveal("ActionsWindow", "ActionsPosition");
	HandleWindowReveal("MessageWindow", "MessagePosition");
	HandleWindowReveal("OptionsWindow", "OptionsPosition");
	HandleWindowReveal("TopWindow", "TopPosition");
	HandleWindowReveal("OtherWindow", "OtherPosition");
	HandleWindowReveal("PortraitWindow", "PortraitPosition");
	//the floatwindow is a special case
	Variables* dict = core->GetDictionary();
	ieDword index;

	if (dict->Lookup( "FloatWindow", index )) {
		if (index != (ieDword) -1) {
			Window* fw = core->GetWindow( (unsigned short) index );
			if (fw) {
				core->SetVisible( (unsigned short) index, WINDOW_VISIBLE );
				fw->Flags |=WF_FLOAT;
				core->SetOnTop( index );
			}
		}
	}
	core->GetVideoDriver()->SetViewport( Owner->XPos, Owner->YPos, Width, Height );
	return 1;
}

//a window got removed, so the GameControl gets enlarged
void GameControl::ResizeDel(Window* win, int type)
{
	switch (type) {
	case 0: //Left
		if (LeftCount!=1) {
			Log(ERROR, "GameControl", "More than one left window!");
		}
		LeftCount--;
		if (!LeftCount) {
			Owner->XPos -= win->Width;
			Owner->Width += win->Width;
			Width = Owner->Width;
		}
		break;

	case 1: //Bottom
		if (BottomCount!=1) {
			Log(ERROR, "GameControl", "More than one bottom window!");
		}
		BottomCount--;
		if (!BottomCount) {
			Owner->Height += win->Height;
			Height = Owner->Height;
		}
		break;

	case 2: //Right
		if (RightCount!=1) {
			Log(ERROR, "GameControl", "More than one right window!");
		}
		RightCount--;
		if (!RightCount) {
			Owner->Width += win->Width;
			Width = Owner->Width;
		}
		break;

	case 3: //Top
		if (TopCount!=1) {
			Log(ERROR, "GameControl", "More than one top window!");
		}
		TopCount--;
		if (!TopCount) {
			Owner->YPos -= win->Height;
			Owner->Height += win->Height;
			Height = Owner->Height;
		}
		break;

	case 4: //BottomAdded
		BottomCount--;
		Owner->Height += win->Height;
		Height = Owner->Height;
		break;
	case 5: //Inactivating
		BottomCount--;
		Owner->Height += win->Height;
		Height = Owner->Height;
		break;
	}
}

//a window got added, so the GameControl gets shrunk
//Owner is the GameControl's window
//GameControl is the only control on that window
void GameControl::ResizeAdd(Window* win, int type)
{
	switch (type) {
	case 0: //Left
		LeftCount++;
		if (LeftCount == 1) {
			Owner->XPos += win->Width;
			Owner->Width -= win->Width;
			Width = Owner->Width;
		}
		break;

	case 1: //Bottom
		BottomCount++;
		if (BottomCount == 1) {
			Owner->Height -= win->Height;
			Height = Owner->Height;
		}
		break;

	case 2: //Right
		RightCount++;
		if (RightCount == 1) {
			Owner->Width -= win->Width;
			Width = Owner->Width;
		}
		break;

	case 3: //Top
		TopCount++;
		if (TopCount == 1) {
			Owner->YPos += win->Height;
			Owner->Height -= win->Height;
			Height = Owner->Height;
		}
		break;

	case 4: //BottomAdded
		BottomCount++;
		Owner->Height -= win->Height;
		Height = Owner->Height;
		break;

	case 5: //Inactivating
		BottomCount++;
		Owner->Height -= win->Height;
		Height = 0;
	}
}

//Create an overhead text over an arbitrary point
// UNUSED
void GameControl::DisplayString(const Point &p, const char *Text)
{
	Scriptable* scr = new Scriptable( ST_TRIGGER );
	scr->overHeadText = (char *) Text;
	scr->textDisplaying = 1;
	scr->timeStartDisplaying = 0;
	scr->Pos = p;
}

//Create an overhead text over a scriptable target
//Multiple texts are possible, as this code copies the text to a new object
void GameControl::DisplayString(Scriptable* target)
{
	Scriptable* scr = new Scriptable( ST_TRIGGER );
	scr->overHeadText = strdup( target->overHeadText );
/* strdup should work here, we use it elsewhere
	size_t len = strlen( target->overHeadText ) + 1;
	scr->overHeadText = ( char * ) malloc( len );
	strcpy( scr->overHeadText, target->overHeadText );
*/
	scr->textDisplaying = 1;
	scr->timeStartDisplaying = target->timeStartDisplaying;
	scr->Pos = target->Pos;

	// add as a "subtitle" to the main message window
	ieDword tmp = 0;
	core->GetDictionary()->Lookup("Duplicate Floating Text", tmp);
	if (tmp) {
		// pass NULL target so pst does not display multiple
		displaymsg->DisplayString(target->overHeadText, NULL);
	}
}

/** changes displayed map to the currently selected PC */
void GameControl::ChangeMap(Actor *pc, bool forced)
{
	//swap in the area of the actor
	Game* game = core->GetGame();
	if (forced || (pc && stricmp( pc->Area, game->CurrentArea) != 0) ) {
		dialoghandler->EndDialog();
		overInfoPoint = NULL;
		overContainer = NULL;
		overDoor = NULL;
		/*this is loadmap, because we need the index, not the pointer*/
		char *areaname = game->CurrentArea;
		if (pc) {
			areaname = pc->Area;
		}
		game->GetMap( areaname, true );
		ScreenFlags|=SF_CENTERONACTOR;
	}
	//center on first selected actor
	Video *video = core->GetVideoDriver();
	Region vp = video->GetViewport();
	if (pc && (ScreenFlags&SF_CENTERONACTOR)) {
		core->timer->SetMoveViewPort( pc->Pos.x, pc->Pos.y, 0, true );
		video->MoveViewportTo( pc->Pos.x-vp.w/2, pc->Pos.y-vp.h/2 );
		ScreenFlags&=~SF_CENTERONACTOR;
	}
}

void GameControl::SetScreenFlags(int value, int mode)
{
	switch(mode) {
		case BM_OR: ScreenFlags|=value; break;
		case BM_NAND: ScreenFlags&=~value; break;
		case BM_SET: ScreenFlags=value; break;
		case BM_AND: ScreenFlags&=value; break;
		case BM_XOR: ScreenFlags^=value; break;
	}
}

void GameControl::SetDialogueFlags(int value, int mode)
{
	switch(mode) {
		case BM_OR: DialogueFlags|=value; break;
		case BM_NAND: DialogueFlags&=~value; break;
		case BM_SET: DialogueFlags=value; break;
		case BM_AND: DialogueFlags&=value; break;
		case BM_XOR: DialogueFlags^=value; break;
	}
}

//copies a screenshot into a sprite
Sprite2D* GameControl::GetScreenshot(bool show_gui)
{
	Sprite2D* screenshot;
	if (show_gui) {
		screenshot = core->GetVideoDriver()->GetScreenshot( Region( 0, 0, 0, 0) );
	} else {
		int hf = HideGUI ();
		Draw (0, 0);
		screenshot = core->GetVideoDriver()->GetScreenshot( Region( 0, 0, 0, 0 ) );
		if (hf) {
			UnhideGUI ();
		}
		core->DrawWindows ();
	}

	return screenshot;
}

//copies a downscaled screenshot into a sprite for save game preview
Sprite2D* GameControl::GetPreview()
{
	// We get preview by first taking a screenshot of quintuple size of the preview control size (a few pixels bigger only in pst),
	// centered in the display. This is to get a decent picture for
	// higher screen resolutions.
	// FIXME: how do orig games solve that?
	Video* video = core->GetVideoDriver();
	int w = video->GetWidth();
	int h = video->GetHeight();
	int x = (w - 640) / 2;
	int y = (h - 405) / 2;

	if (x < 0) {
		x = 0;
	} else {
		w = 515;
	}

	if (y < 0) {
		y = 0;
	} else {
		h = 385;
	}

	if (!x)
		y = 0;

	Draw (0, 0);
	Sprite2D *screenshot = video->GetScreenshot( Region(x, y, w, h) );
	core->DrawWindows();

	Sprite2D* preview = video->SpriteScaleDown ( screenshot, 5 );
	video->FreeSprite( screenshot );
	return preview;
}


/**
 * Returns PC portrait for a currently running game
 */
Sprite2D* GameControl::GetPortraitPreview(int pcslot)
{
	/** Portrait shrink ratio */
	// FIXME: this is just a random PST specific trait
	// you can make it less random with a new feature bit
	int ratio = (core->HasFeature( GF_ONE_BYTE_ANIMID )) ? 1 : 2;

	Video *video = core->GetVideoDriver();

	Actor *actor = core->GetGame()->GetPC( pcslot, false );
	if (! actor) {
		return NULL;
	}
	ResourceHolder<ImageMgr> im(actor->GetPortrait(true));
	if (! im) {
		return NULL;
	}

	Sprite2D* img = im->GetSprite2D();

	if (ratio == 1)
		return img;

	Sprite2D* img_scaled = video->SpriteScaleDown( img, ratio );
	video->FreeSprite( img );

	return img_scaled;
}

Actor *GameControl::GetActorByGlobalID(ieDword globalID)
{
	if (!globalID)
		return NULL;
	Game* game = core->GetGame();
	if (!game)
		return NULL;

	Map* area = game->GetCurrentArea( );
	if (!area)
		return NULL;
	return
		area->GetActorByGlobalID(globalID);
}

Actor *GameControl::GetLastActor()
{
	return GetActorByGlobalID(lastActorID);
}

//Set up an item use which needs targeting
//Slot is an inventory slot
//header is the used item extended header
//u is the user
//target type is a bunch of GetActor flags that alter valid targets
//cnt is the number of different targets (usually 1)
void GameControl::SetupItemUse(int slot, int header, Actor *u, int targettype, int cnt)
{
	memset(spellName, 0, sizeof(ieResRef));
	spellOrItem = -1;
	spellUser = u;
	spellSlot = slot;
	spellIndex = header;
	//item use also uses the casting icon, this might be changed in some custom game?
	SetTargetMode(TARGET_MODE_CAST);
	target_types = targettype;
	spellCount = cnt;
}

//Set up spell casting which needs targeting
//type is the spell's type
//level is the caster level
//idx is the spell's number
//u is the caster
//target type is a bunch of GetActor flags that alter valid targets
//cnt is the number of different targets (usually 1)
void GameControl::SetupCasting(ieResRef spellname, int type, int level, int idx, Actor *u, int targettype, int cnt)
{
	memcpy(spellName, spellname, sizeof(ieResRef));
	spellOrItem = type;
	spellUser = u;
	spellSlot = level;
	spellIndex = idx;
	SetTargetMode(TARGET_MODE_CAST);
	target_types = targettype;
	spellCount = cnt;
}

//another method inherited from Control which has no use here
bool GameControl::SetEvent(int /*eventType*/, EventHandler /*handler*/)
{
	return false;
}

void GameControl::SetDisplayText(char *text, unsigned int time)
{
	if (DisplayText) {
		core->FreeString(DisplayText);
	}
	DisplayTextTime = time;
	DisplayText = text;
}

void GameControl::SetDisplayText(ieStrRef text, unsigned int time)
{
	SetDisplayText(core->GetString(displaymsg->GetStringReference(text), 0), time);
}

void GameControl::ToggleAlwaysRun()
{
	AlwaysRun = !AlwaysRun;
}

}
