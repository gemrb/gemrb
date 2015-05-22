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
#include "Game.h"
#include "GameData.h"
#include "GlobalTimer.h"
#include "ImageMgr.h"
#include "Interface.h"
#include "PathFinder.h"
#include "ScriptEngine.h"
#include "TileMap.h"
#include "Video.h"
#include "damages.h"
#include "ie_cursors.h"
#include "opcode_params.h"
#include "GameScript/GSUtils.h"
#include "GUI/EventMgr.h"
#include "GUI/TextArea.h"
#include "GUI/Window.h"
#include "RNG/RNG_SFMT.h"
#include "Scriptable/Container.h"
#include "Scriptable/Door.h"
#include "Scriptable/InfoPoint.h"

#include <cmath>

namespace GemRB {

#define DEBUG_SHOW_INFOPOINTS   0x01
#define DEBUG_SHOW_CONTAINERS   0x02
#define DEBUG_SHOW_DOORS	DEBUG_SHOW_CONTAINERS
#define DEBUG_SHOW_LIGHTMAP     0x08

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

GameControl::GameControl(const Region& frame)
	: Control(frame),
	windowGroupCounts(),
	ClickPoint()
{
	ControlType = IE_GUI_GAMECONTROL;
	if (!formations) {
		ReadFormations();
	}
	//this is the default action, individual actors should have one too
	//at this moment we use only this
	//maybe we don't even need it
	spellCount = spellIndex = spellOrItem = spellSlot = 0;
	spellUser = NULL;
	spellName[0] = 0;
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
	numScrollCursor = 0;
	DebugFlags = 0;
	AIUpdateCounter = 1;
	AlwaysRun = false; //make this a game flag if you wish
	ieDword tmp=0;

	ClearMouseState();
	ResetTargetMode();

	tmp=0;
	core->GetDictionary()->Lookup("Center",tmp);
	if (tmp) {
		ScreenFlags=SF_ALWAYSCENTER|SF_CENTERONACTOR;
	} else {
		ScreenFlags = SF_CENTERONACTOR;
	}
	DialogueFlags = 0;
	dialoghandler = new DialogHandler();
	DisplayText = NULL;
	DisplayTextTime = 0;
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
Point GameControl::GetFormationPoint(Map *map, unsigned int pos, const Point& src, Point p)
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

void GameControl::Center(const Point& p) const
{
	Video *video = core->GetVideoDriver();
	Region Viewport = video->GetViewport();
	Viewport.x += p.x - Viewport.w / 2;
	Viewport.y += p.y - Viewport.h / 2;
	MoveViewportTo(Viewport.Origin(), false);
}

void GameControl::ClearMouseState()
{
	MouseIsDown = false;
	DrawSelectionRect = false;
	FormationRotation = false;
	DoubleClick = false;
	//refresh the mouse cursor
	core->GetEventMgr()->FakeMouseMove();
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

	actor->CommandActor(action);
}

GameControl::~GameControl(void)
{
	//releasing the viewport of GameControl
	core->GetVideoDriver()->SetViewport( Region() );
	if (formations)	{
		free( formations );
		formations = NULL;
	}
	delete dialoghandler;
	delete DisplayText;
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

	Color color = ColorGreen;
	if (flash) {
		if (step & 2) {
			color = ColorWhite;
		} else {
			if (!actorSelected) color = ColorGreenDark;
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
void GameControl::DrawSelf(Region screen, const Region& /*clip*/)
{
	bool update_scripts = !(DialogueFlags & DF_FREEZE_SCRIPTS);

	Game* game = core->GetGame();
	if (!game)
		return;

	Map *area = core->GetGame()->GetCurrentArea();
	Video* video = core->GetVideoDriver();
	if (!area) {
		video->DrawRect( screen, ColorBlue, true );
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
		MoveViewportTo( viewport.Origin(), false );
	}
	video->DrawRect( screen, ColorBlack, true );

	// setup outlines
	InfoPoint *i;
	unsigned int idx;
	for (idx = 0; (i = area->TMap->GetInfoPoint( idx )); idx++) {
		i->Highlight = false;
		if (overInfoPoint == i && target_mode) {
			if (i->VisibleTrap(0)) {
				i->outlineColor = ColorGreen;
				i->Highlight = true;
				continue;
			}
		}
		if (i->VisibleTrap(DebugFlags & DEBUG_SHOW_INFOPOINTS)) {
			i->outlineColor = ColorRed; // traps
		} else if (DebugFlags & DEBUG_SHOW_INFOPOINTS) {
			i->outlineColor = ColorBlue; // debug infopoints
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
					d->outlineColor = ColorGreen;
					d->Highlight = true;
					continue;
				}
			} else if (!(d->Flags & DOOR_SECRET)) {
				// mouse over, not in target mode, no secret door
				d->outlineColor = ColorCyan;
				d->Highlight = true;
				continue;
			}
		}
		if (d->VisibleTrap(0)) {
			d->outlineColor = ColorRed; // traps
		} else if (d->Flags & DOOR_SECRET) {
			if (DebugFlags & DEBUG_SHOW_DOORS || d->Flags & DOOR_FOUND) {
				d->outlineColor = ColorMagenta; // found hidden door
			} else {
				// secret door is invisible
				continue;
			}
		} else if (DebugFlags & DEBUG_SHOW_DOORS) {
			d->outlineColor = ColorCyan; // debug doors
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
				c->outlineColor = ColorGreen;
				c->Highlight = true;
				continue;
			}
		} else if (overContainer == c) {
			// mouse over, not in target mode
			c->outlineColor = ColorCyan;
			c->Highlight = true;
			continue;
		}
		if (c->VisibleTrap(0)) {
			c->outlineColor = ColorRed; // traps
		} else if (DebugFlags & DEBUG_SHOW_CONTAINERS) {
			c->outlineColor = ColorCyan; // debug containers
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
				DrawArrowMarker(screen, target->Pos, viewport, ColorBlack);
			}
			free(monsters);
		} else {
			trackerID = 0;
		}
	}

	if (lastActorID) {
		Actor* actor = GetLastActor();
		if (actor) {
			DrawArrowMarker(screen, actor->Pos, viewport, ColorGreen);
		}
	}

	if (ScreenFlags & SF_DISABLEMOUSE)
		return;

	// Draw selection rect
	if (DrawSelectionRect) {
		CalculateSelection( gameMousePos );
		video->DrawRect( SelectionRect, ColorGreen, false, true );
	}

	// draw reticles
	if (FormationRotation) {
		Actor *actor;
		int max = game->GetPartySize(false);
		// we only care about PCs and not summons for this. the summons will be included in
		// the final mouse up event.
		int formationPos = 0;
		for(int idx = 1; idx<=max; idx++) {
			actor = game->FindPC(idx);
			if (actor && actor->IsSelected()) {
				// transform the formation point
				Point p = GetFormationPoint(actor->GetCurrentArea(), formationPos++, gameMousePos, ClickPoint);
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
				video->DrawCircle( p.x, p.y, 2, ColorRed );
			} else {
				short oldX = ( node->Parent-> x*16) + 8, oldY = ( node->Parent->y*12 ) + 6;
				video->DrawLine( oldX, oldY, p.x, p.y, ColorGreen );
			}
			if (!node->Next) {
				video->DrawCircle( p.x, p.y, 2, ColorGreen );
				break;
			}
			node = node->Next;
		}
	}

	// Draw lightmap
	if (DebugFlags & DEBUG_SHOW_LIGHTMAP) {
		Sprite2D* spr = area->LightMap->GetSprite2D();
		video->BlitSprite( spr, 0, 0, true );
		Sprite2D::FreeSprite( spr );
		Region point( gameMousePos.x / 16, gameMousePos.y / 12, 2, 2 );
		video->DrawRect( point, ColorRed );
	}

	if (core->HasFeature(GF_ONSCREEN_TEXT) && DisplayText) {
		core->GetTextFont()->Print(screen, *DisplayText, core->InfoTextPalette, IE_FONT_ALIGN_CENTER | IE_FONT_ALIGN_MIDDLE);
		if (update_scripts) {
			// just replicating original engine behaviour
			if (DisplayTextTime == 0) {
				SetDisplayText((String*)NULL, 0);
			} else {
				DisplayTextTime--;
			}
		}
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
		switch (Key) {
			case 'a': //switches through the avatar animations
				if (lastActor) {
					lastActor->GetNextAnimation();
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
				drawPath = core->GetGame()->GetCurrentArea()->FindPath( pfs, gameMousePos, lastActor?lastActor->size:1 );
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
						if (src->LastSpellTarget) {
							src->CastSpellEnd(0, 0);
						} else {
							src->CastSpellPointEnd(0, 0);
						}
					}
				}
				break;
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
			case 'e':// reverses pc order (useful for parties bigger than 6)
				game->ReversePCs();
				break;
			// f
			case 'g'://shows loaded areas and other game information
				game->dump();
				break;
			// h
			case 'i'://interact trigger (from the original game)
				if (!lastActor) {
					lastActor = area->GetActor( gameMousePos, GA_DEFAULT);
				}
				if (lastActor && !(lastActor->GetStat(IE_MC_FLAGS)&MC_EXPORTABLE)) {
					Actor *target;
					int i = game->GetPartySize(true);
					if(i<2) break;
					i=RAND(0, i-1);
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
			case 'j': //teleports the selected actors
				for (i = 0; i < game->selected.size(); i++) {
					Actor* actor = game->selected[i];
					actor->ClearActions();
					MoveBetweenAreasCore(actor, core->GetGame()->CurrentArea, gameMousePos, -1, true);
				}
				break;
			case 'k': //kicks out actor
				if (lastActor && lastActor->InParty) {
					lastActor->Stop();
					lastActor->AddAction( GenerateAction("LeaveParty()") );
				}
				break;
			case 'l': //play an animation (vvc/bam) over an actor
				//the original engine was able to swap through all animations
				if (lastActor) {
					lastActor->AddAnimation("S056ICBL", 0, 0, 0);
				}
				break;
			case 'M':
				if (!lastActor) {
					lastActor = area->GetActor( gameMousePos, GA_DEFAULT);
				}
				if (!lastActor) {
					// ValidTarget never returns immobile targets, making debugging a nightmare
					// so if we don't have an actor, we make really really sure by checking manually
					unsigned int count = area->GetActorCount(true);
					while (count--) {
						Actor *actor = area->GetActor(count, true);
						if (actor->IsOver(gameMousePos)) {
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
					lastActor = area->GetActor( gameMousePos, GA_DEFAULT);
				}
				if (!lastActor) {
					// ValidTarget never returns immobile targets, making debugging a nightmare
					// so if we don't have an actor, we make really really sure by checking manually
					unsigned int count = area->GetActorCount(true);
					while (count--) {
						Actor *actor = area->GetActor(count, true);
						if (actor->IsOver(gameMousePos)) {
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
			case 'n': //prints a list of all the live actors in the area
				core->GetGame()->GetCurrentArea()->dump(true);
				break;
			case 'o': //set up the origin for the pathfinder
				// origin
				pfs = gameMousePos;
				break;
			case 'p': //center on actor
				ScreenFlags|=SF_CENTERONACTOR;
				ScreenFlags^=SF_ALWAYSCENTER;
				break;
			case 'q': //joins actor to the party
				if (lastActor && !lastActor->InParty) {
					lastActor->Stop();
					lastActor->AddAction( GenerateAction("JoinParty()") );
				}
				break;
			case 'r'://resurrects actor
				if (!lastActor) {
					lastActor = area->GetActor( gameMousePos, GA_DEFAULT);
				}
				if (lastActor) {
					Effect *fx = EffectQueue::CreateEffect(heal_ref, lastActor->GetStat(IE_MAXHITPOINTS), 0x30001, FX_DURATION_INSTANT_PERMANENT);
					if (fx) {
						core->ApplyEffect(fx, lastActor, lastActor);
					}
					delete fx;
				}
				break;
			case 's': //switches through the stance animations
				if (lastActor) {
					lastActor->GetNextStance();
				}
				break;
			case 't'://advances time
				// 7200 (one day) /24 (hours) == 300
				game->AdvanceTime(300*AI_UPDATE_TIME);
				//refresh gui here once we got it
				break;
			// u
			case 'V': //
				core->GetDictionary()->DebugDump();
				break;
			case 'v': //marks some of the map visited (random vision distance)
				area->ExploreMapChunk( gameMousePos, RAND(0,29), 1 );
				break;
			case 'w': // consolidates found ground piles under the pointed pc
				area->MoveVisibleGroundPiles(gameMousePos);
				break;
			case 'x': // shows coordinates on the map
				Log(MESSAGE, "GameControl", "Position: %s [%d.%d]", area->GetScriptName(), gameMousePos.x, gameMousePos.y );
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
				// fallthrough
			case 'y': //kills actor
				if (lastActor) {
					//using action so the actor is killed
					//correctly (synchronisation)
					lastActor->Stop();

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

void GameControl::DrawTooltip(const Point& p) const
{
	if (ScreenFlags & SF_DISABLEMOUSE) {
		return;
	}
	Control::DrawTooltip(p);
}

const String& GameControl::TooltipText() const {
	// not bothering checking if game or area is null. If we are somehow entering this method when either is false,
	// then something is horribly broken elsewhere. by definition we cant have a GameControl without these things.
	Map* area = core->GetGame()->GetCurrentArea();
	Actor* actor = area->GetActor(gameMousePos, GA_NO_DEAD|GA_NO_UNSCHEDULED);
	if (actor == NULL) {
		return Control::TooltipText();
	}

	static String tip; // only one game control and we return a const& so cant be temporary.
	const char *name = actor->GetName(-1);
	// FIME: make the actor name a String instead
	String* wname = StringFromCString(name);
	if (wname) {
		tip = *wname;
	}

	int hp = actor->GetStat(IE_HITPOINTS);
	int maxhp = actor->GetStat(IE_MAXHITPOINTS);

	if (actor->InParty) {
		wchar_t hpstring[10];
		swprintf(hpstring, 10, L"%d/%d", hp, maxhp);
		if (/* DISABLES CODE */ (false)) { // FIXME: this should be for PST (how to check?)
			tip += L": ";
		} else {
			tip += L"\n";
		}
		tip += hpstring;
	} else {
		// a guess at a neutral check
		bool enemy = actor->GetStat(IE_EA) != EA_NEUTRAL;
		// test for an injured string being present for this game
		int strindex = displaymsg->GetStringReference(STR_UNINJURED);
		if (enemy && strindex != -1) {
			// non-neutral, not in party: display injured string
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
			String* injuredstring = core->GetString(strindex, 0);
			assert(injuredstring); // we just "checked" for these (by checking for STR_UNINJURED)
			tip += *injuredstring;
			delete injuredstring;
		}
	}

	return tip;
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
			// most secret doors are in walls, so default to the blocked cursor to not give them away
			// iwd ar6010 table/door/puzzle is walkable, secret and undetectable
			Game *game = core->GetGame();
			if (!game) return IE_CURSOR_BLOCKED;
			Map *area = game->GetCurrentArea();
			if (!area) return IE_CURSOR_BLOCKED;
			return area->GetCursor(overDoor->Pos);
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
void GameControl::OnMouseOver(const Point& mp)
{
	if (ScreenFlags & SF_DISABLEMOUSE) {
		return;
	}

	gameMousePos = core->GetVideoDriver()->ConvertToGame(mp);

	if (MouseIsDown && ( !DrawSelectionRect )) {
		if (( abs( gameMousePos.x - ClickPoint.x ) > 5 ) || ( abs( gameMousePos.y - ClickPoint.y ) > 5 )) {
			DrawSelectionRect = true;
		}
	}
	if (FormationRotation) {
		return;
	}
	Game* game = core->GetGame();
	if (!game) return;
	Map* area = game->GetCurrentArea( );
	if (!area) return;
	int nextCursor = area->GetCursor( gameMousePos );
	//make the invisible area really invisible
	if (nextCursor == IE_CURSOR_INVALID) {
		Owner->Cursor = IE_CURSOR_BLOCKED;
		lastCursor = IE_CURSOR_BLOCKED;
		return;
	}

	overInfoPoint = area->TMap->GetInfoPoint( gameMousePos, true );
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

	overDoor = area->TMap->GetDoor( gameMousePos );
	overContainer = area->TMap->GetContainer( gameMousePos );

	if (!DrawSelectionRect) {
		if (overDoor) {
			nextCursor = GetCursorOverDoor(overDoor);
		}

		if (overContainer) {
			nextCursor = GetCursorOverContainer(overContainer);
		}

		// let us target party members even if they are invisible
		lastActor = area->GetActor(gameMousePos, GA_NO_DEAD|GA_NO_UNSCHEDULED);
		if (lastActor && lastActor->Modified[IE_EA]>=EA_CONTROLLED) {
			if (!lastActor->ValidTarget(target_types) || !area->IsVisible(gameMousePos, false)) {
				lastActor = NULL;
			}
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
void GameControl::OnGlobalMouseMove(const Point& p)
{
	if (ScreenFlags & SF_DISABLEMOUSE) {
		return;
	}

	if (Owner->WindowVisibility() != Window::VISIBLE) {
		return;
	}

	int mousescrollspd = core->GetMouseScrollSpeed();

#define SCROLL_AREA_WIDTH 5
	if (p.x <= SCROLL_AREA_WIDTH)
		moveX = -mousescrollspd;
	else {
		if (p.x >= ( core->Width - SCROLL_AREA_WIDTH ))
			moveX = mousescrollspd;
		else
			moveX = 0;
	}
	if (p.y <= SCROLL_AREA_WIDTH)
		moveY = -mousescrollspd;
	else {
		if (p.y >= ( core->Height - 5 ))
			moveY = mousescrollspd;
		else
			moveY = 0;
	}
#undef SCROLL_AREA_WIDTH

	SetScrolling(moveX != 0 || moveY != 0);
}
#endif

void GameControl::MoveViewportTo(Point p, bool center) const
{
	Map *area = core->GetGame()->GetCurrentArea();
	if (!area) return;

	Video *video = core->GetVideoDriver();
	Region vp = video->GetViewport();
	Point mapsize = area->TMap->GetMapSize();

	if (center) {
		p.x -= vp.w/2;
		p.y -= vp.h/2;
	}
	if (p.x < 0) {
		p.x = 0;
	} else if (p.x + vp.w >= mapsize.x) {
		p.x = mapsize.x - vp.w - 1;
	}
	if (p.y < 0) {
		p.y = 0;
	} else if (p.y + vp.h >= mapsize.y) {
		p.y = mapsize.y - vp.h - 1;
	}

	// override any existing viewport moves which may be in progress
	core->timer->SetMoveViewPort(p, 0, false);
	// move it directly ourselves, since we might be paused
	video->MoveViewportTo(p);
}

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
	Sprite2D::FreeSprite(cursor);

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
	source->CommandActor(GenerateActionDirect( "NIDSpecial3()", tgt));
}

//generate action code for source actor to try to defend a target
void GameControl::TryToDefend(Actor *source, Actor *tgt)
{
	source->SetModal(MS_NONE);
	source->CommandActor(GenerateActionDirect( "NIDSpecial4()", tgt));
}

// generate action code for source actor to try to pick pockets of a target (if an actor)
// else if door/container try to pick a lock/disable trap
// The -1 flag is a placeholder for dynamic target IDs
void GameControl::TryToPick(Actor *source, Scriptable *tgt)
{
	source->SetModal(MS_NONE);
	const char* cmdString = NULL;
	switch (tgt->Type) {
		case ST_ACTOR:
			cmdString = "PickPockets([-1])";
			break;
		case ST_DOOR:
		case ST_CONTAINER:
			if (((Highlightable*)tgt)->Trapped && ((Highlightable*)tgt)->TrapDetected) {
				cmdString = "RemoveTraps([-1])";
			} else {
				cmdString = "PickLock([-1])";
			}
			break;
		default:
			Log(ERROR, "GameControl", "Invalid pick target of type %d", tgt->Type);
			return;
	}
	source->CommandActor(GenerateActionDirect(cmdString, tgt));
}

//generate action code for source actor to try to disable trap (only trap type active regions)
void GameControl::TryToDisarm(Actor *source, InfoPoint *tgt)
{
	if (tgt->Type!=ST_PROXIMITY) return;

	source->SetModal(MS_NONE);
	source->CommandActor(GenerateActionDirect( "RemoveTraps([-1])", tgt ));
}

//generate action code for source actor to use item/cast spell on a point
void GameControl::TryToCast(Actor *source, const Point &tgt)
{
	char Tmp[40];

	if (!spellCount) {
		ResetTargetMode();
		return; //not casting or using an own item
	}
	source->Stop();

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
				delete action;
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
	source->Stop();

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
				delete action;
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
	//Nidspecial1 is just an unused action existing in all games
	//(non interactive demo)
	//i found no fitting action which would emulate this kind of
	//dialog initation
	source->SetModal(MS_NONE);
	dialoghandler->targetID = tgt->GetGlobalID(); //this is a hack, but not so deadly
	source->CommandActor(GenerateActionDirect( "NIDSpecial1()", tgt));
}

//generate action code for actor appropriate for the target mode when the target is a container
void GameControl::HandleContainer(Container *container, Actor *actor)
{
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
		char Tmp[256];
		snprintf(Tmp, sizeof(Tmp), "BashDoor(\"%s\")", container->GetScriptName());
		actor->CommandActor(GenerateAction(Tmp));
		return;
	}

	if (target_mode == TARGET_MODE_PICK) {
		TryToPick(actor, container);
		return;
	}

	container->AddTrigger(TriggerEntry(trigger_clicked, actor->GetGlobalID()));
	core->SetCurrentContainer( actor, container);
	actor->CommandActor(GenerateAction("UseContainer()"));
}

//generate action code for actor appropriate for the target mode when the target is a door
void GameControl::HandleDoor(Door *door, Actor *actor)
{
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
		char Tmp[256];
		snprintf(Tmp, sizeof(Tmp), "BashDoor(\"%s\")", door->GetScriptName());
		actor->CommandActor(GenerateAction(Tmp));
		return;
	}

	if (target_mode == TARGET_MODE_PICK) {
		TryToPick(actor, door);
		return;
	}

	door->AddTrigger(TriggerEntry(trigger_clicked, actor->GetGlobalID()));
	actor->TargetDoor = door->GetGlobalID();
	// internal gemrb toggle door action hack - should we use UseDoor instead?
	actor->CommandActor(GenerateAction("NIDSpecial9()"));
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
			if (!trap->GetOverheadText().empty()) {
				if (!trap->OverheadTextIsDisplaying()) {
					trap->DisplayOverheadText(true);
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
				actor->CommandActor(GenerateAction(Tmp));
				return true;
			}
			return true;
		default:;
	}
	return false;
}
/** Mouse Button Down */
void GameControl::OnMouseDown(const Point& p, unsigned short Button, unsigned short Mod)
{
	if (ScreenFlags&SF_DISABLEMOUSE)
		return;

	ClickPoint = p;
	core->GetVideoDriver()->ConvertToGame( ClickPoint );

	ClearMouseState(); // cancel existing mouse action, we dont support multibutton actions
	switch(Button) {
	case GEM_MB_SCRLUP:
		OnSpecialKeyPress(GEM_UP);
		break;
	case GEM_MB_SCRLDOWN:
		OnSpecialKeyPress(GEM_DOWN);
		break;
	case GEM_MB_MENU: //right click.
		if (core->HasFeature(GF_HAS_FLOAT_MENU) && !Mod) {
			core->GetGUIScriptEngine()->RunFunction( "GUICommon", "OpenFloatMenuWindow", false, p);
		} else {
			FormationRotation = true;
		}
		break;
	case GEM_MB_ACTION|GEM_MB_DOUBLECLICK:
		DoubleClick = true;
	case GEM_MB_ACTION:
		// PST uses alt + left click for formation rotation
		// is there any harm in this being true in all games?
		if (Mod&GEM_MOD_ALT) {
			FormationRotation = true;
		} else {
			MouseIsDown = true;
			SelectionRect.x = ClickPoint.x;
			SelectionRect.y = ClickPoint.y;
			SelectionRect.w = 0;
			SelectionRect.h = 0;
		}
		break;
	}
	if (core->GetGame()->selected.size() <= 1
		|| target_mode != TARGET_MODE_NONE) {
		FormationRotation = false;
	}
	if (FormationRotation) {
		lastCursor = IE_CURSOR_USE;
		Owner->Cursor = lastCursor;
	}
}

/** Mouse Button Up */
void GameControl::OnMouseUp(const Point& mp, unsigned short Button, unsigned short /*Mod*/)
{
	MouseIsDown = false;
	if (ScreenFlags & SF_DISABLEMOUSE) {
		return;
	}
	//heh, i found no better place
	core->CloseCurrentContainer();

	Point p = core->GetVideoDriver()->ConvertToGame( mp );
	Game* game = core->GetGame();
	if (!game) return;
	Map* area = game->GetCurrentArea( );
	if (!area) return;

	unsigned int i = 0;
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

	Actor* actor = NULL;
	bool doMove = FormationRotation;
	if (!FormationRotation) {
		//hidden actors are not selectable by clicking on them unless they're party members
		actor = area->GetActor(p, target_types & ~GA_NO_HIDDEN);
		if (actor && actor->Modified[IE_EA]>=EA_CONTROLLED) {
			if (!actor->ValidTarget(GA_NO_HIDDEN)) {
				actor = NULL;
			}
		}
		switch (Button) {
			case GEM_MB_ACTION:
				if (!actor) {
					Actor *pc = core->GetFirstSelectedPC(false);
					if (!pc) {
						//this could be a non-PC
						pc = game->selected[0];
					}
					//add a check if you don't want some random monster handle doors and such
					if (overDoor) {
						HandleDoor(overDoor, pc);
						break;
					}
					if (overContainer) {
						HandleContainer(overContainer, pc);
						break;
					}
					if (overInfoPoint) {
						if (overInfoPoint->Type==ST_TRAVEL) {
							size_t i = game->selected.size();
							ieDword exitID = overInfoPoint->GetGlobalID();
							while(i--) {
								game->selected[i]->UseExit(exitID);
							}
						}
						if (HandleActiveRegion(overInfoPoint, pc, p)) {
							core->SetEventFlag(EF_RESETTARGET);
							break;
						}
					}
					//just a single actor, no formation
					if (game->selected.size()==1
						&& target_mode == TARGET_MODE_CAST
						&& spellCount
						&& (target_types&GA_POINT)) {
						//the player is using an item or spell on the ground
						TryToCast(pc, p);
						break;
					}
				}
				doMove = (!actor && target_mode == TARGET_MODE_NONE);
				break;
			case GEM_MB_MENU:
				// we used to check mod in this case,
				// but it doesnt make sense to initiate an action based on a mod on mouse down
				// then cancel that action because the mod disapeared before mouse up
				if (!core->HasFeature(GF_HAS_FLOAT_MENU)) {
					SetTargetMode(TARGET_MODE_NONE);
				}
				if (!actor) {
					// reset the action bar
					core->GetGUIScriptEngine()->RunFunction("GUICommonWindows", "EmptyControls");
					core->SetEventFlag(EF_ACTION);
				}
				break;
			default:
				return; // we dont handle any other buttons beyond this point
		}
	}

	if (doMove && game->selected.size() > 0) {
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
		if (FormationRotation) {
			p = ClickPoint;
			src = gameMousePos;
		} else {
			src = party[0]->Pos;
		}
		Point move = p;

		for(i = 0; i < party.size(); i++) {
			actor = party[i];
			actor->Stop();

			if (i || party.size() > 1) {
				Map* map = actor->GetCurrentArea();
				move = GetFormationPoint(map, i, src, p);
			}
			CreateMovement(actor, move);
		}
		if (DoubleClick) Center(mp);

		//p is a searchmap travel region
		if ( party[0]->GetCurrentArea()->GetCursor(p) == IE_CURSOR_TRAVEL) {
			char Tmp[256];
			sprintf( Tmp, "NIDSpecial2()" );
			party[0]->AddAction( GenerateAction( Tmp) );
		}
	} else if (actor) {
		if (actor->GetStat(IE_EA)<EA_CHARMED
			&& target_mode == TARGET_MODE_NONE) {
			// we are selecting a party member
			actor->PlaySelectionSound();
		}

		PerformActionOn(actor);
	}
	FormationRotation = false;
	core->GetEventMgr()->FakeMouseMove();
}

void GameControl::OnMouseWheelScroll(short x, short y)
{
	Region Viewport = core->GetVideoDriver()->GetViewport();
	Viewport.x += x;
	Viewport.y += y;

	if (ScreenFlags & SF_LOCKSCROLL) {
		moveX = 0;
		moveY = 0;
	} else {
		MoveViewportTo( Viewport.Origin(), false);
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
		// don't accept keys in dialog
		// dont forward the even either
		return false;
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
			return Control::OnSpecialKeyPress(Key);
	}
	return true;
}

void GameControl::CalculateSelection(const Point &p)
{
	unsigned int i;

	Game* game = core->GetGame();
	Map* area = game->GetCurrentArea( );
	if (DrawSelectionRect) {
		if (p.x < ClickPoint.x) {
			SelectionRect.w = ClickPoint.x - p.x;
			SelectionRect.x = p.x;
		} else {
			SelectionRect.x = ClickPoint.x;
			SelectionRect.w = p.x - ClickPoint.x;
		}
		if (p.y < ClickPoint.y) {
			SelectionRect.h = ClickPoint.y - p.y;
			SelectionRect.y = p.y;
		} else {
			SelectionRect.y = ClickPoint.y;
			SelectionRect.h = p.y - ClickPoint.y;
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

//Hide or unhide all other windows on the GUI (gamecontrol is not hidden by this)
bool GameControl::SetGUIHidden(bool hide)
{
	if (hide) {
		//no gamecontrol visible
		if (!(ScreenFlags&SF_GUIENABLED)
			|| Owner->WindowVisibility() == Window::INVISIBLE ) {
			return false;
		}
		ScreenFlags &=~SF_GUIENABLED;
	} else {
		if (ScreenFlags&SF_GUIENABLED) {
			return false;
		}
		ScreenFlags |= SF_GUIENABLED;
		// Unhide the gamecontrol window
		core->UnhideGCWindow();
	}

	static const char* keys[6][2] = {
		{"PortraitWindow", "PortraitPosition"},
		{"OtherWindow", "OtherPosition"},
		{"TopWindow", "TopPosition"},
		{"OptionsWindow", "OptionsPosition"},
		{"MessageWindow", "MessagePosition"},
		{"ActionsWindow", "ActionsPosition"},
	};

	Variables* dict = core->GetDictionary();
	ieDword index;

	// iterate the list forwards for hiding, and in reverse for unhiding
	int i = hide ? 0 : 5;
	int inc = hide ? 1 : -1;
	WINDOW_RESIZE_OPERATION op = hide ? WINDOW_EXPAND : WINDOW_CONTRACT;
	for (;i >= 0 && i <= 5; i+=inc) {
		const char** val = keys[i];
		//Log(MESSAGE, "GameControl", "window: %s", *val);
		if (dict->Lookup( *val, index )) {
			if (index != (ieDword) -1) {
				Window* w = core->GetWindow(index);
				w->SetFlags(WF_BORDERLESS, BM_OR);
				if (w) {
					w->SetVisibility((Window::Visibility)!hide);
					if (dict->Lookup( *++val, index )) {
						//Log(MESSAGE, "GameControl", "position: %s", *val);
						ResizeParentWindowFor( w, index, op );
						continue;
					}
				}
				Log(ERROR, "GameControl", "Invalid window or position: %s:%u",
					*val, index);
			}
		}
	}

	//FloatWindow doesn't affect gamecontrol, so it is special
	if (dict->Lookup("FloatWindow", index)) {
		if (index != (ieDword) -1) {
			Window* fw = core->GetWindow(index);
			fw->SetVisibility((Window::Visibility)!hide);
			if (!hide) {
				assert(fw != NULL);
				fw->SetFlags(WF_FLOAT|WF_BORDERLESS, BM_OR);
				core->SetOnTop(fw);
			}
		}
	}
	core->GetVideoDriver()->SetViewport( Region(Owner->Frame().Origin(), frame.Dimensions()) );
	return true;
}

void GameControl::ResizeParentWindowFor(Window* win, int type, WINDOW_RESIZE_OPERATION op)
{
	// when GameControl contracts it adds to windowGroupCounts
	// WINDOW_CONTRACT is a positive operation and WINDOW_EXPAND is negative
	const Region& winFrame = win->Frame();
	Region ownerFrame = Owner->Frame();
	if (type < WINDOW_GROUP_COUNT) {
		windowGroupCounts[type] += op;
		if ((op == WINDOW_CONTRACT && windowGroupCounts[type] == 1)
			|| (op == WINDOW_EXPAND && !windowGroupCounts[type])) {

			switch (type) {
				case WINDOW_GROUP_LEFT:
					ownerFrame.x += winFrame.w * op;
					// fallthrough
				case WINDOW_GROUP_RIGHT:
					ownerFrame.w -= winFrame.w * op;
					break;
				case WINDOW_GROUP_TOP:
					ownerFrame.y += winFrame.h * op;
					// fallthrough
				case WINDOW_GROUP_BOTTOM:
					ownerFrame.h -= winFrame.h * op;
					break;
			}
		}

		frame.w = ownerFrame.w;
		frame.h = ownerFrame.h;
	}
	// 4 == BottomAdded; 5 == Inactivating
	else if (type <= 5) {
		windowGroupCounts[WINDOW_GROUP_BOTTOM] += op;
		ownerFrame.h -= winFrame.h * op;
		if (op == WINDOW_CONTRACT && type == 5) {
			frame.h = 0;
		} else {
			frame.h = ownerFrame.h;
		}
	} else {
		Log(ERROR, "GameControl", "Unknown resize type: %d", type);
	}
	Owner->View::SetFrame(ownerFrame);
}

//Create an overhead text over a scriptable target
//Multiple texts are possible, as this code copies the text to a new object
void GameControl::DisplayString(Scriptable* target)
{
	Scriptable* scr = new Scriptable( ST_TRIGGER );
	scr->SetOverheadText(target->GetOverheadText());
	scr->Pos = target->Pos;

	// add as a "subtitle" to the main message window
	ieDword tmp = 0;
	core->GetDictionary()->Lookup("Duplicate Floating Text", tmp);
	if (tmp && !target->GetOverheadText().empty()) {
		// pass NULL target so pst does not display multiple
		displaymsg->DisplayString(target->GetOverheadText());
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
	if (pc && (ScreenFlags&SF_CENTERONACTOR)) {
		MoveViewportTo( pc->Pos, true );
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
Sprite2D* GameControl::GetScreenshot(const Region& rgn, bool show_gui)
{
	Sprite2D* screenshot;
	if (show_gui) {
		screenshot = core->GetVideoDriver()->GetScreenshot( rgn );
	} else {
		int hf = SetGUIHidden(true);
		Draw ();
		screenshot = core->GetVideoDriver()->GetScreenshot( rgn );
		if (hf) {
			SetGUIHidden(false);
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

	Sprite2D *screenshot = GetScreenshot( Region(x, y, w, h) );

	Sprite2D* preview = video->SpriteScaleDown ( screenshot, 5 );
	Sprite2D::FreeSprite( screenshot );
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
	Sprite2D::FreeSprite( img );

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
bool GameControl::SetEvent(int /*eventType*/, ControlEventHandler /*handler*/)
{
	return false;
}

void GameControl::SetDisplayText(String* text, unsigned int time)
{
	delete DisplayText;
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
