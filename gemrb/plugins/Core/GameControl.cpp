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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * $Id$
 */

#ifndef WIN32
#include <sys/time.h>
#endif
#include "../../includes/win32def.h"
#include "GameControl.h"
#include "Interface.h"
#include "DialogMgr.h"
#include "../../includes/strrefs.h"
#include "Effect.h"
#include "GSUtils.h"
#include "TileMap.h"
#include "Video.h"
#include "ResourceMgr.h"
#include "ScriptEngine.h"
#include "Item.h"
#include "Game.h"
#include "SaveGameIterator.h"

#define DEBUG_SHOW_INFOPOINTS   0x01
#define DEBUG_SHOW_CONTAINERS   0x02
#define DEBUG_SHOW_DOORS	DEBUG_SHOW_CONTAINERS
#define DEBUG_SHOW_SEARCHMAP    0x04
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
/*
static Color white = {
	0xff, 0xff, 0xff, 0xff
};
*/
static const Color black = {
	0x00, 0x00, 0x00, 0xff
};
static const Color blue = {
	0x00, 0x00, 0xff, 0x80
};

//Animation* effect;

#define FORMATIONSIZE 10
typedef Point formation_type[FORMATIONSIZE];
ieDword formationcount;
static formation_type *formations=NULL;
static bool mqs = false;

void GameControl::SetTracker(Actor *actor, ieDword dist)
{
	trackerID = actor->GetID();
	distance = dist;
}

void GameControl::MultipleQuickSaves(int arg)
{
	mqs=arg==1;
}

GameControl::GameControl(void)
{
	if (!formations) {
		ReadFormations();
	}
	//this is the default action, individual actors should have one too
	//at this moment we use only this
	//maybe we don't even need it
	//action = GA_DEFAULT | GA_SELECT | GA_NO_DEAD;
	Changed = true;
	spellCount = 0;
	user = NULL;
	lastActorID = 0;
	trackerID = 0;
	distance = 0;
	MouseIsDown = false;
	DrawSelectionRect = false;
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
//	effect = NULL;
	ieDword tmp=0;

	target_mode = TARGET_MODE_NONE;

	core->GetDictionary()->Lookup("Center",tmp);
	if (tmp) {
		ScreenFlags=SF_ALWAYSCENTER|SF_CENTERONACTOR;
	}
	else {
		ScreenFlags = SF_CENTERONACTOR;
	}
	LeftCount = 0;
	BottomCount = 0;
	RightCount = 0;
	TopCount = 0;
	DialogueFlags = 0;
	dlg = NULL;
	targetID = 0;
	speakerID = 0;
	targetOB = NULL;
}

//actually the savegame contains some formation data too, how to use it?
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

Point GameControl::GetFormationOffset(ieDword formation, ieDword pos)
{
	if (formation>=formationcount) formation = 0;
	if (pos>=FORMATIONSIZE) pos=FORMATIONSIZE-1;
	return formations[formation][pos];
}

//don't pass p as a reference
void GameControl::MoveToPointFormation(Actor *actor, Point p, int Orient)
{
	unsigned int pos;
	char Tmp[256];

	int formation=core->GetGame()->GetFormation();
	pos=actor->InParty-1; //either this or the actual # of selected actor?
	if (pos>=FORMATIONSIZE) pos=FORMATIONSIZE-1;
	switch(Orient) {
	case 11: case 12: case 13://east
		p.x-=formations[formation][pos].y*30;
		p.y+=formations[formation][pos].x*30;
		break;
	case 6: case 7: case 8: case 9: case 10: //north
		p.x+=formations[formation][pos].x*30;
		p.y+=formations[formation][pos].y*30;
		break;
	case 3: case 4: case 5: //west
		p.x+=formations[formation][pos].y*30;
		p.y-=formations[formation][pos].x*30;
		break;
	case 0: case 1: case 2: case 14: case 15://south
		p.x-=formations[formation][pos].x*30;
		p.y-=formations[formation][pos].y*30;
		break;
	}
	sprintf( Tmp, "MoveToPoint([%d.%d])", p.x, p.y );
	actor->AddAction( GenerateAction( Tmp) );
}

GameControl::~GameControl(void)
{
	//releasing the viewport of GameControl
	core->GetVideoDriver()->SetViewport( 0,0,0,0 );
	if (formations)	{
		free( formations );
		formations = NULL;
	}
	if (dlg) {
		delete dlg;
	}
}

void GameControl::AutoSave()
{
	const char *folder;
	AutoTable tab("savegame");
	if (tab) {
		folder = tab->QueryField(0);
		core->GetSaveGameIterator()->CreateSaveGame(0, folder, 0);
	}
}

void GameControl::QuickSave()
{
	const char *folder;
	AutoTable tab("savegame");
	if (tab) {
		folder = tab->QueryField(1);
		core->GetSaveGameIterator()->CreateSaveGame(1, folder, mqs == 1);
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

//don't use reference for point!!!
void GameControl::DrawArrowMarker(Region &screen, Point p, Region &viewport)
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
		video->BlitGameSprite( core->GetScrollCursorSprite(arrow_orientations[draw], 0), p.x+screen.x, p.y+screen.y, 0, black, NULL);
	}
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

	//i'm not sure if this should be here
	//if ( game->selected.size() > 0 ) {
	//	ChangeMap(core->GetFirstSelectedPC(true), false);
	//}

	//in multi player (if we ever get to it), only the server must call this
	//if (update_scripts) {
	//	// the game object will run the area scripts as well
	//	game->UpdateScripts();
	//}

	if (Owner->Visible!=WINDOW_VISIBLE) {
		return;
	}

	Video* video = core->GetVideoDriver();
	Region viewport = video->GetViewport();
	if (moveX || moveY) {
		viewport.x += moveX;
		viewport.y += moveY;
		Point mapsize = core->GetGame()->GetCurrentArea()->TMap->GetMapSize();
		if ( viewport.x < 0 )//if we are at top of the map
			viewport.x = 0;
		else if ( (viewport.x + viewport.w) >= mapsize.x) //if we are at the bottom
			viewport.x = mapsize.x - viewport.w;

		if ( viewport.y < 0 ) //if we are at the left of the map
			viewport.y = 0;
		else if ( (viewport.y + viewport.h ) >= mapsize.y ) //if we are at the right
			viewport.y = mapsize.y - viewport.h;
		
		core->timer->SetMoveViewPort( viewport.x, viewport.y, 0, false );
	}
	Region screen( x + XPos, y + YPos, Width, Height );
	Map* area = game->GetCurrentArea( );
	if (!area) {
		video->DrawRect( screen, blue, true );
		return;
	}
	video->DrawRect( screen, black, true );

	// setup outlines
	bool targetting = (target_mode & (TARGET_MODE_CAST |
		TARGET_MODE_PICK | TARGET_MODE_ATTACK | TARGET_MODE_DEFEND));
	InfoPoint *i;
	for (unsigned int idx = 0; (i = area->TMap->GetInfoPoint( idx )); idx++) {
		i->Highlight = false;
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
	for (unsigned int idx = 0; (d = area->TMap->GetDoor( idx )); idx++) {
		d->Highlight = false;
		if (overDoor == d && targetting) {
			if (d->VisibleTrap(0) || (d->Flags & DOOR_LOCKED)) {
				// only highlight targettable doors
				d->outlineColor = green;
				d->Highlight = true;
				continue;
			}
		} else if (overDoor == d && !(d->Flags & DOOR_SECRET)) {
			// mouse over, not in target mode, no secret door
			d->outlineColor = cyan;
			d->Highlight = true;
			continue;
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
	for (unsigned int idx = 0; (c = area->TMap->GetContainer( idx )); idx++) {
		c->Highlight = false;
		if (overContainer == c && targetting) {
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
			Actor **monsters = area->GetAllActorsInRadius(actor->Pos, GA_NO_DEAD, distance);

			int i = 0;
			while(monsters[i]) {
				Actor *target = monsters[i++];
				if (target->InParty) continue;
				if (target->GetStat(IE_NOTRACKING)) continue;
				DrawArrowMarker(screen, target->Pos, viewport);
			}
			delete monsters;
		} else {
			trackerID = 0;
		}
	}

	//in multi player (if we ever get to it), only the server must call this
	//if (update_scripts) {
	//	// the game object will run the area scripts as well
	//	game->UpdateScripts();
	//}

	if (ScreenFlags & SF_DISABLEMOUSE)
		return;
	Point p(lastMouseX, lastMouseY);
	video->ConvertToGame( p.x, p.y );

	// Draw selection rect
	if (DrawSelectionRect) {
		CalculateSelection( p );
		video->DrawRect( SelectionRect, green, false, true );
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

	// Draw searchmap
	if (DebugFlags & DEBUG_SHOW_SEARCHMAP) {
		Sprite2D* spr = area->SearchMap->GetImage();
		video->BlitSprite( spr, 0, 0, true );
		video->FreeSprite( spr );
		Region point( p.x / 16, p.y / 12, 2, 2 );
		video->DrawRect( point, red );
	} else if (DebugFlags & DEBUG_SHOW_LIGHTMAP) {
		Sprite2D* spr = area->LightMap->GetImage();
		video->BlitSprite( spr, 0, 0, true );
		video->FreeSprite( spr );
		Region point( p.x / 16, p.y / 12, 2, 2 );
		video->DrawRect( point, red );
	}

}
/** Sets the Text of the current control, unused here */
int GameControl::SetText(const char* /*string*/, int /*pos*/)
{
	return 0;
}
/** Key Press Event */
void GameControl::OnKeyPress(unsigned char Key, unsigned short /*Mod*/)
{
	if (DialogueFlags&DF_IN_DIALOG) {
		return;
	}
	unsigned int i;
	Game* game = core->GetGame();
	if (!game) return;

	switch (Key) {
		case '0':
			game->SelectActor( NULL, false, SELECT_NORMAL );
			i = game->GetPartySize(false)/2;
			while(i--) {
				SelectActor(i, true);
			}
			break;
		case '-':
			game->SelectActor( NULL, true, SELECT_NORMAL );
			i = game->GetPartySize(false)/2;
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
		case '7':
		case '8':
		case '9':
			SelectActor(Key-'1');
			break;
	default:
		core->GetGame()->SetHotKey(toupper(Key));
		break;
	}
}

void GameControl::SelectActor(int whom, int type)
{
	Game* game = core->GetGame();
	if (whom==-1) {
		game->SelectActor( NULL, true, SELECT_NORMAL );
		return;
	}

	/* doesn't fall through here */
	Actor* actor = game->GetPC( whom,false );
	if (!actor)
		return;

	if (type==0) {
		game->SelectActor( actor, false, SELECT_NORMAL );
		return;
	}
	if (type==1) {
		game->SelectActor( actor, true, SELECT_NORMAL );
		return;
	}

	bool was_selected = actor->IsSelected();
	if (game->SelectActor( actor, true, SELECT_REPLACE ))
		if (was_selected || (ScreenFlags & SF_ALWAYSCENTER)) {
			ScreenFlags |= SF_CENTERONACTOR;
		}
}

static EffectRef heal_ref={"CurrentHPModifier", NULL, -1};

/** Key Release Event */
void GameControl::OnKeyRelease(unsigned char Key, unsigned short Mod)
{
	unsigned int i;
	Game* game = core->GetGame();

	if (!game)
		return;

	if (DialogueFlags&DF_IN_DIALOG) {
		if (Mod) return;
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
						ta->OnKeyPress(Key,Mod);
					}
				}
				break;
		}
		return;
	}
	//cheatkeys with ctrl-
	if (Mod & GEM_MOD_CTRL) {
		if (!core->CheatEnabled()) {
			return;
		}
		Map* area = game->GetCurrentArea( );
		if (!area)
			return;
		Actor *lastActor = area->GetActorByGlobalID(lastActorID);
		Point p(lastMouseX, lastMouseY);
		core->GetVideoDriver()->ConvertToGame( p.x, p.y );
		switch (Key) {
			case 'f':
				core->GetVideoDriver()->ToggleFullscreenMode();
				break;
			case 'd': //disarm ?
				if (overInfoPoint) {
					overInfoPoint->DetectTrap(256);
				}
				if (overContainer) {
					if (overContainer->Trapped &&
						!( overContainer->TrapDetected )) {
						overContainer->TrapDetected = 1;
					}
				}
				break;
			case 'l':
				//the original engine was able to swap through all animations
				if (lastActor) {
					lastActor->AddAnimation("S056ICBL", 0, 0, 0);
				}
				break;

			case 'c':
				if (game->selected.size() > 0 && lastActor) {
					Actor *src = game->selected[0];
					src->CastSpell( "SPWI207", lastActor, false );
				}
				break;

			case 'b':
				//path
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

			case 'o':
				// origin
				pfs.x = lastMouseX;
				pfs.y = lastMouseY;
				core->GetVideoDriver()->ConvertToGame( pfs.x, pfs.y );
				break;
			case 'a':
				//animation
				if (lastActor) {
					lastActor->GetNextAnimation();
				}
				break;
			case 's':
				//stance
				if (lastActor) {
					lastActor->GetNextStance();
				}
				break;
			case 'j':
				// jump
				for (i = 0; i < game->selected.size(); i++) {
					Actor* actor = game->selected[i];
					MoveBetweenAreasCore(actor, core->GetGame()->CurrentArea, p, -1, true);
					printf( "Teleported to %d, %d\n", p.x, p.y );
				}
				break;

			case 'm':
				// 'm' ? debugdump
				if (!lastActor) {
					lastActor = area->GetActor( p, GA_DEFAULT);
				}
				if (lastActor) {
					lastActor->DebugDump();
					break;
				}
				if (overDoor) {
					overDoor->DebugDump();
					break;
				}
				if (overContainer) {
					overContainer->DebugDump();
					break;
				}
				if (overInfoPoint) {
					overInfoPoint->DebugDump();
					break;
				}
				core->GetGame()->GetCurrentArea()->DebugDump();
				break;
			case 'v':
				// explore map from point
				area->ExploreMapChunk( p, rand()%30, 1 );
				break;
			case 'x':
				// shows coordinates
				printf( "%s [%d.%d]\n", area->GetScriptName(), p.x, p.y );
				break;
			case 'g'://shows loaded areas
				game->DebugDump();
				break;
			case 'i':
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
					Effect *fx = EffectQueue::CreateEffect(heal_ref, lastActor->GetBase(IE_MAXHITPOINTS), 0x30001, FX_DURATION_INSTANT_PERMANENT);
					if (fx) {
						core->ApplyEffect(fx, lastActor, lastActor);
					}
				}
				break;
			case 't'://advances time
				// 7200 (one day) /24 (hours) == 300
				game->AdvanceTime(300);
				//refresh gui here once we got it
				break;

			case 'q': //joins actor
				if (lastActor && !lastActor->InParty) {
					lastActor->ClearActions();
					lastActor->ClearPath();
					char Tmp[40];
					strncpy(Tmp,"JoinParty()",sizeof(Tmp) );
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
					strncpy(Tmp,"LeaveParty()",sizeof(Tmp) );
					lastActor->AddAction( GenerateAction(Tmp) );
				}
				break;
			case 'y': //kills actor
				if (lastActor) {
					//using action so the actor is killed
					//correctly (synchronisation)
					lastActor->ClearActions();
					lastActor->ClearPath();
					char Tmp[40];
					strncpy(Tmp,"Kill(Myself)",sizeof(Tmp) );
					lastActor->AddAction( GenerateAction(Tmp) );
				}
				break;
			case 'z':
				if (lastActor) {
					lastActor->GetPrevAnimation();
				}
				break;
			case '1':
				//change paperdoll armour level
				if (! lastActor)
					break;
				lastActor->NewStat(IE_ARMOR_TYPE,1,MOD_ADDITIVE);
				break;
			case '4':
				//show all traps and infopoints
				DebugFlags ^= DEBUG_SHOW_INFOPOINTS;
				printf("Show traps and infopoints %s\n", DebugFlags & DEBUG_SHOW_INFOPOINTS ? "ON" : "OFF");
				break;
			case '5':
				//show the searchmap
				DebugFlags ^= DEBUG_SHOW_SEARCHMAP;
				printf("Show searchmap %s\n", DebugFlags & DEBUG_SHOW_SEARCHMAP ? "ON" : "OFF");
				break;
			case '6':
				//show the lightmap
				DebugFlags ^= DEBUG_SHOW_LIGHTMAP;
				printf("Show lightmap %s\n", DebugFlags & DEBUG_SHOW_LIGHTMAP ? "ON" : "OFF");
				break;
			case '7':
				//show fog of war
				core->FogOfWar ^= 1;
				printf("Show Fog-Of-War: %s\n", core->FogOfWar & 1 ? "ON" : "OFF");
				break;
			case '8':
				//show searchmap on area
				core->FogOfWar ^= 2;
				printf("Show searchmap %s\n", core->FogOfWar & 2 ? "ON" : "OFF");
				break;
			default:
				printf( "KeyRelease:%d - %d\n", Key, Mod );
				break;
		}
		return; //return from cheatkeys
	}
	switch (Key) {
		case '\t':
			if (DialogueFlags & DF_FREEZE_SCRIPTS) break;
			//fallthrough
		case ' ':
			DialogueFlags ^= DF_FREEZE_SCRIPTS;
	 		if (DialogueFlags&DF_FREEZE_SCRIPTS) {
				core->DisplayConstantString(STR_PAUSED,0xff0000);
		 	} else {
				core->DisplayConstantString(STR_UNPAUSED,0xff0000);
			}
			break;
		case 'q':
			QuickSave();
			break;
		case GEM_ALT:
			DebugFlags &= ~DEBUG_SHOW_CONTAINERS;
			break;
		default:
			break;
	}
}

int GameControl::GetCursorOverDoor(Door *overDoor)
{
	if (target_mode&TARGET_MODE_PICK) {
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

int GameControl::GetCursorOverContainer(Container *overContainer)
{
	if (target_mode&TARGET_MODE_PICK) {
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

/** Mouse Over Event */
void GameControl::OnMouseOver(unsigned short x, unsigned short y)
{
	if (ScreenFlags & SF_DISABLEMOUSE) {
		return;
	}

	lastMouseX = x;
	lastMouseY = y;
	Point p( x,y );
	core->GetVideoDriver()->ConvertToGame( p.x, p.y );
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
		nextCursor = overInfoPoint->Cursor;
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
		if (overDoor) {
			nextCursor = GetCursorOverDoor(overDoor);
		}

		if (overContainer) {
			nextCursor = GetCursorOverContainer(overContainer);
		}

		lastActor = area->GetActor( p, GA_DEFAULT | GA_SELECT | GA_NO_DEAD | GA_NO_HIDDEN);
		if (lastActor) {
			lastActorID = lastActor->globalID;
			lastActor->SetOver( true );
			ieDword type = lastActor->GetStat(IE_EA);
			if (type >= EA_EVILCUTOFF) {
				nextCursor = IE_CURSOR_ATTACK;
			} else if ( type > EA_CHARMED ) {
				nextCursor = IE_CURSOR_TALK;
			} else {
				nextCursor = IE_CURSOR_NORMAL;
			}
		} else {
			lastActorID = 0;
		}

		if (target_mode & TARGET_MODE_TALK) {
			nextCursor = IE_CURSOR_TALK;
		} else if (target_mode & TARGET_MODE_ATTACK) {
			nextCursor = IE_CURSOR_ATTACK;
		} else if (target_mode & TARGET_MODE_CAST) {
			nextCursor = IE_CURSOR_CAST;
		} else if (target_mode & TARGET_MODE_DEFEND) {
			nextCursor = IE_CURSOR_DEFEND;
		} else if (target_mode & TARGET_MODE_PICK) {
			if (lastActor) {
				nextCursor = IE_CURSOR_PICK;
				if (lastActor == core->GetFirstSelectedPC(false)) {
					nextCursor |= IE_CURSOR_GRAY;
				}
			} else {
				if (!overContainer && !overDoor) {
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
					if (target_mode & TARGET_MODE_ALLY)
						nextCursor^=1;
					break;

				case EA_ENEMY:
				case EA_GOODBUTRED:
					if (target_mode & TARGET_MODE_ENEMY)
						nextCursor^=1;
					break;
				default:
					if (target_mode & TARGET_MODE_NEUTRAL)
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

#define SCROLL_BORDER 5

/** Global Mouse Move Event */
void GameControl::OnGlobalMouseMove(unsigned short x, unsigned short y)
{
	if (ScreenFlags & SF_DISABLEMOUSE) {
		return;
	}
	
	if (Owner->Visible!=WINDOW_VISIBLE) {
		return;
	}

	int mousescrollspd = core->GetMouseScrollSpeed();

	if (x <= SCROLL_BORDER)
		moveX = -mousescrollspd;
	else {
		if (x >= ( core->Width - SCROLL_BORDER ))
			moveX = mousescrollspd;
		else
			moveX = 0;
	}
	if (y <= SCROLL_BORDER)
		moveY = -mousescrollspd;
	else {
		if (y >= ( core->Height - SCROLL_BORDER ))
			moveY = mousescrollspd;
		else
			moveY = 0;
	}

	if (moveX != 0 || moveY != 0) {
		scrolling = true;
	} else if (scrolling) {
		scrolling = false;

		Video* video = core->GetVideoDriver();
		video->SetDragCursor(NULL);
	}
}

void GameControl::UpdateScrolling() {
	if (!scrolling) return;
	
	int mousescrollspd = core->GetMouseScrollSpeed(); // TODO: why check against this value and not +/-?
	Video* video = core->GetVideoDriver();

	if (moveX == mousescrollspd && moveY == 0) { // right
		video->SetDragCursor(core->GetScrollCursorSprite(0,numScrollCursor));
	} else if (moveX == mousescrollspd && moveY == -mousescrollspd) { // upper right
		video->SetDragCursor(core->GetScrollCursorSprite(1,numScrollCursor));
	} else if (moveX == 0 && moveY == -mousescrollspd) { // up
		video->SetDragCursor(core->GetScrollCursorSprite(2,numScrollCursor));
	} else if (moveX == -mousescrollspd && moveY == -mousescrollspd) { // upper left
		video->SetDragCursor(core->GetScrollCursorSprite(3,numScrollCursor));
	} else if (moveX == -mousescrollspd && moveY == 0) { // left
		video->SetDragCursor(core->GetScrollCursorSprite(4,numScrollCursor));
	} else if (moveX == -mousescrollspd && moveY == mousescrollspd) { // bottom left
		video->SetDragCursor(core->GetScrollCursorSprite(5,numScrollCursor));
	} else if (moveX == 0 && moveY == mousescrollspd) { // bottom
		video->SetDragCursor(core->GetScrollCursorSprite(6,numScrollCursor));
	} else if (moveX == mousescrollspd && moveY == mousescrollspd) { // bottom right
		video->SetDragCursor(core->GetScrollCursorSprite(7,numScrollCursor));
	}

	numScrollCursor = (numScrollCursor+1) % 15;
}

void GameControl::TryToAttack(Actor *source, Actor *tgt)
{
	char Tmp[40];

	source->ClearPath();
	source->ClearActions();
	strncpy(Tmp,"NIDSpecial3()",sizeof(Tmp) );
	source->AddAction( GenerateActionDirect( Tmp, tgt) );
}

void GameControl::TryToDefend(Actor *source, Actor *tgt)
{
	char Tmp[40];

	source->ClearPath();
	source->ClearActions();
	strncpy(Tmp,"NIDSpecial4()",sizeof(Tmp) );
	source->AddAction( GenerateActionDirect( Tmp, tgt) );
}

void GameControl::TryToPick(Actor *source, Actor *tgt)
{
	char Tmp[40];

	source->ClearPath();
	source->ClearActions();
	strncpy(Tmp,"PickPockets([-1])", sizeof(Tmp) );
	source->AddAction( GenerateActionDirect( Tmp, tgt) );
}

void GameControl::TryToPick(Actor *source, Door *tgt)
{
	char Tmp[40];

	source->ClearPath();
	source->ClearActions();
	if (tgt->Trapped && tgt->TrapDetected) {
		snprintf(Tmp, sizeof(Tmp), "RemoveTraps(\"%s\")", tgt->GetScriptName() );
	} else {
		snprintf(Tmp, sizeof(Tmp), "PickLock(\"%s\")", tgt->GetScriptName() );
	}
	source->AddAction( GenerateAction( Tmp ) );
}

void GameControl::TryToPick(Actor *source, Container *tgt)
{
	char Tmp[40];

	source->ClearPath();
	source->ClearActions();
	if (tgt->Trapped && tgt->TrapDetected) {
		snprintf(Tmp, sizeof(Tmp), "RemoveTraps(\"%s\")", tgt->GetScriptName() );
	} else {
		snprintf(Tmp, sizeof(Tmp), "PickLock(\"%s\")", tgt->GetScriptName() );
	}
	source->AddAction( GenerateAction( Tmp ) );
}

void GameControl::TryToDisarm(Actor *source, InfoPoint *tgt)
{
	if (tgt->Type!=ST_PROXIMITY) return;

	char Tmp[40];

	source->ClearPath();
	source->ClearActions();
	snprintf(Tmp, sizeof(Tmp), "RemoveTraps(\"%s\")", tgt->GetScriptName() );
	source->AddAction( GenerateAction( Tmp ) );
}

void GameControl::TryToBash(Actor *source, Scriptable *tgt)
{
	char Tmp[40];

	source->ClearPath();
	source->ClearActions();
	snprintf(Tmp, sizeof(Tmp), "Attack(\"%s\")", tgt->GetScriptName() );
	//snprintf(Tmp, sizeof(Tmp), "NIDSpecial9(\"%s\")", tgt->GetScriptName() );
	source->AddAction( GenerateAction( Tmp ) );
}

void GameControl::TryToCast(Actor *source, Point &tgt)
{
	char Tmp[40];

	if (!spellCount) {
		target_mode = TARGET_MODE_NONE;
		return; //not casting or using an own item
	}
	spellCount--;
	if (spellOrItem>=0) {
		sprintf(Tmp, "NIDSpecial8()");
	} else {
		//using item on target
		sprintf(Tmp, "NIDSpecial7()");
	}
	Action* action = GenerateAction( Tmp );
	action->pointParameter=tgt;
	if (spellOrItem>=0)
	{
		CREMemorizedSpell *si;
		//spell casting at target
		si = source->spellbook.GetMemorizedSpell(spellOrItem, spellSlot, spellIndex);
		if (!si)
		{
			target_mode = TARGET_MODE_NONE;
			return;
		}
		sprintf(action->string0Parameter,"%.8s",si->SpellResRef);
	}
	else
	{
		action->int0Parameter=spellSlot;
		action->int1Parameter=spellIndex;
	}
	source->AddAction( action );
	if (!spellCount) {
		target_mode = TARGET_MODE_NONE;
	}
}

void GameControl::TryToCast(Actor *source, Actor *tgt)
{
	char Tmp[40];

	if (!spellCount) {
		target_mode = TARGET_MODE_NONE;
		return; //not casting or using an own item
	}
	spellCount--;
	if (spellOrItem>=0) {
		sprintf(Tmp, "NIDSpecial6()");
	} else {
		//using item on target
		sprintf(Tmp, "NIDSpecial5()");
	}
	Action* action = GenerateActionDirect( Tmp, tgt);
	if (spellOrItem>=0)
	{
		CREMemorizedSpell *si;
		//spell casting at target
		si = source->spellbook.GetMemorizedSpell(spellOrItem, spellSlot, spellIndex);
		if (!si)
		{
			target_mode = TARGET_MODE_NONE;
			return;
		}
		sprintf(action->string0Parameter,"%.8s",si->SpellResRef);
	}
	else
	{
		action->int0Parameter=spellSlot;
		action->int1Parameter=spellIndex;
	}
	source->AddAction( action );
	if (!spellCount) {
		target_mode = TARGET_MODE_NONE;
	}
}

void GameControl::TryToTalk(Actor *source, Actor *tgt)
{
	char Tmp[40];

	//Nidspecial1 is just an unused action existing in all games
	//(non interactive demo)
	//i found no fitting action which would emulate this kind of
	//dialog initation
	source->ClearPath();
	source->ClearActions();
	strncpy(Tmp,"NIDSpecial1()",sizeof(Tmp) );
	targetID = tgt->globalID; //this is a hack, but not so deadly
	source->AddAction( GenerateActionDirect( Tmp, tgt) );
}

void GameControl::HandleContainer(Container *container, Actor *actor)
{
	char Tmp[256];

	if ((target_mode&TARGET_MODE_CAST) && spellCount) {
		//we'll get the container back from the coordinates
		TryToCast(actor, container->Pos);
		return;
	}

	if (target_mode&TARGET_MODE_ATTACK) {
		TryToBash(actor, container);
		return;
	}

	if ((target_mode&TARGET_MODE_PICK)) {
		TryToPick(actor, container);
		return;
	}

	actor->ClearPath();
	actor->ClearActions();
	strncpy(Tmp,"UseContainer()",sizeof(Tmp) );
	core->SetCurrentContainer( actor, container);
	actor->AddAction( GenerateAction( Tmp) );
}

void GameControl::HandleDoor(Door *door, Actor *actor)
{
	char Tmp[256];

	if ((target_mode&TARGET_MODE_CAST) && spellCount) {
		//we'll get the door back from the coordinates
		TryToCast(actor, door->Pos);
		return;
	}

	if (target_mode&TARGET_MODE_ATTACK) {
		TryToBash(actor, door);
		return;
	}

	if ( (target_mode&TARGET_MODE_PICK) || door->TrapDetected) {
		TryToPick(actor, door);
		return;
	}

	if (door->IsOpen()) {
		actor->ClearPath();
		actor->ClearActions();
		sprintf( Tmp, "CloseDoor(\"%s\")", door->GetScriptName() );
		actor->AddAction( GenerateAction( Tmp) );
	} else {
		actor->ClearPath();
		actor->ClearActions();
		sprintf( Tmp, "OpenDoor(\"%s\")", door->GetScriptName() );
		actor->AddAction( GenerateAction( Tmp) );
	}
}

bool GameControl::HandleActiveRegion(InfoPoint *trap, Actor * actor, Point &p)
{
	if ((target_mode&TARGET_MODE_CAST) && spellCount) {
		//we'll get the active region from the coordinates (if needed)
		TryToCast(actor, p);
		//don't bother with this region further
		return true;
	}
	switch(trap->Type) {
		case ST_TRAVEL:
			trap->Flags|=TRAP_RESET;
			return false;
		case ST_TRIGGER:
			//the importer shouldn't load the script
			//if it is unallowed anyway (though
			//deactivated scripts could be reactivated)
			//only the 'trapped' flag should be honoured
			//there. Here we have to check on the
			//reset trap and deactivated flags
			if (trap->Scripts[0]) {
				if (!(trap->Flags&TRAP_DEACTIVATED) ) {
					trap->LastTrigger = actor->GetID();
					trap->ImmediateEvent();
					//directly feeding the event, even if there are actions in the queue
					trap->Scripts[0]->Update();
					trap->ProcessActions(true);
					//if reset trap flag not set, deactivate it
					//hmm, better not, info triggers don't deactivate themselves on click
					//if (!(trap->Flags&TRAP_RESET)) {
					//	trap->Flags|=TRAP_DEACTIVATED;
					//}
				}
			} else {
				if (trap->overHeadText) {
					if (trap->textDisplaying != 1) {
						trap->textDisplaying = 1;
						trap->timeStartDisplaying = core->GetGame()->Ticks;
						DisplayString( trap );
					}
				}
			}
			if (trap->Flags&TRAP_USEPOINT) {
				//overriding the target point
				p = trap->Pos;
				return false;
			}
			return true;
		default:;
	}
	return false;
}
/** Mouse Button Down */
void GameControl::OnMouseDown(unsigned short x, unsigned short y, unsigned short Button,
	unsigned short /*Mod*/)
{
	if (ScreenFlags&SF_DISABLEMOUSE)
		return;

	short px=x;
	short py=y;
	switch(Button)
	{
	case GEM_MB_SCRLUP:
		OnSpecialKeyPress(GEM_UP);
		break;
	case GEM_MB_SCRLDOWN:
		OnSpecialKeyPress(GEM_DOWN);
		break;
	case GEM_MB_ACTION:
		core->GetVideoDriver()->ConvertToGame( px, py );
		MouseIsDown = true;
		SelectionRect.x = px;
		SelectionRect.y = py;
		StartX = px;
		StartY = py;
		SelectionRect.w = 0;
		SelectionRect.h = 0;
	}
}
/** Mouse Button Up */
void GameControl::OnMouseUp(unsigned short x, unsigned short y, unsigned short Button,
	unsigned short /*Mod*/)
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
	Map* area = game->GetCurrentArea( );

	if (DrawSelectionRect) {
		Actor** ab;
		unsigned int count = area->GetActorInRect( ab, SelectionRect,true );
		for (i = 0; i < highlighted.size(); i++)
			highlighted[i]->SetOver( false );
		highlighted.clear();
		game->SelectActor( NULL, false, SELECT_NORMAL );
		if (count != 0) {
			for (i = 0; i < count; i++) {
				// FIXME: should call handler only once
				game->SelectActor( ab[i], true, SELECT_NORMAL );
			}
		}
		free( ab );
		DrawSelectionRect = false;
		return;
	}

	//hidden actors are not selectable by clicking on them
	Actor* actor = area->GetActor( p, GA_DEFAULT | GA_SELECT | GA_NO_DEAD | GA_NO_HIDDEN);
	if (Button == GEM_MB_MENU) {
		if (actor) {
			//from GSUtils
			DisplayStringCore(actor, VB_SELECT+core->Roll(1,3,-1), DS_CONST|DS_CONSOLE);
			return;
		}
		core->GetDictionary()->SetAt( "MenuX", x );
		core->GetDictionary()->SetAt( "MenuY", y );
		core->GetGUIScriptEngine()->RunFunction( "OpenFloatMenuWindow" );
		return;
	}
	if (Button != GEM_MB_ACTION) {
		return;
	}

	if (!actor && ( game->selected.size() > 0 )) {
		if (overDoor) {
			HandleDoor(overDoor, core->GetFirstSelectedPC(false));
			return;
		}
		if (overInfoPoint) {
			if (HandleActiveRegion(overInfoPoint, core->GetFirstSelectedPC(false), p)) {
				return;
			}
		}
		if (overContainer) {
			HandleContainer(overContainer, core->GetFirstSelectedPC(false));
			return;
		}

		//just a single actor, no formation
		if (game->selected.size()==1) {
			//the player is using an item or spell on the ground
			if ((target_mode&TARGET_MODE_CAST) && spellCount) {
				TryToCast(core->GetFirstSelectedPC(false), p);
				return;
			}

			actor=game->selected[0];
			actor->ClearPath();
			actor->ClearActions();
			sprintf( Tmp, "MoveToPoint([%d.%d])", p.x, p.y );
			actor->AddAction( GenerateAction( Tmp) );
			//p is a searchmap travel region
			if ( actor->GetCurrentArea()->GetCursor(p) == IE_CURSOR_TRAVEL) {
				sprintf( Tmp, "NIDSpecial2()" );
				actor->AddAction( GenerateAction( Tmp) );
			}
			return;
		}
		//party formation movement
		int orient=GetOrient(p, game->selected[0]->Pos);
		for(unsigned int i = 0; i < game->selected.size(); i++) {
			actor=game->selected[i];
			actor->ClearPath();
			actor->ClearActions();
			//formations should be rotated based on starting point
			//of the leader? and destination
			MoveToPointFormation(actor,p,orient);
		}
		//p is a searchmap travel region
		if ( actor->GetCurrentArea()->GetCursor(p) == IE_CURSOR_TRAVEL) {
			sprintf( Tmp, "NIDSpecial2()" );
			actor->AddAction( GenerateAction( Tmp) );
		}
		return;
	}
	if (!actor) return;
	//we got an actor past this point
	DisplayStringCore(actor, VB_SELECT+core->Roll(1,3,-1), DS_CONST|DS_CONSOLE);

	//determining the type of the clicked actor
	ieDword type;

	type = actor->GetStat(IE_EA);
	if ( type >= EA_EVILCUTOFF ) {
		type = ACT_ATTACK; //hostile
	} else if ( type > EA_CHARMED ) {
		type = ACT_TALK; //neutral
	} else {
		type = ACT_NONE; //party
	}

	if (target_mode&TARGET_MODE_ATTACK) {
		type = ACT_ATTACK;
	} else if (target_mode&TARGET_MODE_TALK) {
		type = ACT_TALK;
	} else if (target_mode&TARGET_MODE_CAST) {
		type = ACT_CAST;
	} else if (target_mode&TARGET_MODE_DEFEND) {
		type = ACT_DEFEND;
	} else if (target_mode&TARGET_MODE_PICK) {
		type = ACT_THIEVING;
	}

	//we shouldn't zero this for two reasons in case of spell or item
	//1. there could be multiple targets
	//2. the target mode is important
	if (!(target_mode&TARGET_MODE_CAST) || !spellCount) {
		target_mode = TARGET_MODE_NONE;
	}

	switch (type) {
		case ACT_NONE: //none
			//clicked on a new party member
			// FIXME: call GameControl::SelectActor() instead
			//game->SelectActor( actor, true, SELECT_REPLACE );
			SelectActor( game->InParty(actor) );
			break;
		case ACT_TALK:
			//talk (first selected talks)
			if (game->selected.size()) {
				//if we are in PST modify this to NO!
				Actor *source;
				if (core->HasFeature(GF_PROTAGONIST_TALKS) ) {
					source = game->FindPC(1); //protagonist
				} else {
					source = core->GetFirstSelectedPC(false);
				}
				TryToTalk(source, actor);
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
				source = core->GetFirstSelectedPC(false);
				TryToCast(source, actor);
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
				source = core->GetFirstSelectedPC(false);
				TryToPick(source, actor);
			}
			break;
	}
}
/** Special Key Press */
void GameControl::OnSpecialKeyPress(unsigned char Key)
{
	if (DialogueFlags&DF_IN_DIALOG) {
		switch(Key) {
			case GEM_RETURN:
				//simulating the continue/end button pressed
				core->GetGUIScriptEngine()->RunFunction("CloseContinueWindow");
				break;
		}
		return; //don't accept keys in dialog
	}
	Region Viewport = core->GetVideoDriver()->GetViewport();
	Point mapsize = core->GetGame()->GetCurrentArea()->TMap->GetMapSize();
	switch (Key) {
		case GEM_LEFT:
			if (Viewport.x > 63)
				Viewport.x -= 64;
			else
				Viewport.x = 0;
			break;
		case GEM_UP:
			if (Viewport.y > 63)
				Viewport.y -= 64;
			else
				Viewport.y = 0;
			break;
		case GEM_DOWN:
			if (Viewport.y + Viewport.h + 64 < mapsize.y)
				Viewport.y += 64;
			else {
				Viewport.y = mapsize.y - Viewport.h;
				if (Viewport.y<0) Viewport.y=0;
			}
			break;
		case GEM_RIGHT:
			if (Viewport.x + Viewport.w + 64 < mapsize.x)
				Viewport.x += 64;
			else {
				Viewport.x = mapsize.x - Viewport.w;
				if (Viewport.x<0) Viewport.x=0;
			}
			break;
		case GEM_ALT:
			DebugFlags |= DEBUG_SHOW_CONTAINERS;
			return;
		case GEM_TAB:
			//no effect, i think
			return;
		case GEM_MOUSEOUT:
			moveX = 0;
			moveY = 0;
			return;
		default:
			return;
	}
	if (ScreenFlags & SF_LOCKSCROLL) {
		moveX = 0;
		moveY = 0;
	}
	else {
		core->timer->SetMoveViewPort( Viewport.x, Viewport.y, 0, false );
	}
}

void GameControl::CalculateSelection(Point &p)
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
		Actor* actor = area->GetActor( p, GA_DEFAULT | GA_SELECT | GA_NO_DEAD);
		//Actor* actor = area->GetActor( p, action);
		Actor *lastActor = area->GetActorByGlobalID(lastActorID);
		if (lastActor)
			lastActor->SetOver( false );
		if (!actor) {
			lastActorID = 0;
		} else {
			lastActorID = actor->globalID;
			actor->SetOver( true );
		}
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
			printMessage("GameControl", "Invalid Window Index: ", LIGHT_RED);
			printf("%s:%u\n",WindowName, index);
		}
	}
}

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
			printMessage("GameControl", "Invalid Window Index ", LIGHT_RED);
			printf("%s:%u\n",WindowName, index);
		}
	}
}

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

void GameControl::ResizeDel(Window* win, int type)
{
	switch (type) {
	case 0: //Left
		if (LeftCount!=1) {
			printMessage("GameControl","More than one left window!\n",LIGHT_RED);
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
			printMessage("GameControl","More than one bottom window!\n",LIGHT_RED);
		}
		BottomCount--;
		if (!BottomCount) {
			Owner->Height += win->Height;
			Height = Owner->Height;
		}
		break;

	case 2: //Right
		if (RightCount!=1) {
			printMessage("GameControl","More than one right window!\n",LIGHT_RED);
		}
		RightCount--;
		if (!RightCount) {
			Owner->Width += win->Width;
			Width = Owner->Width;
		}
		break;

	case 3: //Top
		if (TopCount!=1) {
			printMessage("GameControl","More than one top window!\n",LIGHT_RED);
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

void GameControl::InitDialog(Scriptable* spk, Scriptable* tgt, const char* dlgref)
{
	if (tgt->GetInternalFlag()&IF_NOINT) {
		core->DisplayConstantString(STR_TARGETBUSY,0xff0000);
		return;
	}

	DialogMgr* dm = ( DialogMgr* ) core->GetInterface( IE_DLG_CLASS_ID );
	dm->Open( core->GetResourceMgr()->GetResource( dlgref, IE_DLG_CLASS_ID ), true );
	if (dlg) {
		delete dlg;
	}
	dlg = dm->GetDialog();
	core->FreeInterface( dm );

	if (!dlg) {
		printMessage("GameControl", " ", LIGHT_RED);
		printf( "Cannot start dialog: %s\n", dlgref );
		return;
	}
	strnlwrcpy(dlg->ResRef, dlgref, 8); //this isn't handled by GetDialog???
	//target is here because it could be changed when a dialog runs onto
	//and external link, we need to find the new target (whose dialog was
	//linked to)

	Actor *spe = (Actor *) spk;
	speakerID = spe->globalID;
	if (tgt->Type!=ST_ACTOR) {
		targetID=0xffff;
		//most likely this dangling object reference
		//won't cause problems, because trigger points don't
		//get destroyed during a dialog
		targetOB=tgt;
		spk->LastTalkedTo=0;
	} else {
		Actor *tar = (Actor *) tgt;
		speakerID = spe->globalID;
		targetID = tar->globalID;
		spe->LastTalkedTo=targetID;
		tar->LastTalkedTo=speakerID;
	}

	//check if we are already in dialog
	if (DialogueFlags&DF_IN_DIALOG) {
		return;
	}
	UnhideGUI();
	ScreenFlags |= SF_GUIENABLED|SF_DISABLEMOUSE|SF_LOCKSCROLL;
	DialogueFlags |= DF_IN_DIALOG;

	if (tgt->Type==ST_ACTOR) {
		Actor *tar = (Actor *) tgt;
		tar->DialogInterrupt();
	}

	//allow mouse selection from dialog (even though screen is locked)
	core->GetVideoDriver()->SetMouseEnabled(true);
	core->timer->SetMoveViewPort( tgt->Pos.x, tgt->Pos.y, 0, true );
	//there are 3 bits, if they are all unset, the dialog freezes scripts
	if (!(dlg->Flags&7) ) {
		DialogueFlags |= DF_FREEZE_SCRIPTS;
	}
	//opening control size to maximum, enabling dialog window
	core->GetGame()->SetControlStatus(CS_HIDEGUI, BM_NAND);
	core->GetGame()->SetControlStatus(CS_DIALOG, BM_OR);
}

/*try to break will only try to break it, false means unconditional stop*/
void GameControl::EndDialog(bool try_to_break)
{
	if (try_to_break && (DialogueFlags&DF_UNBREAKABLE) ) {
		return;
	}

	Actor *tmp = GetSpeaker();
	if (tmp) {
		tmp->LeaveDialog();
	}
	speakerID = 0;
	if (targetID==0xffff) {
		targetOB->LeaveDialog();
	} else {
		tmp=GetTarget();
		if (tmp) {
			tmp->LeaveDialog();
		}
	}
	targetOB = NULL;
	targetID = 0;
	ds = NULL;
	if (dlg) {
		delete dlg;
		dlg = NULL;
	}
	//restoring original size
	core->GetGame()->SetControlStatus(CS_DIALOG, BM_NAND);
	ScreenFlags &=~(SF_DISABLEMOUSE|SF_LOCKSCROLL);
	DialogueFlags = 0;
}

//translate section values (journal, solved, unsolved, user)
static const int sectionMap[4]={4,1,2,0};

void GameControl::DialogChoose(unsigned int choose)
{
	char Tmp[256];

	TextArea* ta = core->GetMessageTextArea();
	if (!ta) {
		printMessage("GameControl","Dialog aborted???",LIGHT_RED);
		EndDialog();
		return;
	}

	Actor *speaker = GetSpeaker();
	if (!speaker) {
		printMessage("GameControl","Speaker gone???",LIGHT_RED);
		EndDialog();
		return;
	}
	Actor *tgt;
	Scriptable *target;

	if (targetID!=0xffff) {
		tgt = GetTarget();
		target = tgt;
	} else {
		//risky!!!
		target = targetOB;
		tgt=NULL;
	}
	if (!target) {
		printMessage("GameControl","Target gone???",LIGHT_RED);
		EndDialog();
		return;
	}

	//get the first state with true triggers!
	int si;
	if (choose == (unsigned int) -1) {
		si = dlg->FindFirstState( target );
		if (si < 0) {
			core->DisplayConstantStringName(STR_NOTHINGTOSAY,0xff0000,target);
			ta->SetMinRow( false );
			EndDialog();
			return;
		}
		//increasing talkcount after top level condition was determined

		if (tgt) {
			if (DialogueFlags&DF_TALKCOUNT) {
				DialogueFlags&=~DF_TALKCOUNT;
				tgt->TalkCount++;
			} else if (DialogueFlags&DF_INTERACT) {
				DialogueFlags&=~DF_INTERACT;
				tgt->InteractCount++;
			}
		}
		ds = dlg->GetState( si );
	} else {
		if (ds->transitionsCount <= choose) {
			return;
		}

		DialogTransition* tr = ds->transitions[choose];

		ta->PopMinRow();

		if (tr->Flags&IE_DLG_TR_JOURNAL) {
			int Section = 0;
			if (tr->Flags&IE_DLG_UNSOLVED) {
				Section |= 1;
			}
			if (tr->Flags&IE_DLG_SOLVED) {
				Section |= 2;
			}
			if (core->GetGame()->AddJournalEntry(tr->journalStrRef, sectionMap[Section], tr->Flags>>16) ) {
				core->DisplayConstantString(STR_JOURNALCHANGE,0xffff00);
				char *string = core->GetString( tr->journalStrRef );
				//cutting off the strings at the first crlf
				char *poi = strchr(string,'\n');
				if (poi) {
					*poi='\0';
				}
				core->DisplayString( string );
				free( string );
			}
		}

		if (tr->textStrRef != 0xffffffff) {
			//allow_zero is for PST (deionarra's text)
			core->DisplayStringName( (int) (tr->textStrRef), 0x8080FF, speaker, IE_STR_SOUND|IE_STR_SPEECH|IE_STR_ALLOW_ZERO);
			if (core->HasFeature( GF_DIALOGUE_SCROLLS )) {
				ta->AppendText( "", -1 );
			}
		}

		if (tr->action) {
			for (unsigned int i = 0; i < tr->action->count; i++) {
				Action* action = GenerateAction( tr->action->strings[i]);
				if (action) {
					target->AddAction( action );
				} else {
					snprintf(Tmp, sizeof(Tmp),
						"Can't compile action: %s\n",
						tr->action->strings[i] );
					printMessage( "Dialog", Tmp,YELLOW);
				}
			}
		}

		if (tr->Flags & IE_DLG_TR_FINAL) {
			ta->SetMinRow( false );
			EndDialog();
			return;
		}

		// moved immediate execution of dialog actions here
		if (DialogueFlags & DF_FREEZE_SCRIPTS) {
			target->ProcessActions(true);
			//clear queued actions that remained stacked?
			target->ClearActions();
		}

		//displaying dialog for selected option
		int si = tr->stateIndex;
		//follow external linkage, if required
		if (tr->Dialog[0] && strnicmp( tr->Dialog, dlg->ResRef, 8 )) {
			//target should be recalculated!
			tgt = target->GetCurrentArea()->GetActorByDialog(tr->Dialog);
			target = tgt;
			if (!target) {
				printMessage("Dialog","Can't redirect dialog\n",YELLOW);
				ta->SetMinRow( false );
				EndDialog();
				return;
			}
			targetID = tgt->globalID;
			// we have to make a backup, tr->Dialog is freed
			ieResRef tmpresref;
			strnlwrcpy(tmpresref,tr->Dialog, 8);
			InitDialog( speaker, target, tmpresref );
		}
		ds = dlg->GetState( si );
	}
	//displaying npc text
	core->DisplayStringName( ds->StrRef, 0x70FF70, target, IE_STR_SOUND|IE_STR_SPEECH);
	//adding a gap between options and npc text
	ta->AppendText("",-1);
	int i;
	int idx = 0;
	ta->SetMinRow( true );
	//first looking for a 'continue' opportunity, the order is descending (a la IE)
	unsigned int x = ds->transitionsCount;
	while(x--) {
		if (ds->transitions[x]->Flags & IE_DLG_TR_FINAL) {
			continue;
		}
		if (ds->transitions[x]->textStrRef != 0xffffffff) {
			continue;
		}
		if (ds->transitions[x]->Flags & IE_DLG_TR_TRIGGER) {
			if (!dlg->EvaluateDialogTrigger(target, ds->transitions[x]->trigger)) {
				continue;
			}
		}
		core->GetDictionary()->SetAt("DialogOption",x);
		DialogueFlags |= DF_OPENCONTINUEWINDOW;
		goto end_of_choose;
	}
	for (x = 0; x < ds->transitionsCount; x++) {
		if (ds->transitions[x]->Flags & IE_DLG_TR_TRIGGER) {
			if (!dlg->EvaluateDialogTrigger(target, ds->transitions[x]->trigger)) {
				continue;
			}
		}
		idx++;
		if (ds->transitions[x]->textStrRef == 0xffffffff) {
			//dialogchoose should be set to x
			//it isn't important which END option was chosen, as it ends
			core->GetDictionary()->SetAt("DialogOption",x);
			DialogueFlags |= DF_OPENENDWINDOW;
		} else {
			char *string = ( char * ) malloc( 40 );
			sprintf( string, "[s=%d,ffffff,ff0000]%d - [p]", x, idx );
			i = ta->AppendText( string, -1 );
			free( string );
			string = core->GetString( ds->transitions[x]->textStrRef );
			ta->AppendText( string, i );
			free( string );
			ta->AppendText( "[/p][/s]", i );
		}
	}
	// this happens if a trigger isn't implemented or the dialog is wrong
	if (!idx) {
		printMessage("Dialog", "There were no valid dialog options!\n", YELLOW);
		DialogueFlags |= DF_OPENENDWINDOW;
	}
end_of_choose:
	//padding the rows so our text will be at the top
	if (core->HasFeature( GF_DIALOGUE_SCROLLS )) {
		ta->AppendText( "", -1 );
	}
	else {
		ta->PadMinRow();
	}
	// is this correct?
	/* this was moved upwards before evaluating the triggers
	if (DialogueFlags & DF_FREEZE_SCRIPTS) {
		target->ProcessActions(true);
		//clear queued actions that remained stacked?
		target->ClearActions();
	}
	*/
}

void GameControl::DisplayString(Point &p, const char *Text)
{
	Scriptable* scr = new Scriptable( ST_TRIGGER );
	scr->overHeadText = (char *) Text;
	scr->textDisplaying = 1;
	scr->timeStartDisplaying = 0;
	scr->Pos = p;
	scr->ClearCutsceneID( );
	//infoTexts.push_back( scr );
}

void GameControl::DisplayString(Scriptable* target)
{
	Scriptable* scr = new Scriptable( ST_TRIGGER );
	size_t len = strlen( target->overHeadText ) + 1;
	scr->overHeadText = ( char * ) malloc( len );
	strcpy( scr->overHeadText, target->overHeadText );
	scr->textDisplaying = 1;
	scr->timeStartDisplaying = target->timeStartDisplaying;
	scr->Pos = target->Pos;
	scr->SetCutsceneID( target );
}

/** changes displayed map to the currently selected PC */
void GameControl::ChangeMap(Actor *pc, bool forced)
{
	//swap in the area of the actor
	Game* game = core->GetGame();
	if (forced || (stricmp( pc->Area, game->CurrentArea) != 0) ) {
		EndDialog();
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
	Region vp = core->GetVideoDriver()->GetViewport();
	if (ScreenFlags&SF_CENTERONACTOR) {
		core->timer->SetMoveViewPort( pc->Pos.x, pc->Pos.y, 0, true );
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

Sprite2D* GameControl::GetScreenshot(bool show_gui)
{
	Sprite2D* screenshot;
	if (show_gui) {
		screenshot = core->GetVideoDriver()->GetScreenshot( Region( 0, 0, 0, 0) );
	} else {
		HideGUI ();
		Draw (0, 0);
		screenshot = core->GetVideoDriver()->GetScreenshot( Region( 0, 0, 0, 0 ) );
		UnhideGUI ();
		core->DrawWindows ();
	}

	return screenshot;
}


Sprite2D* GameControl::GetPreview()
{
	// We get preview by first taking a screenshot of size 640x405,
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
		w = 640;
	}

	if (y < 0) {
		y = 0;
	} else {
		h = 405;
	}

	if (!x)
		y = 0;

	HideGUI ();
	signed char v = Owner->Visible;
	Owner->Visible = WINDOW_VISIBLE;
	Draw (0, 0);
	Owner->Visible = v;
	Sprite2D *screenshot = video->GetScreenshot( Region(x, y, w, h) );
	UnhideGUI ();
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
	DataStream *str = core->GetResourceMgr()->GetResource( actor->GetPortrait( true ), IE_BMP_CLASS_ID );
	if (! str) {
		return NULL;
	}

	ImageMgr *im = (ImageMgr *) core->GetInterface( IE_BMP_CLASS_ID );
	im->Open( str, true );
	Sprite2D* img = im->GetImage();
	core->FreeInterface(im);

	if (ratio == 1)
		return img;

	Sprite2D* img_scaled = video->SpriteScaleDown( img, ratio );
	video->FreeSprite( img );

	return img_scaled;
}

Actor *GameControl::GetActorByGlobalID(ieWord ID)
{
	if (!ID)
		return NULL;
	Game* game = core->GetGame();
	if (!game)
		return NULL;

	Map* area = game->GetCurrentArea( );
	if (!area)
		return NULL;
	return
		area->GetActorByGlobalID(ID);
}

Actor *GameControl::GetLastActor()
{
	return GetActorByGlobalID(lastActorID);
}

Actor *GameControl::GetTarget()
{
	return GetActorByGlobalID(targetID);
}

Actor *GameControl::GetSpeaker()
{
	return GetActorByGlobalID(speakerID);
}

void GameControl::SetupItemUse(int slot, int header, Actor *u, int targettype, int cnt)
{
	spellOrItem = -1;
	spellUser = u;
	spellSlot = slot;
	spellIndex = header;
	//item use also uses the casting icon, this might be changed
	target_mode = targettype|TARGET_MODE_CAST;
	spellCount = cnt;
}

void GameControl::SetupCasting(int type, int level, int idx, Actor *u, int targettype, int cnt)
{
	spellOrItem = type;
	spellUser = u;
	spellSlot = level;
	spellIndex = idx;
	target_mode = targettype|TARGET_MODE_CAST;
	spellCount = cnt;
}

bool GameControl::SetEvent(int /*eventType*/, const char * /*handler*/)
{
	return false;
}
