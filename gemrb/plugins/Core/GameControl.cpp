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
 */

#ifndef WIN32
#include <sys/time.h>
#endif
#include "../../includes/win32def.h"
#include "GameControl.h"
#include "Interface.h"
#include "AnimationMgr.h"
#include "DialogMgr.h"

#define IE_CHEST_CURSOR	32

extern Interface* core;
#ifdef WIN32
extern HANDLE hConsole;
#endif

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

GameControl::GameControl(void)
{
	Changed = true;
	ChangeArea = false;
	lastActor = NULL;
	MouseIsDown = false;
	DrawSelectionRect = false;
	overDoor = NULL;
	overContainer = NULL;
	overInfoPoint = NULL;
	drawPath = NULL;
	pfsX = 0;
	pfsY = 0;
	InfoTextPalette = core->GetVideoDriver()->CreatePalette( white, black );
	lastCursor = 0;
	moveX = moveY = 0;
	DebugFlags = 0;
	AIUpdateCounter = 1;
	effect = NULL;
	DisableMouse = false;
	LeftCount = 0;
	BottomCount = 0;
	RightCount = 0;
	TopCount = 0;
	GUIEnabled = false;
	Dialogue = false;
	Destination[0] = 0;
	EntranceName[0] = 0;
	dlg = NULL;
	target = NULL;
	speaker = NULL;
	HotKey = 0;
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
	Game* game = core->GetGame();
	if (game->MapIndex == -1) {
		return;
	}
	if (!Width || !Height) {
		return;
	}
	if (( selected.size() == 1 ) && ( selected[0]->InParty )) {
		ChangeMap();
	}
	Video* video = core->GetVideoDriver();
	Region viewport = core->GetVideoDriver()->GetViewport();
	viewport.x += video->moveX;
	viewport.y += video->moveY;
	video->SetViewport( viewport.x, viewport.y );
	Region vp( x + XPos, y + YPos, Width, Height );
	Map* area = game->GetCurrentMap( );
	if (area) {
		core->GSUpdate();
		area->DrawMap( vp, this );
		HotKey = 0;
		if (DisableMouse)
			return;
		short GameX = lastMouseX, GameY = lastMouseY;
		video->ConvertToGame( GameX, GameY );
		if (DrawSelectionRect) {
			CalculateSelection( GameX, GameY );
/* unused ?
			short xs[4] = {
				SelectionRect.x, SelectionRect.x + SelectionRect.w,
				SelectionRect.x + SelectionRect.w, SelectionRect.x
			};
			short ys[4] = {
				SelectionRect.y, SelectionRect.y,
				SelectionRect.y + SelectionRect.h,
				SelectionRect.y + SelectionRect.h
			};
*/
			Point points[4] = {
				{SelectionRect.x, SelectionRect.y},
				{SelectionRect.x + SelectionRect.w, SelectionRect.y},
				{SelectionRect.x + SelectionRect.w, SelectionRect.y + SelectionRect.h},
				{SelectionRect.x, SelectionRect.y + SelectionRect.h}
			};
			Gem_Polygon poly( points, 4 );
			video->DrawPolyline( &poly, green, false );
		}
		if (DebugFlags & 4) {
			Door* d;
			//there is a real assignment in the loop!
			for (unsigned int idx = 0;
				(d = area->tm->GetDoor( idx ));
				idx++) {
				if (d->Flags & 1) {
					video->DrawPolyline( d->closed, cyan, true );
				}
				else {
					video->DrawPolyline( d->open, cyan, true );
				}
			}
		}
		//draw containers when ALT was pressed
		if (DebugFlags & 4) {
			Container* c;
			//there is a real assignment in the loop!
			for (unsigned int idx = 0;
				(c = area->tm->GetContainer( idx ));
				idx++) {
				if (c->TrapDetected && c->Trapped) {
					video->DrawPolyline( c->outline, red, true );
				} else {
					video->DrawPolyline( c->outline, cyan, true );
				}
			}
		}
		if (effect) {
			if (( selected.size() == 1 )) {
				Actor* actor = selected.at( 0 );
				video->BlitSpriteMode( effect->NextFrame(), actor->XPos,
						actor->YPos, 1, false );
			}
		}
		if (DebugFlags & 5) {
			//draw infopoints with blue overlay
			InfoPoint* i;
			//there is a real assignment in the loop!
			for (unsigned int idx = 0;
				(i = area->tm->GetInfoPoint( idx ));
				idx++) {
				if (( i->TrapDetected || ( DebugFlags & 1 ) ) && i->Trapped) {
					video->DrawPolyline( i->outline, red, true );
				} else if (DebugFlags & 1) {
					video->DrawPolyline( i->outline, blue, true );
				}
			}
		} else if (overInfoPoint) {
			if (overInfoPoint->TrapDetected && overInfoPoint->Trapped) {
				video->DrawPolyline( overInfoPoint->outline, red, true );
			}
		}
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
				Font* font = core->GetFont( 9 );
				Region rgn( infoTexts[i]->XPos - 200,
					infoTexts[i]->YPos - 100, 400, 400 );
				//printf("Printing InfoText at [%d,%d,%d,%d]\n", rgn.x, rgn.y, rgn.w, rgn.h);
				rgn.x += video->xCorr;
				rgn.y += video->yCorr;
				font->Print( rgn,
						( unsigned char * ) infoTexts[i]->overHeadText,
						InfoTextPalette,
						IE_FONT_ALIGN_CENTER | IE_FONT_ALIGN_TOP, false );
			}
		}

		if (drawPath) {
			PathNode* node = drawPath;
			while (true) {
				short GameX = ( node-> x*16) + 8, GameY = ( node->y*12 ) + 6;
				if (!node->Parent) {
					video->DrawCircle( GameX, GameY, 2, green );
				} else {
					short oldX = ( node->Parent-> x*16) + 8, oldY = ( node->Parent->y*12 ) + 6;
					video->DrawLine( oldX, oldY, GameX, GameY, green );
				}
				if (!node->Next) {
					video->DrawCircle( GameX, GameY, 2, green );
					break;
				}
				node = node->Next;
			}
		}

		if (DebugFlags & 2) {
			Sprite2D* spr = area->SearchMap->GetImage();
			video->BlitSprite( spr, 0, 0, true );
			video->FreeSprite( spr );
			Region point( GameX / 16, GameY / 12, 1, 1 );
			video->DrawRect( point, red );
		}
	} else {
		core->GetVideoDriver()->DrawRect( vp, blue );
	}
}
/** Sets the Text of the current control */
int GameControl::SetText(const char* string, int pos)
{
	return 0;
}
/** Key Press Event */
void GameControl::OnKeyPress(unsigned char Key, unsigned short Mod)
{
	HotKey=tolower(Key);
}

void GameControl::SelectActor(int whom)
{
	for (size_t i = 0; i < selected.size(); i++) {
		selected[i]->Select( false );
	}
	selected.clear();
	Game* game = core->GetGame();
	if(whom==-1) {
		for(int i = 0; i < game->GetPartySize(0); i++) {
			Actor* actor = game->GetPC( i );
			if (!actor) {
				continue;
			}
			if (actor->GetStat(IE_STATE_ID)&STATE_DEAD) {
				continue;
			}
			selected.push_back( actor );
			actor->Select( true );
		}
		return;
	}
	/* doesn't fall through here */
	Actor* actor = game->GetPC( whom );
	if (actor && !(actor->GetStat(IE_STATE_ID)&STATE_DEAD)) {
		selected.push_back( actor );
		actor->Select( true );
	}
}

/** Key Release Event */
void GameControl::OnKeyRelease(unsigned char Key, unsigned short Mod)
{
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
			DebugFlags &= ~8;
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
			case 'a':
				//'a'
				 {
					if (overInfoPoint) {
						if (overInfoPoint->Trapped &&
							!( overInfoPoint->TrapDetected )) {
							overInfoPoint->TrapDetected = 1;
							core->GetVideoDriver()->FreeSprite( overInfoPoint->outline->fill );
							overInfoPoint->outline->fill = NULL;
						}
					}
					if (overContainer) {
						if (overContainer->Trapped &&
							!( overContainer->TrapDetected )) {
							overContainer->TrapDetected = 1;
							core->GetVideoDriver()->FreeSprite( overContainer->outline->fill );
							overContainer->outline->fill = NULL;
						}
					}
				}
				break;

			case 'b':
				 {
					if (selected.size() == 1) {
						if (!effect) {
							AnimationMgr* anim = ( AnimationMgr* )
								core->GetInterface( IE_BAM_CLASS_ID );
							DataStream* ds = core->GetResourceMgr()->GetResource( "S056ICBL",
																		IE_BAM_CLASS_ID );
							anim->Open( ds, true );
							effect = anim->GetAnimation( 1, 0, 0 );
						} else {
							delete( effect );
							effect = NULL;
						}
					}
				}
				break;

			case 'p':
				//'p'
				 {
					short GameX = lastMouseX, GameY = lastMouseY;
					core->GetVideoDriver()->ConvertToGame( GameX, GameY );
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
					drawPath = core->GetPathFinder()->FindPath( pfsX, pfsY,
														GameX, GameY );
				}
				break;

			case 's':
				// s
				 {
					pfsX = lastMouseX; 
					pfsY = lastMouseY;
					core->GetVideoDriver()->ConvertToGame( pfsX, pfsY );
				}
				break;

			case 'j':
				// j
				 {
					for (unsigned int i = 0; i < selected.size(); i++) {
						Actor* actor = selected[i];
						short cX = lastMouseX; 
						short cY = lastMouseY;
						core->GetVideoDriver()->ConvertToGame( cX, cY );
						actor->SetPosition( cX, cY );
						printf( "Teleported to %d, %d\n", cX, cY );
					}
				}
				break;

			case 'm':
				// 'm'
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
				} {
					Game* game = core->GetGame();
					Map* area = game->GetCurrentMap( );
					area->DebugDump();
				}
				break;
			case 'x':
				// 'x'
				 {
					Game* game = core->GetGame();
					Map* area = game->GetCurrentMap( );
					short cX = lastMouseX; 
					short cY = lastMouseY;
					core->GetVideoDriver()->ConvertToGame( cX, cY );
					printf( "%s [%d.%d]\n", area->scriptName, cX, cY );
				}
				break;
			case 'y':
				if (lastActor) {
					lastActor->ClearActions();
					lastActor->DeleteMe = true;
				}
				break;
			case '4':
				// '4'  //show all traps
				DebugFlags ^= 1;
				break;
			case '5':
				// '5' //show the searchmap
				DebugFlags ^= 2;
				break;
			default:
				printf( "KeyRelease:%d  %d\n", Key, Mod );
				break;
		}
	}
}
/** Mouse Over Event */
void GameControl::OnMouseOver(unsigned short x, unsigned short y)
{
	if (DisableMouse) {
		return;
	}
	int nextCursor = 0;

	lastMouseX = x;
	lastMouseY = y;
	short GameX = x, GameY = y;
	core->GetVideoDriver()->ConvertToGame( GameX, GameY );
	if (MouseIsDown && ( !DrawSelectionRect )) {
		if (( abs( GameX - StartX ) > 5 ) || ( abs( GameY - StartY ) > 5 )) {
			DrawSelectionRect = true;
		}
	}
	Game* game = core->GetGame();
	Map* area = game->GetCurrentMap( );

	switch (area->GetBlocked( GameX, GameY ) & 3) {
		case 0:
			nextCursor = 6;
			break;

		case 1:
			nextCursor = 4;
			break;

		case 2:
		case 3:
			nextCursor = 34;
			break;
	}

	overInfoPoint = area->tm->GetInfoPoint( GameX, GameY );
	if (overInfoPoint) {
		if (overInfoPoint->Type != ST_PROXIMITY) {
			nextCursor = overInfoPoint->Cursor;
		}
	}

	if (overDoor) {
		overDoor->Highlight = false;
	}
	overDoor = area->tm->GetDoor( GameX, GameY );
	if (overDoor) {
		overDoor->Highlight = true;
		nextCursor = overDoor->Cursor;
		overDoor->outlineColor = cyan;
	}

	if (overContainer) {
		overContainer->Highlight = false;
	}
	overContainer = area->tm->GetContainer( GameX, GameY );
	if (overContainer) {
		overContainer->Highlight = true;
		if (overContainer->TrapDetected && overContainer->Trapped) {
			nextCursor = 38;
			overContainer->outlineColor = red;
		} else {
			nextCursor = 2;
			overContainer->outlineColor = cyan;
		}
	}

	if (!DrawSelectionRect) {
		Actor* actor = area->GetActor( GameX, GameY );
		if (lastActor)
			lastActor->SetOver( false );
		if (!actor) {
			lastActor = NULL;
		} else {
			lastActor = actor;
			lastActor->SetOver( true );
			if (( lastActor->Modified[IE_STATE_ID] & STATE_DEAD ) == 0) {
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
						nextCursor = 0;
						break;

					case ENEMY:
					case GOODBUTRED:
						nextCursor = 12;
						break;
					default:
						nextCursor = 18;
						break;
				}
			}
		}
	}

	if (lastCursor != nextCursor) {
		( ( Window * ) Owner )->Cursor = nextCursor;
		lastCursor = nextCursor;
	}
}

void GameControl::TryToTalk(Actor *source, Actor *tgt)
{
	if(tgt->GetNextAction()) {
		DisplayString("Target is busy...");
		return;
	}
	char Tmp[40];

	strncpy(Tmp,"NIDSpecial1()",40);
printf("TryTotalk\n");
	target=tgt; //this is a hack, a deadly one
	source->AddAction( GameScript::GenerateAction( Tmp, true ) );
printf("trytotalk added\n");
}

/** Mouse Button Down */
void GameControl::OnMouseDown(unsigned short x, unsigned short y,
	unsigned char Button, unsigned short Mod)
{
	if (DisableMouse) {
		return;
	}
	short GameX = x, GameY = y;
	core->GetVideoDriver()->ConvertToGame( GameX, GameY );
	MouseIsDown = true;
	SelectionRect.x = GameX;
	SelectionRect.y = GameY;
	StartX = GameX;
	StartY = GameY;
	SelectionRect.w = 0;
	SelectionRect.h = 0;
}
/** Mouse Button Up */
void GameControl::OnMouseUp(unsigned short x, unsigned short y,
	unsigned char Button, unsigned short Mod)
{
	if (DisableMouse) {
		return;
	}
	MouseIsDown = false;
	short GameX = x, GameY = y;
	core->GetVideoDriver()->ConvertToGame( GameX, GameY );
	Game* game = core->GetGame();
	Map* area = game->GetCurrentMap( );
	if (!DrawSelectionRect) {
		Actor* actor = area->GetActor( GameX, GameY );

		if (!actor && ( selected.size() == 1 )) {
			actor = selected.at( 0 );
			Door* door = area->tm->GetDoor( GameX, GameY );
			if (door) {
				if (door->Flags&1) {
					actor->ClearPath();
					actor->ClearActions();
					char Tmp[256];
					sprintf( Tmp, "OpenDoor(\"%s\")", door->Name );
					actor->AddAction( GameScript::GenerateAction( Tmp, true ) );
				} else {
					actor->ClearPath();
					actor->ClearActions();
					char Tmp[256];
					sprintf( Tmp, "CloseDoor(\"%s\")", door->Name );
					actor->AddAction( GameScript::GenerateAction( Tmp, true ) );
				}
				return;
			}
			if (overInfoPoint) {
				switch (overInfoPoint->Type) {
					case ST_TRIGGER:
						 {
							if (overInfoPoint->Scripts[0]) {
								overInfoPoint->LastTrigger = selected[0];
								overInfoPoint->Clicker = selected[0];
								overInfoPoint->Scripts[0]->Update();
							} else {
								if (overInfoPoint->overHeadText) {
									if (overInfoPoint->textDisplaying != 1) {
										overInfoPoint->textDisplaying = 1;
										//infoTexts.push_back(overInfoPoint);
										GetTime( overInfoPoint->timeStartDisplaying );
										DisplayString( overInfoPoint );
									}
								}
							}
						}
						break;

					case ST_TRAVEL:
						 {
							actor->ClearPath();
							actor->ClearActions();
							char Tmp[256];
							sprintf( Tmp, "MoveToPoint([%d.%d])",
								overInfoPoint->XPos, overInfoPoint->YPos );
							actor->AddAction( GameScript::GenerateAction( Tmp,
												true ) );
						}
						break;
					default: //all other types are ignored
						break;
				}
			}
			if (!overInfoPoint || ( overInfoPoint->Type != ST_TRIGGER )) {
				//actor->WalkTo(GameX, GameY);
				actor->ClearPath();
				actor->ClearActions();
				char Tmp[256];
				sprintf( Tmp, "MoveToPoint([%d.%d])", GameX, GameY );
				actor->AddAction( GameScript::GenerateAction( Tmp, true ) );
			}
		}
		//determining the type of the clicked actor
		int type;

		if(actor && selected.size() ) {
			type = actor->GetStat(IE_EA);
			if( type>=EVILCUTOFF ) {
				type = 2;
			}
			else if( type > GOODCUTOFF ) {
				type = 1;
			}
			else {
				type = 0;
			}
		}
		else {
			type = 0;
		}
		switch (type) {
		case 0:
			for (size_t i = 0; i < selected.size(); i++)
				selected[i]->Select( false );
			selected.clear();
			if (actor) {
				selected.push_back( actor );
				actor->Select( true );
			}
			break;
		case 1:
			//talk
			TryToTalk(selected[0], actor);
			break;
		case 2:
			//attack
			break;
		}
	} else {
		Actor** ab;
		int count = area->GetActorInRect( ab, SelectionRect );
		for (size_t i = 0; i < highlighted.size(); i++)
			highlighted[i]->SetOver( false );
		highlighted.clear();
		for (size_t i = 0; i < selected.size(); i++) {
			selected[i]->Select( false );
			selected[i]->SetOver( false );
		}
		selected.clear();
		if (count != 0) {
			for (int i = 0; i < count; i++) {
				ab[i]->Select( true );
				selected.push_back( ab[i] );
			}
		}
		free( ab );
	}
	DrawSelectionRect = false;
}
/** Special Key Press */
void GameControl::OnSpecialKeyPress(unsigned char Key)
{
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
			DebugFlags ^= 4;
			printf( "ALT pressed\n" );
			break;
		case GEM_TAB:
			DebugFlags |= 8;
			printf( "TAB pressed\n" );
			break;
		case GEM_MOUSEOUT:
			moveX = 0;
			moveY = 0;
			break;
	}
	core->GetVideoDriver()->SetViewport( Viewport.x, Viewport.y );
}
Map *GameControl::SetCurrentArea(int Index)
{
	Game* game = core->GetGame();
	game->MapIndex = Index;
	Map* area = game->GetCurrentMap( );
	Actor* ab = game->GetPC( 0 );
	if (ab) {
		area->AddActor( ab );
	}
	//night or day?
	area->PlayAreaSong( 0 );
	core->GetPathFinder()->SetMap( area->SearchMap, area->tm->XCellCount * 4,
							( area->tm->YCellCount * 64 ) / 12 );
	return area;
}

void GameControl::CalculateSelection(unsigned short x, unsigned short y)
{
	Game* game = core->GetGame();
	Map* area = game->GetCurrentMap( );
	if (DrawSelectionRect) {
		if (x < StartX) {
			SelectionRect.w = StartX - x;
			SelectionRect.x = x;
		} else {
			SelectionRect.x = StartX;
			SelectionRect.w = x - StartX;
		}
		if (y < StartY) {
			SelectionRect.h = StartY - y;
			SelectionRect.y = y;
		} else {
			SelectionRect.y = StartY;
			SelectionRect.h = y - StartY;
		}
		Actor** ab;
		int count = area->GetActorInRect( ab, SelectionRect );
		if (count != 0) {
			for (size_t i = 0; i < highlighted.size(); i++)
				highlighted[i]->SetOver( false );
			highlighted.clear();
			for (int i = 0; i < count; i++) {
				ab[i]->SetOver( true );
				highlighted.push_back( ab[i] );
			}
		}
		free( ab );
	} else {
		Actor* actor = area->GetActor( x, y );
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
	DisableMouse = active;
	core->GetVideoDriver()->DisableMouse = active;
	core->GetVideoDriver()->moveX = 0;
	core->GetVideoDriver()->moveY = 0;
}

void GameControl::HideGUI()
{
	if (!GUIEnabled) {
		return;
	}
	GUIEnabled = false;
	Variables* dict = core->GetDictionary();
	unsigned long index;
	if (dict->Lookup( "MessageWindow", index )) {
		if (index != (unsigned long) -1) {
			Window* mw = core->GetWindow( index );
			core->SetVisible( index, 0 );
			if (dict->Lookup( "MessagePosition", index )) {
				ResizeDel( mw, index );
			}
		}
	}
	if (dict->Lookup( "OptionsWindow", index )) {
		if (index != (unsigned long) -1) {
			Window* ow = core->GetWindow( index );
			core->SetVisible( index, 0 );
			if (dict->Lookup( "OptionsPosition", index )) {
				ResizeDel( ow, index );
			}
		}
	}
	if (dict->Lookup( "PortraitWindow", index )) {
		if (index != (unsigned long) -1) {
			Window* pw = core->GetWindow( index );
			core->SetVisible( index, 0 );
			if (dict->Lookup( "PortraitPosition", index )) {
				ResizeDel( pw, index );
			}
		}
	}
	if (dict->Lookup( "ActionsWindow", index )) {
		if (index != (unsigned long) -1) {
			Window* aw = core->GetWindow( index );
			core->SetVisible( index, 0 );
			if (dict->Lookup( "ActionsPosition", index )) {
				ResizeDel( aw, index );
			}
		}
	}
	if (dict->Lookup( "TopWindow", index )) {
		if (index != (unsigned long) -1) {
			Window* tw = core->GetWindow( index );
			core->SetVisible( index, 0 );
			if (dict->Lookup( "TopPosition", index )) {
				ResizeDel( tw, index );
			}
		}
	}
	if (dict->Lookup( "OtherWindow", index )) {
		if (index != (unsigned long) -1) {
			Window* tw = core->GetWindow( index );
			core->SetVisible( index, 0 );
			if (dict->Lookup( "OtherPosition", index )) {
				ResizeDel( tw, index );
			}
		}
	}
	if (dict->Lookup( "FloatWindow", index )) {
		if (index != (unsigned long) -1) {
/* this appears to be needless
			Window* fw = core->GetWindow( index );
*/
			core->SetVisible( index, 0 );
		}
	}
	core->GetVideoDriver()->SetViewport( ( ( Window * ) Owner )->XPos,
		( ( Window * ) Owner )->YPos, Width, Height );
}

void GameControl::UnhideGUI()
{
	if (GUIEnabled) {
		return;
	}
	GUIEnabled = true;
	Variables* dict = core->GetDictionary();
	unsigned long index;
	if (dict->Lookup( "MessageWindow", index )) {
		if (index != (unsigned long) -1) {
			Window* mw = core->GetWindow( index );
			core->SetVisible( index, 1 );
			if (dict->Lookup( "MessagePosition", index )) {
				ResizeAdd( mw, index );
			}
		}
	}
	if (dict->Lookup( "ActionsWindow", index )) {
		if (index != (unsigned long) -1) {
			Window* aw = core->GetWindow( index );
			core->SetVisible( index, 1 );
			if (dict->Lookup( "ActionsPosition", index )) {
				ResizeAdd( aw, index );
			}
		}
	}
	if (dict->Lookup( "OptionsWindow", index )) {
		if (index != (unsigned long) -1) {
			Window* ow = core->GetWindow( index );
			core->SetVisible( index, 1 );
			if (dict->Lookup( "OptionsPosition", index )) {
				ResizeAdd( ow, index );
			}
		}
	}
	if (dict->Lookup( "PortraitWindow", index )) {
		if (index != (unsigned long) -1) {
			Window* pw = core->GetWindow( index );
			core->SetVisible( index, 1 );
			if (dict->Lookup( "PortraitPosition", index )) {
				ResizeAdd( pw, index );
			}
		}
	}
	if (dict->Lookup( "TopWindow", index )) {
		if (index != (unsigned long) -1) {
			Window* tw = core->GetWindow( index );
			core->SetVisible( index, 1 );
			if (dict->Lookup( "TopPosition", index )) {
				ResizeAdd( tw, index );
			}
		}
	}
	if (dict->Lookup( "OtherWindow", index )) {
		if (index != (unsigned long) -1) {
			Window* tw = core->GetWindow( index );
			core->SetVisible( index, 1 );
			if (dict->Lookup( "OtherPosition", index )) {
				ResizeAdd( tw, index );
			}
		}
	}
	if (dict->Lookup( "FloatWindow", index )) {
		if (index != (unsigned long) -1) {
			Window* fw = core->GetWindow( index );
			core->SetVisible( index, 1 );
			fw->Floating = true;
			core->SetOnTop( index );
		}
	}
	core->GetVideoDriver()->SetViewport( ( ( Window * ) Owner )->XPos,
								( ( Window * ) Owner )->YPos, Width, Height );
}

void GameControl::ResizeDel(Window* win, unsigned char type)
{
	switch (type) {
		case 0:
			//Left
			 {
				LeftCount--;
				if (!LeftCount) {
					( ( Window * ) Owner )->XPos -= win->Width;
					( ( Window * ) Owner )->Width += win->Width;
					Width = ( ( Window * ) Owner )->Width;
				}
			}
			break;

		case 1:
			//Bottom
			 {
				BottomCount--;
				if (!BottomCount) {
					( ( Window * ) Owner )->Height += win->Height;
					Height = ( ( Window * ) Owner )->Height;
				}
			}
			break;

		case 2:
			//Right
			 {
				RightCount--;
				if (!RightCount) {
					( ( Window * ) Owner )->Width += win->Width;
					Width = ( ( Window * ) Owner )->Width;
				}
			}
			break;

		case 3:
			//Top
			 {
				TopCount--;
				if (!TopCount) {
					( ( Window * ) Owner )->YPos -= win->Height;
					( ( Window * ) Owner )->Height += win->Height;
					Height = ( ( Window * ) Owner )->Height;
				}
			}
			break;

		case 4:
			//BottomAdded
			 {
				BottomCount--;
				( ( Window * ) Owner )->Height += win->Height;
				Height = ( ( Window * ) Owner )->Height;
			}
			break;
	}
}

void GameControl::ResizeAdd(Window* win, unsigned char type)
{
	switch (type) {
		case 0:
			//Left
			 {
				LeftCount++;
				if (LeftCount == 1) {
					( ( Window * ) Owner )->XPos += win->Width;
					( ( Window * ) Owner )->Width -= win->Width;
					Width = ( ( Window * ) Owner )->Width;
				}
			}
			break;

		case 1:
			//Bottom
			 {
				BottomCount++;
				if (BottomCount == 1) {
					( ( Window * ) Owner )->Height -= win->Height;
					Height = ( ( Window * ) Owner )->Height;
				}
			}
			break;

		case 2:
			//Right
			 {
				RightCount++;
				if (RightCount == 1) {
					( ( Window * ) Owner )->Width -= win->Width;
					Width = ( ( Window * ) Owner )->Width;
				}
			}
			break;

		case 3:
			//Top
			 {
				TopCount++;
				if (TopCount == 1) {
					( ( Window * ) Owner )->YPos += win->Height;
					( ( Window * ) Owner )->Height -= win->Height;
					Height = ( ( Window * ) Owner )->Height;
				}
			}
			break;

		case 4:
			//BottomAdded
			 {
				BottomCount++;
				( ( Window * ) Owner )->Height -= win->Height;
				Height = ( ( Window * ) Owner )->Height;
			}
			break;
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
	strncpy(dlg->ResRef, dlgref, 8); //this isn't handled by GetDialog???
	//target is here because it could be changed when a dialog runs onto
	//and external link, we need to find the new target (whose dialog was
	//linked to)
	this->target = target;
	speaker->LastTalkedTo=target;
	target->LastTalkedTo=speaker;
	if (Dialogue) {
		return;
	}
	this->speaker = speaker;
	DisableMouse = true;
	Dialogue = true;
	unsigned long index;
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

static void AddTalk(TextArea* ta, Actor* speaker, char* speaker_color,
	char* text, char* text_color)
{
	char* format = "[color=%s]%s -  [/color][p][color=%s]%s[/color][/p]";
	int newlen = strlen( format ) + strlen( speaker->LongName ) +
		strlen( speaker_color ) + strlen( text ) +
		strlen( text_color ) + 1;
	char* newstr = ( char* ) malloc( newlen );
	sprintf( newstr, format, speaker_color, speaker->LongName, text_color,
		text );

	ta->AppendText( newstr, -1 );
	ta->AppendText( "", -1 );

	free( newstr );
}

void GameControl::EndDialog()
{
	if (speaker) {
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
	DisableMouse = false;
	Dialogue = false;
}

int GameControl::FindFirstState(Scriptable* target, Dialog* dlg)
{
	for (int i = 0; i < dlg->StateCount(); i++) {
		if (EvaluateDialogTrigger( target, dlg->GetState( i )->trigger )) {
			return i;
		}
	}
	return -1;
}

bool GameControl::EvaluateDialogTrigger(Scriptable* target, DialogString* trigger)
{
	int ORcount = 0;
	int result;
	bool subresult = true;

	if (!trigger) {
		return false;
	}
	for (unsigned int t = 0; t < trigger->count; t++) {
		result = GameScript::EvaluateString( target, trigger->strings[t] );
		if (result > 1) {
			if (ORcount)
				printf( "[Dialog]: Unfinished OR block encountered!\n" );
			ORcount = result;
			subresult = false;
			continue;
		}
		if (ORcount) {
			subresult |= ( result != 0 );
			if (--ORcount)
				continue;
			result = subresult ? 1 : 0;
		}
		if (!result)
			return 0;
	}
	if (ORcount) {
		printf( "[Dialog]: Unfinished OR block encountered!\n" );
	}
	return 1;
}

void GameControl::DialogChoose(unsigned int choose)
{
	unsigned long index;

	if (!core->GetDictionary()->Lookup( "MessageWindow", index )) {
		return;
	}
	Window* win = core->GetWindow( index );
	if (!core->GetDictionary()->Lookup( "MessageTextArea", index )) {
		return;
	}
	TextArea* ta = ( TextArea* ) win->GetControl( index );
	//get the first state with true triggers!
	if (choose == (unsigned long) -1) {
		int si = FindFirstState( target, dlg );
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

		if(tr->Flags&IE_DLG_TR_JOURNAL) {
			int Section = 0;
			if(tr->Flags&IE_DLG_UNSOLVED) {
				Section |= 1;
			}
			if(tr->Flags&IE_DLG_SOLVED) {
				Section |= 2;
			}
			core->GetGame()->AddJournalEntry(tr->journalStrRef, Section, tr->Flags>>16);
//			your journal has changed...
//			JournalChanged = true;
		}

		ta->PopLines( ds->transitionsCount + 1 );
		char *string = core->GetString( tr->textStrRef );
		AddTalk( ta, target, "A0A0FF", string, "8080FF" );
		free( string );

		if (tr->action) {
			for (unsigned int i = 0; i < tr->action->count; i++) {
				Action* action = GameScript::GenerateAction( tr->action->strings[i], true );
				if (action) {
						speaker->AddAction( action );
				} else {
					printf( "[GameScript]: can't compile action: %s\n",
						tr->action->strings[i] );
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
			InitDialog( speaker, target, tr->Dialog );
		}
		ds = dlg->GetState( si );
	}
	char* string = core->GetString( ds->StrRef, IE_STR_SOUND );
	AddTalk( ta, speaker, "FF0000", string, "70FF70" );
	free( string );
	int i;
	ta->SetMinRow( true );
	int idx = 0;
	for (unsigned int x = 0; x < ds->transitionsCount; x++) {
		if (ds->transitions[x]->Flags & IE_DLG_TR_TRIGGER) {
			if(!EvaluateDialogTrigger(speaker, ds->transitions[x]->trigger)) {
				continue;
			}
		}
		if (ds->transitions[x]->textStrRef == 0) {
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
}

void GameControl::DisplayString(Scriptable* target)
{
	Scriptable* scr = new Scriptable( ST_TRIGGER );
	int len = strlen( target->overHeadText ) + 1;
	scr->overHeadText = ( char * ) malloc( len );
	strcpy( scr->overHeadText, target->overHeadText );
	scr->textDisplaying = 1;
	scr->timeStartDisplaying = target->timeStartDisplaying;
	scr->XPos = target->XPos;
	scr->YPos = target->YPos;
	scr->MySelf = target;
	infoTexts.push_back( scr );
}

void GameControl::ChangeMap()
{
	Actor* pc = selected.at( 0 );
	//setting the current area
	strncpy(core->GetGame()->CurrentArea, core->GetGame()->GetCurrentMap()->scriptName,8);
	if (stricmp( pc->Area, core->GetGame()->CurrentArea) == 0) {
		return;
	}
	EndDialog();
	overInfoPoint = NULL;
	overContainer = NULL;
	overDoor = NULL;
	for (unsigned int i = 0; i < infoTexts.size(); i++) {
		delete( infoTexts[i] );
	}
	infoTexts.clear();
	selected.clear();
	core->GetGame()->DelMap( MapIndex, true );
	int mi = core->GetGame()->LoadMap( pc->Area );
	Map* map = SetCurrentArea( mi );
	selected.push_back( pc );
	if (EntranceName[0]) {
		Entrance* ent = map->GetEntrance( EntranceName );
		unsigned int XPos, YPos;
		if (!ent) {
			textcolor( YELLOW );
			printf( "WARNING!!! %s EntryPoint does not Exists\n",
				EntranceName );
			textcolor( WHITE );
			XPos = map->tm->XCellCount * 4;
			YPos = ( map->tm->YCellCount * 64 ) / 12;
		} else {
			XPos = ent->XPos / 16; 
			YPos = ent->YPos / 12;
		}
		core->GetPathFinder()->AdjustPosition( XPos, YPos );
		pc->XPos = ( unsigned short ) ( XPos * 16 ) + 8;
		pc->YPos = ( unsigned short ) ( YPos * 12 ) + 8;
		EntranceName[0] = 0;
	}
	Region vp = core->GetVideoDriver()->GetViewport();
	core->GetVideoDriver()->SetViewport( pc->XPos - ( vp.w / 2 ),
								pc->YPos - ( vp.h / 2 ) );
	ChangeArea = false;
}

void GameControl::DisplayString(char* Text)
{
	unsigned long WinIndex, TAIndex;

	core->GetDictionary()->Lookup( "MessageWindow", WinIndex );
	if (( WinIndex != (unsigned long) -1 ) &&
		( core->GetDictionary()->Lookup( "MessageTextArea", TAIndex ) )) {
		Window* win = core->GetWindow( WinIndex );
		if (win) {
			TextArea* ta = ( TextArea* ) win->GetControl( TAIndex );
			ta->AppendText( Text, -1 );
		}
	}
}

