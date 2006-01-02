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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/GameControl.cpp,v 1.278 2006/01/02 23:26:54 avenger_teambg Exp $
 */

#ifndef WIN32
#include <sys/time.h>
#endif
#include "../../includes/win32def.h"
#include "GameControl.h"
#include "Interface.h"
#include "AnimationMgr.h"
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
#define DEBUG_XXX		0x10

static Color cyan = {
	0x00, 0xff, 0xff, 0xff
};
static Color red = {
	0xff, 0x00, 0x00, 0xff
};
static Color magenta = {
	0xff, 0x00, 0xff, 0xff
};
static Color green = {
	0x00, 0xff, 0x00, 0xff
};
static Color white = {
	0xff, 0xff, 0xff, 0xff
};
static Color black = {
	0x00, 0x00, 0x00, 0xff
};
static Color blue = {
	0x00, 0x00, 0xff, 0x80
};

Animation* effect;

#define FORMATIONSIZE 10
typedef Point formation_type[FORMATIONSIZE];
int formationcount;
static formation_type *formations=NULL;
static bool mqs = false;

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
	action = GA_DEFAULT | GA_SELECT | GA_NO_DEAD;
	Changed = true;
	lastActorID = 0;
	MouseIsDown = false;
	DrawSelectionRect = false;
	overDoor = NULL;
	overContainer = NULL;
	overInfoPoint = NULL;
	drawPath = NULL;
	pfs.x = 0;
	pfs.y = 0;
	InfoTextPalette = core->GetVideoDriver()->CreatePalette( white, black );
	lastCursor = IE_CURSOR_NORMAL;
	moveX = moveY = 0;
	DebugFlags = 0;
	AIUpdateCounter = 1;
	effect = NULL;
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
}

//actually the savegame contains some formation data too, how to use it?
void GameControl::ReadFormations()
{
	int i,j;
	TableMgr * tab;
	int table=core->LoadTable("formatio");
	if (table<0) {
		goto fallback;
	}
 	tab = core->GetTable( table);
	if (!tab) {
		core->DelTable(table);
		goto fallback;
	}
	formationcount = tab->GetRowCount();
	formations = (formation_type *) calloc(formationcount, sizeof(formation_type));
	for(i=0; i<formationcount; i++) {
		for(j=0;j<FORMATIONSIZE;j++) {
			int k=atoi(tab->QueryField(i,j*2));
			formations[i][j].x=k;
			k=atoi(tab->QueryField(i,j*2+1));
			formations[i][j].y=k;
		}
	}
//
// read in formation data
//
	core->DelTable(table);
	return;
fallback:
	formationcount = 1;
	formations = (formation_type *) calloc(1,sizeof(formation_type) );
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
	free( InfoTextPalette );
	for (unsigned int i = 0; i < infoTexts.size(); i++) {
		delete( infoTexts[i] );
	}
	if (dlg) {
		delete dlg;
	}
}

void GameControl::QuickSave()
{
	const char *folder;

	int SlotTable = core->LoadTable( "savegame" );
	if (SlotTable >= 0) {
	        TableMgr* tab = core->GetTable( SlotTable );
                folder = tab->QueryField(1);
		core->GetSaveGameIterator()->CreateSaveGame(1, folder, mqs == 1);
		if (SlotTable >= 0) {
		        core->DelTable(SlotTable);
		}
	}
}

/* changes the textcolor */
void GameControl::SetInfoTextColor(Color color)
{
	free( InfoTextPalette ); //it always exists, see constructor
	InfoTextPalette = core->GetVideoDriver()->CreatePalette( color, black );
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
	if ( game->selected.size() > 0 ) {
		ChangeMap(core->GetFirstSelectedPC(), false);
	}
	Video* video = core->GetVideoDriver();
	Region viewport = core->GetVideoDriver()->GetViewport();
	viewport.x += video->moveX;
	viewport.y += video->moveY;
	core->MoveViewportTo( viewport.x, viewport.y, false );
	Region screen( x + XPos, y + YPos, Width, Height );
	Map* area = game->GetCurrentArea( );
	if (!area) {
		core->GetVideoDriver()->DrawRect( screen, blue, true );
		return;
	}
	core->GetVideoDriver()->DrawRect( screen, black, true );
	area->DrawMap( screen, this );

	//in multi player (if we ever get to it), only the server must call this
	if (update_scripts) {
		// the game object will run the area scripts as well
		game->UpdateScripts();
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

	// Show doors
	if (DebugFlags & DEBUG_SHOW_DOORS) {
		Door* d;
		//there is a real assignment in the loop!
		for (unsigned int idx = 0;
			(d = area->TMap->GetDoor( idx ));
			idx++) {
			if (d->TrapFlags) {
				d->outlineColor = red;
			} else {
				if (d->Flags &DOOR_SECRET) {
					if (d->Flags & DOOR_FOUND) {
						d->outlineColor = magenta;
					} else {
						//secret door is invisible
						continue;
					}
				}
				d->outlineColor = cyan;
			}
			d->DrawOutline();
		}
	}

	// Show containers
	if (DebugFlags & DEBUG_SHOW_CONTAINERS) {
		Container* c;

		//there is a real assignment in the loop!
		for (unsigned int idx = 0;
			(c = area->TMap->GetContainer( idx ));
			idx++) {
			if (c->TrapDetected && c->Trapped) {
				c->outlineColor = red;
			} else {
				c->outlineColor = cyan;
			}
			c->DrawOutline();			
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

			core->GetVideoDriver()->DrawPolyline( poly, c, true );
		}
	}


	//Draw spell effect, this must be stored in the actors
	//not like this
	if (effect) {
		if (( game->selected.size() > 0 )) {
			Actor* actor = core->GetFirstSelectedPC();
			video->BlitSprite( effect->NextFrame(), actor->Pos.x,
					actor->Pos.y, false );
		}
	}

	// Show traps
	if (DebugFlags & DEBUG_SHOW_INFOPOINTS) {
		//draw infopoints with blue overlay
		InfoPoint* i;
		//there is a real assignment in the loop!
		for (unsigned int idx = 0; (i = area->TMap->GetInfoPoint( idx )); idx++) {
			if (i->VisibleTrap( 1 ) ) {
				i->outlineColor = red; //traps
				i->DrawOutline();
			} else {
				i->outlineColor = blue; //infopoints
				i->DrawOutline();
			}
		}
	} else {
		InfoPoint* i;
		for (unsigned int idx = 0; (i = area->TMap->GetInfoPoint( idx )); idx++) {
			if (i->VisibleTrap( 0 ) ) {
				i->outlineColor = red; //traps
				i->DrawOutline();
			}
		}
	}

	// Draw infopoint texts for some time
	for (size_t i = 0; i < infoTexts.size(); i++) {
		unsigned long time;
		GetTime( time );
		if (( time - infoTexts[i]->timeStartDisplaying ) >= 10000) {
			infoTexts[i]->textDisplaying = 0;
			std::vector< Scriptable*>::iterator m;
			m = infoTexts.begin() + i;
			( *m )->GetCutsceneID()->textDisplaying = 0;
			delete( *m );
			infoTexts.erase( m );
			i--;
			continue;
		}
		if (infoTexts[i]->textDisplaying == 1) {
			Font* font = core->GetFont( 1 );
			Region rgn( infoTexts[i]->Pos.x - 200,
				infoTexts[i]->Pos.y - 100, 400, 400 );
			//printf("Printing InfoText at [%d,%d,%d,%d]\n", rgn.x, rgn.y, rgn.w, rgn.h);
			rgn.x += video->xCorr;
			rgn.y += video->yCorr;
			font->Print( rgn,
					( unsigned char * ) infoTexts[i]->overHeadText,
					InfoTextPalette,
					IE_FONT_ALIGN_CENTER | IE_FONT_ALIGN_TOP, false );
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
	core->GetGame()->SetHotKey(toupper(Key));
	switch (Key) {
	case ' ':
		DialogueFlags ^= DF_FREEZE_SCRIPTS;
		if (DialogueFlags&DF_FREEZE_SCRIPTS) {
			core->DisplayConstantString(STR_PAUSED,0xff0000);
		} else {
			core->DisplayConstantString(STR_UNPAUSED,0xff0000);
		}
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

/** Key Release Event */
void GameControl::OnKeyRelease(unsigned char Key, unsigned short Mod)
{
	unsigned int i;

	Game* game = core->GetGame();

	if (!game)
		return;

	switch (Key) {
		case 'q':
			QuickSave();
			break;
		case GEM_ALT:
			DebugFlags &= ~DEBUG_SHOW_CONTAINERS;
			break;
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
		case '\t':
			//not GEM_TAB
			DebugFlags &= ~DEBUG_XXX;
			printf( "TAB released\n" );
			return;
		default:
			break;
	}
	if (!core->CheatEnabled()) {
		return;
	}
	if (Mod & GEM_MOD_CTRL) //ctrl
	{
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
			case 'b':
				if (game->selected.size() > 0) {
					if (!effect) {
						AnimationFactory* af = ( AnimationFactory* )
		 				core->GetResourceMgr()->GetFactoryResource( "S056ICBL", IE_BAM_CLASS_ID );

						effect = af->GetCycle( 1 );
					} else {
						delete( effect );
						effect = NULL;
					}
				}
				break;

			case 'c':
				if (game->selected.size() > 0 && lastActor) {
					Actor *src = game->selected[0];
					bool res = src->spellbook.CastSpell( "SPWI207", src, lastActor );
					printf( "Cast Spell: %d\n", res );
				}
				break;

			case 'p':
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
				drawPath = core->GetGame()->GetCurrentArea()->FindPath( pfs, p );

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
			case 'r'://resurrects actor
				if (!lastActor) {
					lastActor = area->GetActor( p, GA_DEFAULT);
				}
				if (lastActor) {
					lastActor->Resurrect();
				}
				break;
			case 't'://advances time
				// 7200 (one day) /24 (hours) == 300
				game->GameTime += 300;
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
			case 'w': //removes actor
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
				if (! lastActor)
					break;

				for (i = 0; i < 10; i++) {
					CREItem* ci = lastActor->inventory.GetSlotItem (i);
					if (! ci)
						continue;

					Item* item = core->GetItem(ci->ItemResRef);
					if (item) {
						for (int f = 0; f < item->EquippingFeatureCount; f++) {
							AddEffect( item->equipping_features+f, lastActor, lastActor );
						}
						core->FreeItem( item, false );
					}
				}
				lastActor->fxqueue.ApplyAllEffects( lastActor );
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
			default:
				printf( "KeyRelease:%d - %d\n", Key, Mod );
				break;
		}
	}
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
	Map* area = game->GetCurrentArea( );

	int nextCursor = area->GetCursor( p );

	overInfoPoint = area->TMap->GetInfoPoint( p );
	if (overInfoPoint) {
		if (overInfoPoint->Type != ST_PROXIMITY) {
			nextCursor = overInfoPoint->Cursor;
		}
	}

	if (overDoor) {
		overDoor->Highlight = false;
	}
	overDoor = area->TMap->GetDoor( p );
	if (overDoor) {
		overDoor->Highlight = true;
		if (overDoor->Flags & DOOR_LOCKED) {
			nextCursor = IE_CURSOR_LOCK;
		} else {
			nextCursor = overDoor->Cursor;
		}
		overDoor->outlineColor = cyan;
	}

	if (overContainer) {
		overContainer->Highlight = false;
	}
	overContainer = area->TMap->GetContainer( p );
	if (overContainer) {
		overContainer->Highlight = true;
		if (overContainer->TrapDetected && overContainer->Trapped) {
			nextCursor = IE_CURSOR_TRAP;
			overContainer->outlineColor = red;
		} else {
			if (overContainer->Flags & CONT_LOCKED) {
				nextCursor = IE_CURSOR_LOCK2;
			} else if (overContainer->Type==IE_CONTAINER_PILE) {
				nextCursor = IE_CURSOR_TAKE;
			} else {
				nextCursor = IE_CURSOR_CHEST;
			}
			overContainer->outlineColor = cyan;
		}
	}

	if (!DrawSelectionRect) {
		Actor* actor = area->GetActor( p, action);
		Actor *lastActor = area->GetActorByGlobalID(lastActorID);
		if (lastActor)
			lastActor->SetOver( false );
		if (actor) {
			lastActorID = actor->globalID;
			actor->SetOver( true );
			ieDword type = actor->GetStat(IE_EA);
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
		}

		
		if (actor) {
			switch (actor->GetStat(IE_EA)) {
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
	if (lastCursor != nextCursor) {
		( ( Window * ) Owner )->Cursor = nextCursor;
		lastCursor = nextCursor;
	}
}

void GameControl::TryToAttack(Actor *source, Actor *tgt)
{
	char Tmp[40];
	ieWord tmp;

	//this won't work atm, target must be honoured by Attack
	source->ClearPath();
	source->ClearActions();
	strncpy(Tmp,"NIDSpecial3()",sizeof(Tmp) );
	tmp = targetID;
	targetID=tgt->globalID; //this is a hack, not deadly, but a hack
	source->AddAction( GenerateAction( Tmp) );
	//we restore the old target ID, because this variable is primarily
	//to keep track of the target of a dialog, and attacking isn't talking
	targetID = tmp;
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
	targetID=tgt->globalID; //this is a hack, but not so deadly
	source->AddAction( GenerateAction( Tmp) );
}

void GameControl::HandleContainer(Container *container, Actor *actor)
{
	char Tmp[256];

	actor->ClearPath();
	actor->ClearActions();
	strncpy(Tmp,"UseContainer()",sizeof(Tmp) );
	core->SetCurrentContainer( actor, container);
	actor->AddAction( GenerateAction( Tmp) );
}

void GameControl::HandleDoor(Door *door, Actor *actor)
{
	char Tmp[256];

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
					trap->Scripts[0]->Update();
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
						GetTime( trap->timeStartDisplaying );
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
void GameControl::OnMouseDown(unsigned short x, unsigned short y,
	unsigned char Button, unsigned short /*Mod*/)
{
	if ((ScreenFlags&SF_DISABLEMOUSE) || (Button != GEM_MB_ACTION) ) {
		return;
	}
	Point p(x,y);
	core->GetVideoDriver()->ConvertToGame( p.x, p.y );
	MouseIsDown = true;
	SelectionRect.x = p.x;
	SelectionRect.y = p.y;
	StartX = p.x;
	StartY = p.y;
	SelectionRect.w = 0;
	SelectionRect.h = 0;
}
/** Mouse Button Up */
void GameControl::OnMouseUp(unsigned short x, unsigned short y,
	unsigned char Button, unsigned short /*Mod*/)
{
	unsigned int i;
	char Tmp[256];

	if (ScreenFlags & SF_DISABLEMOUSE) {
		return;
	}
	//heh, i found no better place
	core->CloseCurrentContainer();
	if (Button == GEM_MB_MENU) {
		core->GetDictionary()->SetAt( "MenuX", x );
		core->GetDictionary()->SetAt( "MenuY", y );
		core->GetGUIScriptEngine()->RunFunction( "OpenFloatMenuWindow" );
		return;
	}
	if (Button != GEM_MB_ACTION) {
		return;
	}

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
	Actor* actor = area->GetActor( p, action );

	if (!actor && ( game->selected.size() > 0 )) {
		if (overDoor) {
			HandleDoor(overDoor, core->GetFirstSelectedPC());
			return;
		}
		if (overInfoPoint) {
			if (HandleActiveRegion(overInfoPoint, core->GetFirstSelectedPC(), p)) {
				return;
			}
		}
		if (overContainer) {
			HandleContainer(overContainer, core->GetFirstSelectedPC());
			return;
		}

		//just a single actor, no formation
		if (game->selected.size()==1) {
			actor=game->selected[0];
			actor->ClearPath();
			actor->ClearActions();
			sprintf( Tmp, "MoveToPoint([%d.%d])", p.x, p.y );
			actor->AddAction( GenerateAction( Tmp) );
			//we clicked over a searchmap travel region
				if ( ( ( Window * ) Owner )->Cursor == IE_CURSOR_TRAVEL) {
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
		//we clicked over a searchmap travel region
		if ( ( ( Window * ) Owner )->Cursor == IE_CURSOR_TRAVEL) {
			sprintf( Tmp, "NIDSpecial2()" );
			actor->AddAction( GenerateAction( Tmp) );
		}
		return;
	}
	if (!actor) return;
	//we got an actor past this point

	//determining the type of the clicked actor
	ieDword type;

	type = actor->GetStat(IE_EA);
	if ( type >= EA_EVILCUTOFF ) {
		type = 2; //hostile
	} else if ( type > EA_CHARMED ) {
		type = 1; //neutral
	} else {
		type = 0; //party
	}
	
	if (target_mode&TARGET_MODE_ATTACK) {
		type = 2;
	} else if (target_mode&TARGET_MODE_TALK) {
		type = 1;
	} else if (target_mode&TARGET_MODE_CAST) {
		type = 3;
	}

	target_mode = TARGET_MODE_NONE;

	switch (type) {
		case 0:
			//clicked on a new party member
			// FIXME: call GameControl::SelectActor() instead
			//game->SelectActor( actor, true, SELECT_REPLACE );
			SelectActor( game->InParty(actor) );
			break;
		case 1:
			//talk (first selected talks)
			if (game->selected.size()) {
				//if we are in PST modify this to NO!
				Actor *source;
				if (core->HasFeature(GF_PROTAGONIST_TALKS) ) {
					source = game->FindPC(1); //protagonist
				} else {
					source = core->GetFirstSelectedPC();
				}
				TryToTalk(source, actor);
			}
			break;
		case 2:
			//all of them attacks the red circled actor
			for(i=0;i<game->selected.size();i++) {
				TryToAttack(game->selected[i], actor);
			}
			break;
		case 3: //cast on target
			break;
	}
}
/** Special Key Press */
void GameControl::OnSpecialKeyPress(unsigned char Key)
{
	if (DialogueFlags&DF_IN_DIALOG) {
		return; //don't accept keys in dialog
	}
	Region Viewport = core->GetVideoDriver()->GetViewport();
	switch (Key) {
		case GEM_LEFT:
			if (Viewport.x > 63)
				Viewport.x -= 64;
			break;
		case GEM_UP:
			if (Viewport.y > 63)
				Viewport.y -= 64;
			break;
		case GEM_DOWN:
			if (Viewport.y < 32000)
				Viewport.y += 64;
			break;
		case GEM_RIGHT:
			if (Viewport.x < 32000)
				Viewport.x += 64;
			break;
		case GEM_ALT:
			DebugFlags |= DEBUG_SHOW_CONTAINERS;
			break;
		case GEM_TAB:
			DebugFlags |= DEBUG_XXX;
			printf( "TAB pressed\n" );
			break;
		case GEM_MOUSEOUT:
			moveX = 0;
			moveY = 0;
			break;
	}
	if (ScreenFlags & SF_LOCKSCROLL) {
		moveX = 0;
		moveY = 0;
	}
	else {
		core->MoveViewportTo( Viewport.x, Viewport.y, false );
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
		if (count != 0) {
			for (i = 0; i < highlighted.size(); i++)
				highlighted[i]->SetOver( false );
			highlighted.clear();
			for (i = 0; i < count; i++) {
				ab[i]->SetOver( true );
				highlighted.push_back( ab[i] );
			}
		}
		free( ab );
	} else {
		Actor* actor = area->GetActor( p, action);
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
		ScreenFlags |= (SF_DISABLEMOUSE | SF_LOCKSCROLL);
	} else {
		ScreenFlags &= ~(SF_DISABLEMOUSE | SF_LOCKSCROLL);
	}
}

void GameControl::HandleWindowHide(const char *WindowName, const char *WindowPosition)
{
	Variables* dict = core->GetDictionary();
	ieDword index;

	if (dict->Lookup( WindowName, index )) {
		if (index != (ieDword) -1) {
			Window* w = core->GetWindow( index );
			if (w) {
				core->SetVisible( index, WINDOW_INVISIBLE );
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
	if (core->GetVisible(0) == 0 ) {
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
			core->SetVisible( index, WINDOW_INVISIBLE );
		}
	}
	core->GetVideoDriver()->SetViewport( ( ( Window * ) Owner )->XPos, ( ( Window * ) Owner )->YPos, Width, Height );
	return 1;
}


void GameControl::HandleWindowReveal(const char *WindowName, const char *WindowPosition)
{
	Variables* dict = core->GetDictionary();
	ieDword index;

	if (dict->Lookup( WindowName, index )) {
		if (index != (ieDword) -1) {
			Window* w = core->GetWindow( index );
			if (w) {
				core->SetVisible( index, WINDOW_VISIBLE );
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
			Window* fw = core->GetWindow( index );
			core->SetVisible( index, WINDOW_VISIBLE );
			fw->Flags |=WF_FLOAT;
			core->SetOnTop( index );
		}
	}
	core->GetVideoDriver()->SetViewport( ( ( Window * ) Owner )->XPos, ( ( Window * ) Owner )->YPos, Width, Height );
	return 1;
}

void GameControl::ResizeDel(Window* win, unsigned char type)
{
	switch (type) {
	case 0: //Left
		if (LeftCount!=1) {
			printMessage("GameControl","More than one left window!\n",LIGHT_RED);
		}
		LeftCount--;
		if (!LeftCount) {
			( ( Window * ) Owner )->XPos -= win->Width;
			( ( Window * ) Owner )->Width += win->Width;
			Width = ( ( Window * ) Owner )->Width;
		}
		break;

	case 1: //Bottom
		if (BottomCount!=1) {
			printMessage("GameControl","More than one bottom window!\n",LIGHT_RED);
		}
		BottomCount--;
		if (!BottomCount) {
			( ( Window * ) Owner )->Height += win->Height;
			Height = ( ( Window * ) Owner )->Height;
		}
		break;

	case 2: //Right
		if (RightCount!=1) {
			printMessage("GameControl","More than one right window!\n",LIGHT_RED);
		}
		RightCount--;
		if (!RightCount) {
			( ( Window * ) Owner )->Width += win->Width;
			Width = ( ( Window * ) Owner )->Width;
		}
		break;

	case 3: //Top
		if (TopCount!=1) {
			printMessage("GameControl","More than one top window!\n",LIGHT_RED);
		}
		TopCount--;
		if (!TopCount) {
			( ( Window * ) Owner )->YPos -= win->Height;
			( ( Window * ) Owner )->Height += win->Height;
			Height = ( ( Window * ) Owner )->Height;
		}
		break;

	case 4: //BottomAdded
		BottomCount--;
		( ( Window * ) Owner )->Height += win->Height;
		Height = ( ( Window * ) Owner )->Height;
		break;
	case 5: //Inactivating
		BottomCount--;
		( ( Window * ) Owner )->Height += win->Height;
		Height = ( ( Window * ) Owner )->Height;
		break;
	}
}

void GameControl::ResizeAdd(Window* win, unsigned char type)
{
	switch (type) {
	case 0: //Left
		LeftCount++;
		if (LeftCount == 1) {
			( ( Window * ) Owner )->XPos += win->Width;
			( ( Window * ) Owner )->Width -= win->Width;
			Width = ( ( Window * ) Owner )->Width;
		}
		break;

	case 1: //Bottom
		BottomCount++;
		if (BottomCount == 1) {
			( ( Window * ) Owner )->Height -= win->Height;
			Height = ( ( Window * ) Owner )->Height;
		}
		break;

	case 2: //Right
		RightCount++;
		if (RightCount == 1) {
			( ( Window * ) Owner )->Width -= win->Width;
			Width = ( ( Window * ) Owner )->Width;
		}
		break;

	case 3: //Top
		TopCount++;
		if (TopCount == 1) {
			( ( Window * ) Owner )->YPos += win->Height;
			( ( Window * ) Owner )->Height -= win->Height;
			Height = ( ( Window * ) Owner )->Height;
		}
		break;

	case 4: //BottomAdded
		BottomCount++;
		( ( Window * ) Owner )->Height -= win->Height;
		Height = ( ( Window * ) Owner )->Height;
		break;

	case 5: //Inactivating
		BottomCount++;
		( ( Window * ) Owner )->Height -= win->Height;
		Height = 0;
	}
}

void GameControl::InitDialog(Actor* spk, Actor* tgt, const char* dlgref)
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
		printf( "[GameControl]: Cannot start dialog: %s\n", dlgref );
		return;
	}
	strnlwrcpy(dlg->ResRef, dlgref, 8); //this isn't handled by GetDialog???
	//target is here because it could be changed when a dialog runs onto
	//and external link, we need to find the new target (whose dialog was
	//linked to)
	targetID = tgt->globalID;
	spk->LastTalkedTo=targetID;
	speakerID = spk->globalID;
	tgt->LastTalkedTo=speakerID;

	//check if we are already in dialog
	if (DialogueFlags&DF_IN_DIALOG) {
		return;
	}
	UnhideGUI();
	ScreenFlags |= SF_GUIENABLED|SF_DISABLEMOUSE|SF_CENTERONACTOR|SF_LOCKSCROLL;
	DialogueFlags |= DF_IN_DIALOG;
	//allow mouse selection from dialog (even though screen is locked)
	core->GetVideoDriver()->DisableMouse = false;
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
	if (try_to_break && (DialogueFlags&DF_UNBREAKABLE) )
	{
		return;
	}

	/* I'm not convinced we should call these, but it is possible
	Actor *target = GetTarget();
	Actor *speaker = GetSpeaker();
	if (target) {
		target->ReleaseCurrentAction();
	}
	if (speaker) {
		speaker->ReleaseCurrentAction();
	}
	*/

	speakerID = 0;
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

	Actor *target = GetTarget();
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
		if (DialogueFlags&DF_TALKCOUNT) {
			DialogueFlags&=~DF_TALKCOUNT; 
			target->TalkCount++;
		} else if (DialogueFlags&DF_INTERACT) {
			DialogueFlags&=~DF_INTERACT;
			target->InteractCount++;
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
			if (core->GetGame()->AddJournalEntry(tr->journalStrRef, Section, tr->Flags>>16) ) {
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
			core->DisplayStringName( tr->textStrRef, 0x8080FF, speaker);
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
		int si = tr->stateIndex;
		//follow external linkage, if required
		if (tr->Dialog[0] && strnicmp( tr->Dialog, dlg->ResRef, 8 )) {
			//target should be recalculated!
			target = target->GetCurrentArea()->GetActorByDialog(tr->Dialog);
			if (!target) {
				printMessage("Dialog","Can't redirect dialog",YELLOW);
				ta->SetMinRow( false );
				EndDialog();
				return;
			}
			targetID = target->globalID;
			// we have to make a backup, tr->Dialog is freed
			ieResRef tmpresref;
			strnlwrcpy(tmpresref,tr->Dialog, 8);
			InitDialog( speaker, target, tmpresref );
		}
		ds = dlg->GetState( si );
	}
	//displaying npc text
	core->DisplayStringName( ds->StrRef, 0x70FF70, target );
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
		printMessage("Dialog", "There were no valid dialog options!", YELLOW);
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
	if (DialogueFlags & DF_FREEZE_SCRIPTS) {
		target->ProcessActions();
		//clear queued actions that remained stacked?
		target->ClearActions();
	}
}

void GameControl::DisplayString(Point &p, const char *Text)
{
	Scriptable* scr = new Scriptable( ST_TRIGGER );
	scr->overHeadText = (char *) Text;
	scr->textDisplaying = 1;
	scr->timeStartDisplaying = 0;
	scr->Pos = p;
	scr->SetCutsceneID( NULL );
	infoTexts.push_back( scr );
}

void GameControl::DisplayString(Scriptable* target)
{
	Scriptable* scr = new Scriptable( ST_TRIGGER );
	int len = strlen( target->overHeadText ) + 1;
	scr->overHeadText = ( char * ) malloc( len );
	strcpy( scr->overHeadText, target->overHeadText );
	scr->textDisplaying = 1;
	scr->timeStartDisplaying = target->timeStartDisplaying;
	scr->Pos = target->Pos;
	scr->SetCutsceneID( target );
	infoTexts.push_back( scr );
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
		for (unsigned int i = 0; i < infoTexts.size(); i++) {
			delete( infoTexts[i] );
		}
		infoTexts.clear();
		/*this is loadmap, because we need the index, not the pointer*/
		char *areaname = game->CurrentArea;
		if (pc) {
			areaname = pc->Area;
		}
		Map *map = game->GetMap( areaname, true );
		map->SetupAmbients();
		int Song;
		if (game->CombatCounter) {
			//battlesong
			Song = 3;
		} else {
			//night or day
			Song = game->GameTime%7200/3600;
		}
		map->PlayAreaSong( Song );

		ScreenFlags|=SF_CENTERONACTOR;
	}
	//center on first selected actor
	Region vp = core->GetVideoDriver()->GetViewport();
	if (ScreenFlags&SF_CENTERONACTOR) {
		core->MoveViewportTo( pc->Pos.x, pc->Pos.y, true );
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

Sprite2D* GameControl::GetPreview()
{
	HideGUI ();
	Draw (0, 0);
	Sprite2D* preview = core->GetVideoDriver()->GetPreview( 128, 81 );
	UnhideGUI ();
	core->DrawWindows ();

	return preview;
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
