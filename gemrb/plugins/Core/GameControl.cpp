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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/GameControl.cpp,v 1.211 2005/03/31 13:54:32 avenger_teambg Exp $
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

#define DEBUG_SHOW_INFOPOINTS   0x01
#define DEBUG_SHOW_CONTAINERS   0x02
#define DEBUG_SHOW_DOORS	DEBUG_SHOW_CONTAINERS
#define DEBUG_SHOW_SEARCHMAP    0x04
#define DEBUG_SHOW_PALETTES     0x08
#define DEBUG_XXX	        0x10

static Color cyan = {
	0x00, 0xff, 0xff, 0xff
};
static Color red = {
	0xff, 0x00, 0x00, 0xff
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

static void AddTalk(TextArea* ta, Actor* speaker, const char* speaker_color,
	char* text, const char* text_color)
{
	const char* format = "[color=%s]%s -  [/color][p][color=%s]%s[/color][/p]";
	int newlen = (int)(strlen( format ) + strlen( speaker->GetName(-1) ) +
		strlen( speaker_color ) + strlen( text ) +
		strlen( text_color ) + 1);
	char* newstr = ( char* ) malloc( newlen );
	sprintf( newstr, format, speaker_color, speaker->GetName(-1), text_color,
		text );

	ta->AppendText( newstr, -1 );
	ta->AppendText( "", -1 );

	free( newstr );
}

GameControl::GameControl(void)
{
	if(!formations) {
		ReadFormations();
	}
	//this is the default action, individual actors should have one too
	//at this moment we use only this
	action = GA_DEFAULT | GA_SELECT | GA_NO_DEAD;
	Changed = true;
	lastActor = NULL;
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

	core->GetDictionary()->Lookup("Center",tmp);
	if(tmp) {
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
	target = NULL;
	speaker = NULL;
	HotKey = 0;
}

//actually the savegame contains some formation data too, how to use it?
void GameControl::ReadFormations()
{
	int i,j;
	TableMgr * tab;
	int table=core->LoadTable("formatio");
	if(table<0) {
		goto fallback;
	}
 	tab = core->GetTable( table);
	if(!tab) {
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

	int formation=core->GetGame()->WhichFormation;
	pos=actor->InParty-1; //either this or the actual # of selected actor?
	if(pos>=FORMATIONSIZE) pos=FORMATIONSIZE-1;
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
	actor->AddAction( GameScript::GenerateAction( Tmp, true ) );
}

GameControl::~GameControl(void)
{
	free( InfoTextPalette );
	for (unsigned int i = 0; i < infoTexts.size(); i++) {
		delete( infoTexts[i] );
	}
	if(dlg) {
		delete dlg;
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
	if (game->MapIndex == -1) {
		return;
	}
	if (((short) Width) <=0 || ((short) Height) <= 0) {
		return;
	}
	if ( game->selected.size() > 0 ) {
		ChangeMap(game->selected[0], false);
	}
	Video* video = core->GetVideoDriver();
	Region viewport = core->GetVideoDriver()->GetViewport();
	viewport.x += video->moveX;
	viewport.y += video->moveY;
	core->MoveViewportTo( viewport.x, viewport.y, false );
	Region vp( x + XPos, y + YPos, Width, Height );
	Map* area = game->GetCurrentMap( );
	if (!area) {
		core->GetVideoDriver()->DrawRect( vp, blue, true );
		return;
	}
	core->GetVideoDriver()->DrawRect( vp, black, true );
	//shall we stop globaltimer?
	//core->GSUpdate(update_scripts);
	area->DrawMap( vp, this, update_scripts );
	if (ScreenFlags & SF_DISABLEMOUSE)
		return;
	Point p = {lastMouseX, lastMouseY};
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
			d->DrawOutline();
		}
	}

	// Show containers
	if (DebugFlags & DEBUG_SHOW_CONTAINERS) {
		Container* c;

		DialogueFlags |= DF_FREEZE_SCRIPTS;		//there is a real assignment in the loop!
		for (unsigned int idx = 0;
			(c = area->TMap->GetContainer( idx ));
			idx++) {
			if (c->TrapDetected && c->Trapped) {
				video->DrawPolyline( c->outline, red, true );
			} else {
				video->DrawPolyline( c->outline, cyan, true );
			}
		}
	}

	//Draw spell effect, this must be stored in the actors
	//not like this
	if (effect) {
		if (( game->selected.size() > 0 )) {
			Actor* actor = game->selected[0];
			video->BlitSpriteMode( effect->NextFrame(), actor->Pos.x,
					actor->Pos.y, 1, false );
		}
	}

	// Show traps and containers
	if (DebugFlags & (DEBUG_SHOW_INFOPOINTS | DEBUG_SHOW_CONTAINERS)) {
		//draw infopoints with blue overlay
		InfoPoint* i;
		//there is a real assignment in the loop!
		for (unsigned int idx = 0; (i = area->TMap->GetInfoPoint( idx )); idx++) {
			if (i->VisibleTrap( DebugFlags & DEBUG_SHOW_INFOPOINTS ) ) {
				video->DrawPolyline( i->outline, red, true );
			} else if (DebugFlags & DEBUG_SHOW_INFOPOINTS) {
				video->DrawPolyline( i->outline, blue, true );
			}
		}
	} else if (overInfoPoint) {
		if (overInfoPoint->VisibleTrap(0) ) {
			video->DrawPolyline( overInfoPoint->outline, red, true );
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
			( *m )->MySelf->textDisplaying = 0;
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
			Point p = { ( node-> x*16) + 8, ( node->y*12 ) + 6 };
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
		Region point( p.x / 16, p.y / 12, 1, 1 );
		video->DrawRect( point, red );
	}

/*
	// Draw actor's palettes
	if ((DebugFlags & DEBUG_SHOW_PALETTES) && lastActor) {
		int i;

		Color *Pal1 = lastActor->anims->OrigPalette;
		for (i = 0; i < 256; i++) {
			Region rgn( 10 * (i % 64), 30 + 10 * (i / 64), 10, 10 );
			video->DrawRect( rgn, Pal1[i], true, false);
		}
		Color *Pal2 = lastActor->anims->Palette;
		for (i = 0; i < 256; i++) {
			Region rgn( 10 * (i % 64), 90 + 10 * (i / 64), 10, 10 );
			video->DrawRect( rgn, Pal2[i], true, false);
		}
	}
*/
}
/** Sets the Text of the current control, unused here */
int GameControl::SetText(const char* /*string*/, int /*pos*/)
{
	return 0;
}
/** Key Press Event */
void GameControl::OnKeyPress(unsigned char Key, unsigned short /*Mod*/)
{
	HotKey=toupper(Key);
}

void GameControl::DeselectAll()
{
	core->GetGame()->SelectActor( NULL, false, SELECT_NORMAL );
}

void GameControl::SelectActor(int whom)
{
	Game* game = core->GetGame();
	if (whom==-1) {
		game->SelectActor( NULL, true, SELECT_NORMAL );
		return;
	}

	/* doesn't fall through here */
	Actor* actor = game->GetPC( whom );
	if (!actor) 
		return;

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

	switch (Key) {
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
		switch (Key) {
			case 'd': //disarm ?
				if (overInfoPoint) {
					overInfoPoint->DetectTrap(256);
					core->GetVideoDriver()->FreeSprite( overInfoPoint->outline->fill );
					overInfoPoint->outline->fill = NULL;
				}
				if (overContainer) {
					if (overContainer->Trapped &&
						!( overContainer->TrapDetected )) {
						overContainer->TrapDetected = 1;
						core->GetVideoDriver()->FreeSprite( overContainer->outline->fill );
						overContainer->outline->fill = NULL;
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

			case 'p':
				//path
				 {
					Point p = {lastMouseX, lastMouseY};
					core->GetVideoDriver()->ConvertToGame( p.x, p.y );
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
					drawPath = core->GetGame()->GetCurrentMap()->FindPath( pfs, p );

				}
				break;

			case 'o': 
				// origin
				pfs.x = lastMouseX; 
				pfs.y = lastMouseY;
				core->GetVideoDriver()->ConvertToGame( pfs.x, pfs.y );
				break;
			case 'a':
				//animation
				if (lastActor)
					lastActor->GetNextAnimation();
				break;
			case 's':
				//stance
				if (lastActor)
					lastActor->GetNextStance();
				break;
			case 'j':
				// jump
				for (i = 0; i < game->selected.size(); i++) {
					Actor* actor = game->selected[i];
					Point c = {lastMouseX, lastMouseY};
					core->GetVideoDriver()->ConvertToGame( c.x, c.y );
					GameScript::MoveBetweenAreasCore(actor, core->GetGame()->CurrentArea, c, -1, true);
					printf( "Teleported to %d, %d\n", c.x, c.y );
				}
				break;

			case 'm':
				// 'm' ? debugdump
				if (lastActor) {
					lastActor->DebugDump();
					return;
				}
				if (overDoor) {
					overDoor->DebugDump();
					return;
				}
				if (overContainer) {
					overContainer->DebugDump();
					return;
				}
				if (overInfoPoint) {
					overInfoPoint->DebugDump();
					return;
				}
				core->GetGame()->GetCurrentMap()->DebugDump();
				break;
			case 'v':
				// explore map from point
				 {
					Map* area = game->GetCurrentMap( );
					Point p = {lastMouseX, lastMouseY};
					core->GetVideoDriver()->ConvertToGame( p.x, p.y );
					area->ExploreMapChunk( p, rand()%30, true );
				}
				break;
			case 'x':
				// shows coordinates
				 {
					Map* area = game->GetCurrentMap( );
					short cX = lastMouseX; 
					short cY = lastMouseY;
					core->GetVideoDriver()->ConvertToGame( cX, cY );
					printf( "%s [%d.%d]\n", area->GetScriptName(), cX, cY );
				}
				break;
			case 'y': //kills actor
				if (lastActor) {
					//using action so the actor is killed
					//correctly (synchronisation)
					lastActor->ClearActions();
					char Tmp[40];
					strncpy(Tmp,"Kill(Myself)",sizeof(Tmp) );
					lastActor->AddAction( GameScript::GenerateAction(Tmp) );
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
				//show actor's palettes
				DebugFlags ^= DEBUG_SHOW_PALETTES;
				printf("Show actor's palettes %s\n", DebugFlags & DEBUG_SHOW_PALETTES ? "ON" : "OFF");
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
	int nextCursor = IE_CURSOR_NORMAL;

	lastMouseX = x;
	lastMouseY = y;
	Point p = { x,y };
	core->GetVideoDriver()->ConvertToGame( p.x, p.y );
	if (MouseIsDown && ( !DrawSelectionRect )) {
		if (( abs( p.x - StartX ) > 5 ) || ( abs( p.y - StartY ) > 5 )) {
			DrawSelectionRect = true;
		}
	}
	Game* game = core->GetGame();
	Map* area = game->GetCurrentMap( );

	switch (area->GetBlocked( p ) & (PATH_MAP_PASSABLE|PATH_MAP_TRAVEL)) {
		case 0:
			nextCursor = IE_CURSOR_BLOCKED;
			break;

		case PATH_MAP_PASSABLE:
			nextCursor = IE_CURSOR_WALK;
			break;

		case PATH_MAP_TRAVEL:
		case PATH_MAP_PASSABLE|PATH_MAP_TRAVEL:
			nextCursor = IE_CURSOR_TRAVEL;
			break;
	}

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
		if (lastActor)
			lastActor->SetOver( false );
		if (!actor) {
			lastActor = NULL;
		} else {
			lastActor = actor;
			lastActor->SetOver( true );
			switch (lastActor->Modified[IE_EA]) {
				case EVILCUTOFF:
				case GOODCUTOFF:
					break;

				case PC:
				case FAMILIAR:
				case ALLY:
				case CONTROLLED:
				case CHARMED:
				case EVILBUTGREEN:
					nextCursor = IE_CURSOR_NORMAL;
					break;

				case ENEMY:
				case GOODBUTRED:
					nextCursor = IE_CURSOR_ATTACK;
					break;
				default:
					nextCursor = IE_CURSOR_TALK;
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

	//this won't work atm, target must be honoured by Attack
	source->ClearPath();
	source->ClearActions();
	strncpy(Tmp,"Attack()",sizeof(Tmp) );
	target=tgt; //this is a hack, a deadly one
	source->AddAction( GameScript::GenerateAction( Tmp, true ) );
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
	target=tgt; //this is a hack, a deadly one
	source->AddAction( GameScript::GenerateAction( Tmp, true ) );
}

void GameControl::HandleDoor(Door *door, Actor *actor)
{
	char Tmp[256];

	if (door->IsOpen()) {
		actor->ClearPath();
		actor->ClearActions();
		sprintf( Tmp, "CloseDoor(\"%s\")", door->GetScriptName() );
		actor->AddAction( GameScript::GenerateAction( Tmp, true ) );
	} else {
		actor->ClearPath();
		actor->ClearActions();
		sprintf( Tmp, "OpenDoor(\"%s\")", door->GetScriptName() );
		actor->AddAction( GameScript::GenerateAction( Tmp, true ) );
	}
}

//maybe actor is unneeded
bool GameControl::HandleActiveRegion(InfoPoint *trap, Actor * /*actor*/)
{
	Game* game = core->GetGame();
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
				if(!(trap->Flags&TRAP_DEACTIVATED) ) {
					trap->LastTrigger = game->selected[0];
					trap->Scripts[0]->Update();
					//if reset trap flag not set, deactivate it
					if(!(trap->Flags&TRAP_RESET)) {
						trap->Flags|=TRAP_DEACTIVATED;
					}
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
	Point p = {x,y};
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
	Point p = {x,y};
	core->GetVideoDriver()->ConvertToGame( p.x, p.y );
	Game* game = core->GetGame();
	Map* area = game->GetCurrentMap( );
	if (DrawSelectionRect) {
		Actor** ab;
		unsigned int count = area->GetActorInRect( ab, SelectionRect,true );
		for (i = 0; i < highlighted.size(); i++)
			highlighted[i]->SetOver( false );
		highlighted.clear();
		DeselectAll();
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
			HandleDoor(overDoor, game->selected[0]);
			return;
		}
		if(overInfoPoint) {
			if(HandleActiveRegion(overInfoPoint, game->selected[0])) {
				return;
			}
		}
		//just a single actor, no formation
		if(game->selected.size()==1) {
			actor=game->selected[0];
			actor->ClearPath();
			actor->ClearActions();
			sprintf( Tmp, "MoveToPoint([%d.%d])", p.x, p.y );
			actor->AddAction( GameScript::GenerateAction( Tmp, true ) );
			//we clicked over a searchmap travel region
				if( ( ( Window * ) Owner )->Cursor == IE_CURSOR_TRAVEL) {
				sprintf( Tmp, "NIDSpecial2()" );
				actor->AddAction( GameScript::GenerateAction( Tmp, true ) );
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
		if( ( ( Window * ) Owner )->Cursor == IE_CURSOR_TRAVEL) {
			sprintf( Tmp, "NIDSpecial2()" );
			actor->AddAction( GameScript::GenerateAction( Tmp, true ) );
		}
		return;
	}
	if(!actor) return;
	//we got an actor past this point

	//determining the type of the clicked actor
	int type;

	type = actor->GetStat(IE_EA);
	if( type>=EVILCUTOFF ) {
		type = 2; //hostile
	}
	else if( type > GOODCUTOFF ) {
		type = 1; //neutral
	}
	else {
		type = 0; //party
	}
	
	switch (type) {
		case 0:
			//clicked on a new party member
			// FIXME: call GameControl::SelectActor() instead
			game->SelectActor( actor, true, SELECT_REPLACE );
			break;
		case 1:
			//talk (first selected talks)
			if(game->selected.size()) {
				TryToTalk(game->selected[0], actor);
			}
			break;
		case 2:
			//all of them attacks the red circled actor
			for(i=0;i<game->selected.size();i++) {
				TryToAttack(game->selected[i], actor);
			}
			break;
	}
}
/** Special Key Press */
void GameControl::OnSpecialKeyPress(unsigned char Key)
{
	if(DialogueFlags&DF_IN_DIALOG) {
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
			DebugFlags ^= DEBUG_SHOW_CONTAINERS;
			printf( "ALT pressed\n" );
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

Map *GameControl::SetCurrentArea(int Index)
{
	Game* game = core->GetGame();
	game->MapIndex = Index;
	Map* area = game->GetCurrentMap( );
	memcpy(game->CurrentArea, area->GetScriptName(), 9);
	area->SetupAmbients();
	//night or day?
	//if in combat, play battlesong (or don't stop song here)
	//if night, play night song
	area->PlayAreaSong( 0 );
	return area;
}

void GameControl::CalculateSelection(Point &p)
{
	unsigned int i;
	Game* game = core->GetGame();
	Map* area = game->GetCurrentMap( );
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
		Actor* actor = area->GetActor( p, action );
		if (lastActor)
			lastActor->SetOver( false );
		if (!actor) {
			lastActor = NULL;
		} else {
			lastActor = actor;
			lastActor->SetOver( true );
		}
	}
}

void GameControl::SetCutSceneMode(bool active)
{
	if(active) {
		ScreenFlags |= SF_DISABLEMOUSE;
	}
	else {
		ScreenFlags &= ~SF_DISABLEMOUSE;
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
				core->SetVisible( index, 0 );
				if (dict->Lookup( WindowPosition, index )) {
					ResizeDel( w, index );
				}
				return;
			}
			printMessage("GameControl", "Invalid Window Index", LIGHT_RED);
			printf("%s:%u\n",WindowName, index);
		}
	}
}

void GameControl::HideGUI()
{
	if (!(ScreenFlags&SF_GUIENABLED) ) {
		return;
	}
	ScreenFlags &=~SF_GUIENABLED;
	HandleWindowHide("MessageWindow", "MessagePosition");
	HandleWindowHide("OptionsWindow", "OptionsPosition");
	HandleWindowHide("PortraitWindow", "PortraitPosition");
	HandleWindowHide("ActionsWindow", "ActionsPosition");
	HandleWindowHide("TopWindow", "TopPosition");
	HandleWindowHide("OtherWindow", "OtherPosition");
	//FloatWindow doesn't affect gamecontrol, so it is special
	Variables* dict = core->GetDictionary();
	ieDword index;

	if (dict->Lookup( "FloatWindow", index )) {
		if (index != (ieDword) -1) {
			core->SetVisible( index, 0 );
		}
	}
	core->GetVideoDriver()->SetViewport( ( ( Window * ) Owner )->XPos, ( ( Window * ) Owner )->YPos, Width, Height );
}


void GameControl::HandleWindowReveal(const char *WindowName, const char *WindowPosition)
{
	Variables* dict = core->GetDictionary();
	ieDword index;

	if (dict->Lookup( WindowName, index )) {
		if (index != (ieDword) -1) {
			Window* w = core->GetWindow( index );
			if (w) {
				core->SetVisible( index, 1 );
				if (dict->Lookup( WindowPosition, index )) {
					ResizeAdd( w, index );
				}
				return;
			}
			printMessage("GameControl", "Invalid Window Index", LIGHT_RED);
			printf("%s:%u\n",WindowName, index);
		}
	}
}

void GameControl::UnhideGUI()
{
	if (ScreenFlags&SF_GUIENABLED) {
		return;
	}
	ScreenFlags |= SF_GUIENABLED;
	// Unhide the gamecontrol window
	core->SetVisible( 0, 1 );

	HandleWindowReveal("MessageWindow", "MessagePosition");
	HandleWindowReveal("OptionsWindow", "OptionsPosition");
	HandleWindowReveal("PortraitWindow", "PortraitPosition");
	HandleWindowReveal("ActionsWindow", "ActionsPosition");
	HandleWindowReveal("TopWindow", "TopPosition");
	HandleWindowReveal("OtherWindow", "OtherPosition");
	//the floatwindow is a special case
	Variables* dict = core->GetDictionary();
	ieDword index;

	if (dict->Lookup( "FloatWindow", index )) {
		if (index != (ieDword) -1) {
			Window* fw = core->GetWindow( index );
			core->SetVisible( index, 1 );
			fw->Floating = true;
			core->SetOnTop( index );
		}
	}
	core->GetVideoDriver()->SetViewport( ( ( Window * ) Owner )->XPos, ( ( Window * ) Owner )->YPos, Width, Height );
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

void GameControl::InitDialog(Actor* speaker, Actor* target, const char* dlgref)
{
	DialogMgr* dm = ( DialogMgr* ) core->GetInterface( IE_DLG_CLASS_ID );
	dm->Open( core->GetResourceMgr()->GetResource( dlgref, IE_DLG_CLASS_ID ), true );
	if(dlg) {
		delete dlg;
	}
	dlg = dm->GetDialog();
	core->FreeInterface( dm );

	if (!dlg) {
		printf( "[GameControl]: Cannot start dialog: %s\n", dlgref );
		return;
	}
	strnuprcpy(dlg->ResRef, dlgref, 8); //this isn't handled by GetDialog???
	//target is here because it could be changed when a dialog runs onto
	//and external link, we need to find the new target (whose dialog was
	//linked to)
	this->target = target;
	speaker->LastTalkedTo=target;
	target->LastTalkedTo=speaker;
	if (DialogueFlags&DF_IN_DIALOG) {
		return;
	}
	this->speaker = speaker;
	ScreenFlags |= SF_GUIENABLED|SF_DISABLEMOUSE|SF_CENTERONACTOR;
	DialogueFlags |= DF_IN_DIALOG;
	//allow mouse selection from dialog (even though screen is locked)
	core->GetVideoDriver()->DisableMouse = false;
	//there are 3 bits, if they are all unset, the dialog freezes scripts
	if (!(dlg->Flags&7) ) {
		DialogueFlags |= DF_FREEZE_SCRIPTS;
	}
	ieDword index;
	core->GetDictionary()->Lookup( "MessageWindowSize", index );
	if (index == 0) {
		// FIXME: should use RunEventHandler()
		core->GetGUIScriptEngine()->RunFunction( "OnIncreaseSize" );
		core->GetGUIScriptEngine()->RunFunction( "OnIncreaseSize" );
	} else {
		if (index == 1) {
			// FIXME: should use RunEventHandler()
			core->GetGUIScriptEngine()->RunFunction( "OnIncreaseSize" );
		}
	}
	DialogChoose( (unsigned int) -1 );
}

/*try to break will only try to break it, false means unconditional stop*/
void GameControl::EndDialog(bool try_to_break)
{
	if(try_to_break && (DialogueFlags&DF_UNBREAKABLE) )
	{
		return;
	}
	if(speaker && (DialogueFlags&DF_TALKCOUNT) )
	{
		speaker->TalkCount++;
	}
	if (speaker) { //this could be wrong
		speaker->CurrentAction = NULL;
	}
	speaker = NULL;
	target = NULL;
	ds = NULL;
	if (dlg) {
		delete dlg;
		dlg = NULL;
	}
	// FIXME: should use RunEventHandler()
	core->GetGUIScriptEngine()->RunFunction( "OnDecreaseSize" );
	core->GetGUIScriptEngine()->RunFunction( "OnDecreaseSize" );
	ScreenFlags &=~SF_DISABLEMOUSE;
	DialogueFlags = 0;
}

void GameControl::DialogChoose(unsigned int choose)
{
	char Tmp[256];
	ieDword index;

	if (!core->GetDictionary()->Lookup( "MessageWindow", index )) {
		return;
	}
	Window* win = core->GetWindow( index );
	if (!core->GetDictionary()->Lookup( "MessageTextArea", index )) {
		return;
	}
	TextArea* ta = ( TextArea* ) win->GetControl( index );
	//get the first state with true triggers!
	if (choose == (unsigned int) -1) {
		int si = dlg->FindFirstState( target );
		if (si < 0) {
			printf( "[Dialog]: No top level condition evaluated for true.\n" );
			ta->SetMinRow( false );
			EndDialog();
			return;
		}
		ds = dlg->GetState( si );
	} else {
		if (ds->transitionsCount <= choose)
			return;

		DialogTransition* tr = ds->transitions[choose];

		if (tr->Flags&IE_DLG_TR_JOURNAL) {
			int Section = 0;
			if (tr->Flags&IE_DLG_UNSOLVED) {
				Section |= 1;
			}
			if (tr->Flags&IE_DLG_SOLVED) {
				Section |= 2;
			}
			core->GetGame()->AddJournalEntry(tr->journalStrRef, Section, tr->Flags>>16);
//			your journal has changed...
//			JournalChanged = true;
		}

		ta->PopLines( ds->transitionsCount + 1 );
		if (tr->textStrRef != 0xffffffff) {
			char *string = core->GetString( tr->textStrRef );
			AddTalk( ta, target, "A0A0FF", string, "8080FF" );
			free( string );
		}

		if (tr->action) {
			for (unsigned int i = 0; i < tr->action->count; i++) {
				Action* action = GameScript::GenerateAction( tr->action->strings[i], true );
				if (action) {
						speaker->AddAction( action );
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
			speaker = core->GetGame()->GetCurrentMap()->GetActorByDialog(tr->Dialog);
			if(!speaker) {
				printMessage("Dialog","Can't redirect dialog",YELLOW);
				ta->SetMinRow( false );
				EndDialog();
				return;
			}
			InitDialog( speaker, target, tr->Dialog );
		}
		ds = dlg->GetState( si );
	}
	char* string = core->GetString( ds->StrRef, IE_STR_SOUND|IE_STR_SPEECH);
	AddTalk( ta, speaker, "FF0000", string, "70FF70" );
	free( string );
	int i;
	ta->SetMinRow( true );
	int idx = 0;
	for (unsigned int x = 0; x < ds->transitionsCount; x++) {
		if (ds->transitions[x]->Flags & IE_DLG_TR_TRIGGER) {
			if(!dlg->EvaluateDialogTrigger(speaker, ds->transitions[x]->trigger)) {
				continue;
			}
		}
		if (ds->transitions[x]->textStrRef == 0xffffffff) {
			string = ( char * ) malloc( 40 );
			sprintf( string, "[s=%d,ffffff,ff0000]Continue", x );
			i = ta->AppendText( string, -1 );
			free( string );
			ta->AppendText( "[/s]", i );
		} else {
			string = ( char * ) malloc( 40 );
			idx++;
			sprintf( string, "[s=%d,ffffff,ff0000]%d - [p]", x, idx );
			i = ta->AppendText( string, -1 );
			free( string );
			string = core->GetString( ds->transitions[x]->textStrRef );
			ta->AppendText( string, i );
			free( string );
			ta->AppendText( "[/p][/s]", i );
		}
	}
	ta->AppendText( "", -1 );
	// is this correct?
	if (DialogueFlags & DF_FREEZE_SCRIPTS) {
		speaker->ProcessActions();
	}
}

void GameControl::DisplayString(Point &p, const char *Text)
{
	Scriptable* scr = new Scriptable( ST_TRIGGER );
	scr->overHeadText = (char *) Text;
	scr->textDisplaying = 1;
	scr->timeStartDisplaying = 0;
	scr->Pos = p;
	scr->MySelf = NULL;
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
	scr->MySelf = target;
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
		int mi = core->GetGame()->LoadMap( pc->Area );
		SetCurrentArea( mi );
		ScreenFlags|=SF_CENTERONACTOR;
	}
	//center on first selected actor
	Region vp = core->GetVideoDriver()->GetViewport();
	if(ScreenFlags&SF_CENTERONACTOR) {
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
