/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003-2005 The GemRB Project
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
#include "../../includes/win32def.h"
#include "../../includes/strrefs.h"
#include "ActorBlock.h"
#include "Interface.h"
#include "SpriteCover.h"
#include "TileMap.h"
#include "Video.h"
#include "Audio.h"
#include "Item.h"
#include "Spell.h"
#include "Map.h"
#include "Game.h"
#include "GameControl.h"
#include "Projectile.h"

#include <cassert>

extern Interface* core;

#define YESNO(x) ( (x)?"Yes":"No")

/***********************
 *  Scriptable Class   *
 ***********************/
Scriptable::Scriptable(ScriptableType type)
{
	Type = type;
	CutSceneId = NULL;
	for (int i = 0; i < MAX_SCRIPTS; i++) {
		Scripts[i] = NULL;
	}
	overHeadText = NULL;
	textDisplaying = 0;
	timeStartDisplaying = 0;
	scriptName[0] = 0;
	LastTrigger = 0;
	LastEntered = 0;
	LastDisarmed = 0;
	LastDisarmFailed = 0;
	LastUnlocked = 0;
	LastOpenFailed = 0;
	LastPickLockFailed = 0;
	DialogName = 0;
	CurrentAction = NULL;
	UnselectableTimer = 0;
	startTime = 0;   //executing scripts
	lastRunTime = 0; //evaluating scripts
	lastDelay = 0;

	interval = ( 1000 / AI_UPDATE_TIME );
	WaitCounter = 0;
	playDeadCounter = 0;
	if (Type==ST_ACTOR) {
		InternalFlags = IF_VISIBLE | IF_ONCREATION;
	} else {
		InternalFlags = IF_ACTIVE | IF_VISIBLE | IF_ONCREATION;
	}
	area = 0;
	Pos.x = 0;
	Pos.y = 0;

	SpellHeader = -1;
	LastTargetPos.empty();
	locals = new Variables();
	locals->SetType( GEM_VARIABLES_INT );
	locals->ParseKey( 1 );
	InitTriggers();
}

Scriptable::~Scriptable(void)
{
	if (CurrentAction) {
		ReleaseCurrentAction();
	}
	ClearActions();
	for (int i = 0; i < MAX_SCRIPTS; i++) {
		if (Scripts[i]) {
			delete( Scripts[i] );
		}
	}
	if (overHeadText) {
		core->FreeString( overHeadText );
	}
	if (locals) {
		delete( locals );
	}
}

void Scriptable::SetScriptName(const char* text)
{
	//if (text && text[0]) { //this leaves some uninitialized bytes
	//lets hope this won't break anything
	if (text) {
		strnspccpy( scriptName, text, 32 );
	}
}

/** Gets the DeathVariable */
const char* Scriptable::GetScriptName(void)
{
	 return scriptName;
}

Map* Scriptable::GetCurrentArea() const
{
	//this could be NULL, always check it
	return area;
}

void Scriptable::SetMap(Map *map)
{
	if (!map) {
		printMessage("Scriptable","Null map set!\n",LIGHT_RED);
		abort();
	}
	area = map;
}

//ai is nonzero if this is an actor currently in the party
//if the script level is AI_SCRIPT_LEVEL, then we need to 
//load an AI script (.bs) instead of (.bcs)
void Scriptable::SetScript(const ieResRef aScript, int idx, bool ai)
{
	if (idx >= MAX_SCRIPTS) {
		printMessage("Scriptable","Invalid script index!\n",LIGHT_RED);
		abort();
	}
	if (Scripts[idx]) {
		delete Scripts[idx];
	}
	Scripts[idx] = NULL;
	// NONE is an 'invalid' script name, never used seriously
	// This hack is to prevent flooding of the console
	if (aScript[0] && stricmp(aScript, "NONE") ) {
		if (idx!=AI_SCRIPT_LEVEL) ai=false;
		Scripts[idx] = new GameScript( aScript, Type, locals, idx, ai );
		Scripts[idx]->MySelf = this;
	}
}

void Scriptable::SetScript(int index, GameScript* script)
{
	if (index >= MAX_SCRIPTS) {
		printMessage("Scriptable","Invalid script index!\n",LIGHT_RED);
		return;
	}
	if (Scripts[index] ) {
		delete Scripts[index];
	}
	Scripts[index] = script;
}

void Scriptable::DisplayHeadText(const char* text)
{
	if (overHeadText) {
		core->FreeString( overHeadText );
	}
	overHeadText = (char *) text;
	if (text) {
		timeStartDisplaying=core->GetGame()->Ticks;
		textDisplaying = 1;
	}
	else {
		timeStartDisplaying = 0;
		textDisplaying = 0;
	}
}

#define MAX_DELAY  6000
static const Color black={0,0,0,0};

void Scriptable::DrawOverheadText(Region &screen)
{
	unsigned long time = core->GetGame()->Ticks;
	Palette *palette = NULL;

	if (!textDisplaying)
		return;

	time -= timeStartDisplaying;

	Font* font = core->GetFont( 1 );
	if (time >= MAX_DELAY) {
		textDisplaying = 0;
	} else {
		time = MAX_DELAY-time;
		if (time<256) {
			const Color overHeadColor = {time,time,time,time};
			palette = core->CreatePalette(overHeadColor, black);
		}
	}

	int cs = 100;
	if (Type==ST_ACTOR) {
		cs = ((Selectable *) this)->size*50;
	}

	Region rgn( Pos.x-100+screen.x, Pos.y - cs + screen.y, 200, 400 );
	font->Print( rgn, ( unsigned char * ) overHeadText,
		palette?palette:core->InfoTextPalette, IE_FONT_ALIGN_CENTER | IE_FONT_ALIGN_TOP, false );
	//core->GetVideoDriver()->FreePalette(palette);
	gamedata->FreePalette(palette);
}

void Scriptable::ImmediateEvent()
{
	lastRunTime=0;
	/*
	for(int i=0;i<MAX_SCRIPTS;i++) {
		if (Scripts[i]) {
			Scripts[i]->RunNow();
		}
	}
	*/
}

bool Scriptable::IsPC()
{
	if(Type==ST_ACTOR) {
		if (((Actor *) this)->InParty) {
			return true;
		}
	}
	return false;
}

void Scriptable::ExecuteScript(int scriptCount)
{
	if (core->GetGameControl()->GetScreenFlags()&SF_CUTSCENE) {
		return;
	}
	if (WaitCounter) {
		return;
	}

	ieDword thisTime = core->GetGame()->Ticks;
	if (( thisTime - lastRunTime ) < 1000) {
		return;
	}

	lastDelay = lastRunTime;
	lastRunTime = thisTime;

	bool alive = false;

	for (int i=0;i<scriptCount;i++) {
		//disable AI script level for actors in party when the player disabled them
		if ((i==AI_SCRIPT_LEVEL) && IsPC() ) {
			if (core->GetGame()->ControlStatus&CS_PARTY_AI) {
				continue;
			}
		}

		GameScript *Script = Scripts[i];
		if (Script) {
			alive |= Script->Update();
		}
	}
	if (alive && UnselectableTimer) {
			UnselectableTimer--;
			if (!UnselectableTimer) {
				if (Type==ST_ACTOR) {
					((Actor *) this)->SetCircleSize();
				}
			}
	}
}

void Scriptable::AddAction(Action* aC)
{
	if (!aC) {
		printf( "[GameScript]: NULL action encountered for %s!\n",scriptName );
		return;
	}
	InternalFlags|=IF_ACTIVE;
	actionQueue.push_back( aC );
	aC->IncRef();
}

void Scriptable::AddActionInFront(Action* aC)
{
	if (!aC) {
		printf( "[GameScript]: NULL action encountered for %s!\n",scriptName );
		return;
	}
	InternalFlags|=IF_ACTIVE;
	actionQueue.push_front( aC );
	aC->IncRef();
}

Action* Scriptable::GetNextAction()
{
	if (actionQueue.size() == 0) {
		return NULL;
	}
	return actionQueue.front();
}

Action* Scriptable::PopNextAction()
{
	if (actionQueue.size() == 0) {
		return NULL;
	}
	Action* aC = actionQueue.front();
	actionQueue.pop_front();
	return aC;
}

void Scriptable::ClearActions()
{
	for (unsigned int i = 0; i < actionQueue.size(); i++) {
		Action* aC = actionQueue.front();
		actionQueue.pop_front();
		aC->Release();
	}
	actionQueue.clear();
	WaitCounter = 0;
	playDeadCounter = 0; // i'm not sure about this
	LastTarget = 0;
	//clear the triggers as fast as possible when queue ended?
	ClearTriggers();
}

void Scriptable::ReleaseCurrentAction()
{
	if (CurrentAction) {
		CurrentAction->Release();
		CurrentAction = NULL;
	}
}

ieWord Scriptable::GetGlobalID()
{
	if (Type == ST_ACTOR) {
		Actor *actor = (Actor *) this;
		return actor->globalID;
	}
	return 0;
}

void Scriptable::ProcessActions(bool force)
{
	unsigned long thisTime = core->GetGame()->Ticks;

	if (Type == ST_ACTOR) {
		Actor *actor = (Actor *) this;
		actor->PerformAttack(thisTime);
	}

	if (!force && (( thisTime - startTime ) < interval)) {
		return;
	}
	startTime = thisTime;
	if (playDeadCounter) {
		playDeadCounter--;
		if (!playDeadCounter) {
			Movable* mov = ( Movable* ) this;
			mov->SetStance( IE_ANI_GET_UP );
		}
	}
	if (WaitCounter) {
		WaitCounter--;
		return;
	}

	//don't do anything while moving?
	//maybe this should be fixed
	if (InMove()) {
		return;
	}

	if (CurrentAction) {
		ReleaseCurrentAction();
	}

	while (!CurrentAction) {
		CurrentAction = PopNextAction();
		if (!CurrentAction) {
			if (CutSceneId) {
				CutSceneId = NULL;
			}
			//removing the triggers at the end of the
			//block
			//ClearTriggers();
			break;
		}
		GameScript::ExecuteAction( this, CurrentAction );
		//break execution in case of a Wait flag
		if (WaitCounter) {
			//clear triggers while waiting
			//ClearTriggers();
			break;
		}
		//break execution in case of movement
		if (InMove()) {
			break;
		}
	}
	//most likely the best place to clear triggers is here
	//queue is empty, or there is a looong action subject to break
	ClearTriggers();
	if (InternalFlags&IF_IDLE) {
		Deactivate();
	}
}

bool Scriptable::InMove()
{
	if (Type!=ST_ACTOR) {
		return false;
	}
	Movable *me = (Movable *) this;
	return me->GetNextStep()!=NULL;
}

void Scriptable::SetWait(unsigned long time)
{
	WaitCounter = time;
}

unsigned long Scriptable::GetWait()
{
	return WaitCounter;
}

Scriptable *Scriptable::GetCutsceneID()
{
	return CutSceneId;
}

void Scriptable::LeaveDialog()
{
	InternalFlags |=IF_WASINDIALOG;
}

//this ends cutscene mode for this Sender
void Scriptable::ClearCutsceneID()
{
	CutSceneId=NULL;
	InternalFlags &= ~IF_CUTSCENEID;
}

//if the cutsceneID doesn't exist, we simply skip the action
//because the cutscene script executer DOESN'T get hijacked
void Scriptable::SetCutsceneID(Scriptable *csid)
{
	CutSceneId=csid;
	InternalFlags |= IF_CUTSCENEID;
}

//also turning off the idle flag so it won't run continuously
void Scriptable::Deactivate()
{
	InternalFlags &=~(IF_ACTIVE|IF_IDLE);
}

void Scriptable::Hide()
{
	InternalFlags &=~(IF_ACTIVE|IF_VISIBLE);
}

void Scriptable::Unhide()
{
	InternalFlags |= IF_ACTIVE|IF_VISIBLE;
}

void Scriptable::Interrupt()
{
	InternalFlags &= ~IF_NOINT;
}

void Scriptable::NoInterrupt()
{
	InternalFlags |= IF_NOINT;
}

//turning off the not interruptable flag, actions should reenable it themselves
//also turning off the idle flag
//heh, no, i wonder why did i touch the interruptable flag here
void Scriptable::Activate()
{
	InternalFlags |= IF_ACTIVE;
	InternalFlags &= ~IF_IDLE;
}

ieDword Scriptable::GetInternalFlag()
{
	return InternalFlags;
}

void Scriptable::InitTriggers()
{
	tolist.clear();
	bittriggers = 0;
}

void Scriptable::ClearTriggers()
{
	for (TriggerObjects::iterator m = tolist.begin(); m != tolist.end (); m++) {
		*(*m) = 0;
	}
	if (!bittriggers) {
		return;
	}
	if (bittriggers & BT_DIE) {
		InternalFlags &= ~IF_JUSTDIED;
	}
	if (bittriggers & BT_ONCREATION) {
		 InternalFlags &= ~IF_ONCREATION;
	}
	if (bittriggers & BT_BECAMEVISIBLE) {
		 InternalFlags &= ~IF_BECAMEVISIBLE;
	}
	if (bittriggers & BT_PARTYRESTED) {
		 InternalFlags &= ~IF_PARTYRESTED;
	}
	if (bittriggers & BT_WASINDIALOG) {
		InternalFlags &= ~IF_WASINDIALOG;
	}
	InitTriggers();
}

void Scriptable::SetBitTrigger(ieDword bittrigger)
{
	bittriggers |= bittrigger;
}

void Scriptable::AddTrigger(ieDword *actorref)
{
	tolist.push_back(actorref);
}

void Scriptable::CastSpellPointEnd( const ieResRef SpellResRef )
{
	if (Type==ST_ACTOR) {
		((Actor *) this)->SetStance(IE_ANI_CONJURE);
	}

	if (SpellHeader == -1) {
		LastTargetPos.empty();
		return;
	}

	if (LastTargetPos.isempty()) {
		SpellHeader = -1;
		return;
	}

	Spell* spl = gamedata->GetSpell( SpellResRef );
	//create projectile from known spellheader
	Projectile *pro=spl->GetProjectile(SpellHeader);
	SpellHeader = -1;
	if (pro) {
		pro->SetCaster(GetGlobalID());
		GetCurrentArea()->AddProjectile(pro, Pos, LastTargetPos);
	}
	LastTarget = 0;
	LastTargetPos.empty();
}

void Scriptable::CastSpellEnd( const ieResRef SpellResRef )
{
	if (Type==ST_ACTOR) {
		((Actor *) this)->SetStance(IE_ANI_CONJURE);
	}

	if (SpellHeader == -1) {
		LastTarget=0;
		return;
	}
	if (!LastTarget) {
		SpellHeader = -1;
		return;
	}
	Spell* spl = gamedata->GetSpell( SpellResRef );
	//create projectile from known spellheader
	Projectile *pro=spl->GetProjectile(SpellHeader);
	if (pro) {
		pro->SetCaster(GetGlobalID());
		GetCurrentArea()->AddProjectile(pro, Pos, LastTarget);
	}
	gamedata->FreeSpell(spl, SpellResRef, false);
	LastTarget = 0;
	LastTargetPos.empty();
}

//set target as point
//if spell needs to be depleted, do it
//if spell is illegal stop casting
void Scriptable::CastSpellPoint( const ieResRef SpellResRef, Point &target, bool deplete )
{
	LastTarget = 0;
	LastTargetPos.empty();
	if (Type==ST_ACTOR) {
		Actor *actor = (Actor *) this;
		if (actor->HandleCastingStance(SpellResRef,deplete) ) {
			return;
		}
	}
	LastTargetPos = target;
	SpellCast(SpellResRef);
}

//set target as actor (if target isn't actor, use its position)
//if spell needs to be depleted, do it
//if spell is illegal stop casting
void Scriptable::CastSpell( const ieResRef SpellResRef, Scriptable* target, bool deplete )
{
	LastTarget = 0;
	LastTargetPos.empty();
	if (Type==ST_ACTOR) {
		Actor *actor = (Actor *) this;
		if (actor->HandleCastingStance(SpellResRef,deplete) ) {
			return;
		}
	}

	if (!target) target = this;

	if (target->Type!=ST_ACTOR) {
		LastTargetPos = target->Pos;
	} else {
		LastTarget = target->GetGlobalID();
	}
	SpellCast(SpellResRef);
}

//start spellcasting (common part)
void Scriptable::SpellCast(const ieResRef SpellResRef)
{
	Spell* spl = gamedata->GetSpell( SpellResRef );
	if (!spl) {
		SpellHeader = -1;
		return;
	}

	//cfb
	if (Type==ST_ACTOR) {
		int level = ((Actor *) this)->GetXPLevel(true);
		SpellHeader = spl->GetHeaderIndexFromLevel(level);
		Actor *actor = (Actor *) this;
		EffectQueue *fxqueue=spl->GetEffectBlock(-1, SpellHeader);
		fxqueue->SetOwner(actor);
		fxqueue->AddAllEffects(actor, actor->Pos);
		delete fxqueue;
	} else {
		SpellHeader = 0;
	}

	SPLExtHeader *header = spl->GetExtHeader(SpellHeader);
	SetWait(header->CastingTime*AI_UPDATE_TIME);
	gamedata->FreeSpell(spl, SpellResRef, false);
}

/********************
 * Selectable Class *
 ********************/

Selectable::Selectable(ScriptableType type)
	: Scriptable( type )
{
	Selected = false;
	Over = false;
	size = 0;
	cover = NULL;
	circleBitmap[0] = NULL;
	circleBitmap[1] = NULL;
}

void Selectable::SetSpriteCover(SpriteCover* c)
{
	delete cover;
	cover = c;
}

Selectable::~Selectable(void)
{
	delete cover;
}

void Selectable::SetBBox(Region &newBBox)
{
	BBox = newBBox;
}

static const unsigned long tp_steps[8]={3,2,1,0,1,2,3,4};

void Selectable::DrawCircle(Region &vp)
{
	/*  BG2 colours ground circles as follows:
	dark green for unselected party members
	bright green for selected party members
	flashing green/white for a party member the mouse is over
	bright red for enemies
	flashing red/white for enemies the mouse is over
	flashing cyan/white for neutrals the mouse is over
	*/

	if (size<=0) {
		return;
	}
	Color mix;
	Color* col = NULL;
	Sprite2D* sprite = circleBitmap[0];

	if (Selected) {
		col = &selectedColor;
		sprite = circleBitmap[1];
	} else if (Over) {
		//doing a time dependent flashing of colors
		//if it is too fast, increase the 6 to 7
		unsigned long step;
		GetTime( step );
		step = tp_steps [(step >> 6) & 7];
		mix.a=overColor.a;
		mix.r=(overColor.r*step+selectedColor.r*(8-step))/8;
		mix.g=(overColor.g*step+selectedColor.g*(8-step))/8;
		mix.b=(overColor.b*step+selectedColor.b*(8-step))/8;
		col = &mix;
	} else {
		return;
	}

	if (sprite) {
		core->GetVideoDriver()->BlitSprite( sprite, Pos.x - vp.x, Pos.y - vp.y, true );
	}
	else {
		// for size >= 2, radii are (size-1)*16, (size-1)*12
		// for size == 1, radii are 12, 9
		int csize = (size - 1) * 4;
		if (csize < 4) csize = 3;
		core->GetVideoDriver()->DrawEllipse( (ieWord) (Pos.x - vp.x), (ieWord) (Pos.y - vp.y),
		(ieWord) (csize * 4), (ieWord) (csize * 3), *col );
	}
}

// Check if P is over our ground circle
bool Selectable::IsOver(Point &P)
{
	int csize = size;
	if (csize < 2) csize = 2;

	int dx = P.x - Pos.x;
	int dy = P.y - Pos.y;

	// check rectangle first
	if (dx < -(csize-1)*16 || dx > (csize-1)*16) return false;
	if (dy < -(csize-1)*12 || dy > (csize-1)*12) return false;

	// then check ellipse
	int r = 9*dx*dx + 16*dy*dy; // 48^2 * (  (dx/16)^2 + (dy/12)^2  )

	return (r <= 48*48*(csize-1)*(csize-1));
}

bool Selectable::IsSelected()
{
	return Selected==1;
}

void Selectable::SetOver(bool over)
{
	Over = over;
}

//don't call this function after rendering the cover and before the
//blitting of the sprite or bad things will happen :)
void Selectable::Select(int Value)
{
	if (Selected!=0x80 || Value!=1) {
		Selected = (ieWord) Value;
	}
	//forcing regeneration of the cover
	SetSpriteCover(NULL);
}

void Selectable::SetCircle(int circlesize, const Color &color, Sprite2D* normal_circle, Sprite2D* selected_circle)
{
	size = circlesize;
	selectedColor = color;
	overColor.r = color.r >> 1;
	overColor.g = color.g >> 1;
	overColor.b = color.b >> 1;
	overColor.a = color.a;
	circleBitmap[0] = normal_circle;
	circleBitmap[1] = selected_circle;
}

//used for creatures
int Selectable::WantDither()
{
	//if dithering is disabled globally, don't do it
	if (core->FogOfWar&4) {
		return 0;
	}
	//if actor is dead, dither it if polygon wants
	if (Selected&0x80) {
		return 1;
	}
	//if actor is selected dither it
	if (Selected) {
		return 2;
	}
	return 1;
}

/***********************
 * Highlightable Class *
 ***********************/

Highlightable::Highlightable(ScriptableType type)
	: Scriptable( type )
{
	outline = NULL;
	Highlight = false;
	Cursor = IE_CURSOR_NORMAL;
	KeyResRef[0] = 0;
}

Highlightable::~Highlightable(void)
{
	if (outline) {
		delete( outline );
	}
}

bool Highlightable::IsOver(Point &Pos)
{
	if (!outline) {
		return false;
	}
	return outline->PointIn( Pos );
}

void Highlightable::DrawOutline()
{
	if (!outline) {
		return;
	}
	core->GetVideoDriver()->DrawPolyline( outline, outlineColor, true );
}

void Highlightable::SetCursor(unsigned char CursorIndex)
{
	Cursor = CursorIndex;
}

/*****************
 * Movable Class *
 *****************/

Movable::Movable(ScriptableType type)
	: Selectable( type )
{
	Destination = Pos;
	Orientation = 0;
	NewOrientation = 0;
	StanceID = 0;
	path = NULL;
	step = NULL;
	timeStartStep = 0;
	lastFrame = NULL;
	Area[0] = 0;
	AttackMovements[0] = 100;
	AttackMovements[1] = 0;
	AttackMovements[2] = 0;
}

Movable::~Movable(void)
{
	if (path) {
		ClearPath();
	}
}

int Movable::GetPathLength()
{
	PathNode *node = GetNextStep(0);
	int i = 0;
	while (node->Next) {
		i++;
		node = node->Next;
	}
	return i;
}

PathNode *Movable::GetNextStep(int x)
{
	if (!step) {
		DoStep((unsigned int) ~0);
	}
	PathNode *node = step;
	while(node && x--) {
		node = node->Next;
	}
	return node;
}

Point Movable::GetMostLikelyPosition()
{
	if (!path) {
		return Pos;
	}

//actually, sometimes middle path would be better, if
//we stand in Destination already
	int halfway = GetPathLength()/2;
	PathNode *node = GetNextStep(halfway);
	if (node) {
		return Point((ieWord) ((node->x*16)+8), (ieWord) ((node->y*12)+6) );
	}
	return Destination;
}

void Movable::SetStance(unsigned int arg)
{
	//don't modify stance from dead back to anything if the actor is dead
	if ((StanceID==IE_ANI_TWITCH || StanceID==IE_ANI_DIE) && (arg!=IE_ANI_TWITCH) ) {
		if (GetInternalFlag()&IF_REALLYDIED) {
			printMessage("Movable","Stance overridden by death\n", YELLOW);
			return;
		}
	}

	if (arg<MAX_ANIMS) {
		 StanceID=(unsigned char) arg;

		 if (StanceID == IE_ANI_ATTACK) {
			 // Set stance to a random attack animation

			 int random = rand()%100;
			 if (random < AttackMovements[0]) {
				 StanceID = IE_ANI_ATTACK_BACKSLASH;
			 } else if (random < AttackMovements[0] + AttackMovements[1]) {
				 StanceID = IE_ANI_ATTACK_SLASH;
			 } else {
				 StanceID = IE_ANI_ATTACK_JAB;
			 }
		 }

	} else {
		StanceID=IE_ANI_AWAKE; //
		printf("Tried to set invalid stance id (%u)\n", arg);
	}
}

void Movable::SetAttackMoveChances(ieWord *amc)
{
	AttackMovements[0]=amc[0];
	AttackMovements[1]=amc[1];
	AttackMovements[2]=amc[2];
}



//this could be used for WingBuffet as well
void Movable::MoveLine(int steps, int Pass, ieDword orient)
{
	//remove previous path
	ClearPath();
	if (!steps)
		return;
	Point p = Pos;
	p.x/=16;
	p.y/=14;
	path = area->GetLine( p, steps, orient, Pass );
}

void Movable::DoStep(unsigned int walk_speed)
{
	if (!path) {
		return;
	}
	ieDword time = core->GetGame()->Ticks;
	if (!step) {
		step = path;
		timeStartStep = time;
	}
	if (( time - timeStartStep ) >= walk_speed) {
		//printf("[New Step] : Orientation = %d\n", step->orient);
		step = step->Next;
		timeStartStep = time;
	}

	SetOrientation (step->orient, false);
	StanceID = IE_ANI_WALK;
	Pos.x = ( step->x * 16 ) + 8;
	Pos.y = ( step->y * 12 ) + 6;
	if (!step->Next) {
		ClearPath();
		NewOrientation = Orientation;
		//since clearpath no longer sets currentaction to NULL
		//we set it here
		ReleaseCurrentAction();
		return;
	}
	if (!walk_speed) {
		return;
	}
	if (step->Next->x > step->x)
		Pos.x += ( unsigned short )
			( ( ( ( ( step->Next->x * 16 ) + 8 ) - Pos.x ) * ( time - timeStartStep ) ) / walk_speed );
	else
		Pos.x -= ( unsigned short )
			( ( ( Pos.x - ( ( step->Next->x * 16 ) + 8 ) ) * ( time - timeStartStep ) ) / walk_speed );
	if (step->Next->y > step->y)
		Pos.y += ( unsigned short )
			( ( ( ( ( step->Next->y * 12 ) + 6 ) - Pos.y ) * ( time - timeStartStep ) ) / walk_speed );
	else
		Pos.y -= ( unsigned short )
			( ( ( Pos.y - ( ( step->Next->y * 12 ) + 6 ) ) * ( time - timeStartStep ) ) / walk_speed );
}

void Movable::AddWayPoint(Point &Des)
{
	if (!path) {
		WalkTo(Des);
		return;
	}
	Destination = Des;
	//it is tempting to use 'step' here, as it could
	//be about half of the current path already
	PathNode *endNode=path;
	while(endNode->Next) {
		endNode=endNode->Next;
	}
	Point p(endNode->x, endNode->y);
	area->BlockSearchMap( Pos, size, 0);
	PathNode *path2 = area->FindPath( p, Des, size );
	endNode->Next=path2;
	//probably it is wise to connect it both ways?
	path2->Parent=endNode;
}

void Movable::FixPosition()
{
	if (Type!=ST_ACTOR) {
		return;
	}
	Actor *actor = (Actor *) this;
	if (actor->GetStat(IE_DONOTJUMP)&DNJ_BIRD ) {
		return;
	}
	//before fixposition, you should remove own shadow
	area->BlockSearchMap( Pos, size, 0);
	Pos.x/=16;
	Pos.y/=12;
	GetCurrentArea()->AdjustPosition(Pos);
	Pos.x*=16;
	Pos.y*=12;
}

void Movable::WalkTo(Point &Des, int distance)
{
	ClearPath();
	FixPosition();
	area->BlockSearchMap( Pos, size, 0);
	path = area->FindPath( Pos, Des, size, distance );
	//ClearPath sets destination, so Destination must be set after it
	//also we should set Destination only if there is a walkable path
	if (path) {
		Destination = Des;
	}
}

void Movable::RunAwayFrom(Point &Des, int PathLength, int flags)
{
	ClearPath();
	area->BlockSearchMap( Pos, size, 0);
	path = area->RunAway( Pos, Des, size, PathLength, flags );
}

void Movable::RandomWalk(bool can_stop, bool run)
{
	if (path) {
		return;
	}
	//if not continous random walk, then stops for a while
	if (can_stop && (rand()&3) ) {
		SetWait((rand()&7)+7);
		return;
	}
	if (run) {
		InternalFlags|=IF_RUNNING;
	}
	//area->BlockSearchMap( Pos, size, 0);
	Point p = Pos;
	p.x+=core->Roll(1,50,-25);
	p.y+=core->Roll(1,50,-25);
	path = area->RunAway( Pos, p, size, 50, 0 );
}

void Movable::MoveTo(Point &Des)
{
	area->BlockSearchMap( Pos, size, 0);
	Pos = Des;
	Destination = Des;
	area->BlockSearchMap( Pos, size, IsPC()?PATH_MAP_PC:PATH_MAP_NPC);
}

void Movable::ClearPath()
{
	//this is to make sure attackers come to us
	//make sure ClearPath doesn't screw Destination (in the rare cases Destination
	//is set before ClearPath
	Destination = Pos;
	if (StanceID==IE_ANI_WALK || StanceID==IE_ANI_RUN) {
		StanceID = IE_ANI_AWAKE;
	}
	InternalFlags&=~IF_NORECTICLE;
	PathNode* thisNode = path;
	while (thisNode) {
		PathNode* nextNode = thisNode->Next;
		delete( thisNode );
		thisNode = nextNode;
	}
	path = NULL;
	step = NULL;
	//don't call ReleaseCurrentAction
}

void Movable::DrawTargetPoint(Region &vp)
{
	if (!path || !Selected || (InternalFlags&IF_NORECTICLE) )
		return;

	// generates "step" from sequence 3 2 1 0 1 2 3 4
	// updated each 1/15 sec
	unsigned long step;
	GetTime( step );
	step = tp_steps [(step >> 6) & 7];

	//Region vp = core->GetVideoDriver()->GetViewport();
	core->GetVideoDriver()->DrawEllipse( (ieWord) (Destination.x - vp.x),
		(ieWord) (Destination.y - vp.y), (unsigned short) (size * 10 - step),
		(unsigned short) ( size * 15 / 2 - step), selectedColor );

}

/**********************
 * Tiled Object Class *
 **********************/

TileObject::TileObject()
{
	opentiles=NULL;
	opencount=0;
	closedtiles=NULL;
	closedcount=0;
	Flags = 0;
}

TileObject::~TileObject()
{
	if (opentiles) {
		free( opentiles );
	}
	if (closedtiles) {
		free( closedtiles );
	}
}

void TileObject::SetOpenTiles(unsigned short* Tiles, int cnt)
{
	if (opentiles) {
		free( opentiles );
	}
	opentiles = Tiles;
	opencount = cnt;
}

void TileObject::SetClosedTiles(unsigned short* Tiles, int cnt)
{
	if (closedtiles) {
		free( closedtiles );
	}
	closedtiles = Tiles;
	closedcount = cnt;
}

/**************
 * Door Class *
 **************/

Door::Door(TileOverlay* Overlay)
	: Highlightable( ST_DOOR )
{
	tiles = NULL;
	tilecount = 0;
	Flags = 0;
	open = NULL;
	closed = NULL;
	open_ib = NULL;
	oibcount = 0;
	closed_ib = NULL;
	cibcount = 0;
	OpenSound[0] = 0;
	CloseSound[0] = 0;
	LockSound[0] = 0;
	UnLockSound[0] = 0;
	overlay = Overlay;
	LinkedInfo[0] = 0;
	OpenStrRef = (ieDword) -1;
}

Door::~Door(void)
{
	if (Flags&DOOR_OPEN) {
		if (closed) {
			delete( closed );
		}
	} else {
		if (open) {
			delete( open );
		}
	}
	if (tiles) {
		free( tiles );
	}
	if (open_ib) {
		free( open_ib );
	}
	if (closed_ib) {
		free( closed_ib );
	}
}

void Door::ImpedeBlocks(int count, Point *points, unsigned int value)
{
	for(int i=0;i<count;i++) {
		unsigned int tmp = area->SearchMap->GetPixelIndex( points[i].x, points[i].y ) & PATH_MAP_NOTDOOR;
		area->SearchMap->SetPixelIndex( points[i].x, points[i].y, (ieByte) (tmp|value) );
	}
}

void Door::UpdateDoor()
{
	if (Flags&DOOR_OPEN) {
		outline = open;
	} else {
		outline = closed;
	}
	unsigned int oval, cval;

	oval = PATH_MAP_IMPASSABLE; 
	if (Flags & DOOR_TRANSPARENT) {
		cval = PATH_MAP_DOOR_TRANSPARENT;
	}
	else {
		cval = PATH_MAP_DOOR_OPAQUE;
	}
	if (Flags &DOOR_OPEN) {
		ImpedeBlocks(cibcount, closed_ib, 0);
		ImpedeBlocks(oibcount, open_ib, cval);
	}
	else {
		ImpedeBlocks(oibcount, open_ib, 0);
		ImpedeBlocks(cibcount, closed_ib, cval);
	}

	InfoPoint *ip=area->TMap->GetInfoPoint(LinkedInfo);
	if (ip) {
		if (Flags&DOOR_OPEN) ip->Flags&=~INFO_DOOR;
		else ip->Flags|=INFO_DOOR;
	}
}

void Door::ToggleTiles(int State, bool playsound)
{
	int i;
	int state;

	if (State) {
		state = !closedIndex;
		if (playsound && ( OpenSound[0] != '\0' ))
			core->GetAudioDrv()->Play( OpenSound );
	} else {
		state = closedIndex;
		if (playsound && ( CloseSound[0] != '\0' ))
			core->GetAudioDrv()->Play( CloseSound );
	}
	for (i = 0; i < tilecount; i++) {
		overlay->tiles[tiles[i]]->tileIndex = (ieByte) state;
	}

	//set door_open as state
	Flags = (Flags & ~DOOR_OPEN) | (State==!core->HasFeature(GF_REVERSE_DOOR) );
}

//this is the short name (not the scripting name)
void Door::SetName(const char* name)
{
	strnlwrcpy( ID, name, 8 );
}

void Door::SetTiles(unsigned short* Tiles, int cnt)
{
	if (tiles) {
		free( tiles );
	}
	tiles = Tiles;
	tilecount = cnt;
}

void Door::SetDoorLocked(bool Locked, bool playsound)
{
	if (Locked) {
		if (Flags & DOOR_LOCKED) return;
		Flags|=DOOR_LOCKED;
		if (playsound && ( LockSound[0] != '\0' ))
			core->GetAudioDrv()->Play( LockSound );
	}
	else {
		if (!(Flags & DOOR_LOCKED)) return;
		Flags&=~DOOR_LOCKED;
		if (playsound && ( UnLockSound[0] != '\0' ))
			core->GetAudioDrv()->Play( UnLockSound );
	}
}

bool Door::IsOpen() const
{
	bool ret = (bool) core->HasFeature(GF_REVERSE_DOOR);
	if (Flags&DOOR_OPEN) {
		ret=!ret;
	}
	return ret;
}

//also mark actors to fix position
bool Door::BlockedOpen(bool Open, bool ForceOpen)
{
	bool blocked;
	int count;
	Point *points;

	blocked = false;
	if (Open) {
		count=oibcount;
		points=open_ib;
	} else {
		count=cibcount;
		points=closed_ib;
	}
	//getting all impeded actors flagged for jump
	Region rgn;
	rgn.w=16;
	rgn.h=12;
	for(int i=0;i<count;i++) {
		Actor** ab;
		rgn.x=points[i].x*16;
		rgn.y=points[i].y*12;
		unsigned int tmp = area->SearchMap->GetPixelIndex( points[i].x, points[i].y ) & PATH_MAP_ACTOR;
		if (tmp) {
			int ac = area->GetActorInRect(ab, rgn, false);
			while(ac--) {
				if (ab[ac]->GetBase(IE_DONOTJUMP)) {
					continue;
				}
				ab[ac]->SetBase(IE_DONOTJUMP, DNJ_JUMP);
				blocked = true;
			}
			if (ab) {
				free(ab);
			}
		}
	}

	if ((Flags&DOOR_SLIDE) || ForceOpen) {
		return false;
	}
	return blocked;
}

void Door::SetDoorOpen(bool Open, bool playsound, ieDword ID)
{
	if (playsound) {
		//the door cannot be blocked when opening,
		//but the actors will be pushed
		//BlockedOpen will mark actors to be pushed
		if (BlockedOpen(Open,0) && !Open) {
			//clear up the blocking actors
			area->JumpActors(false);
			return;
		}
		area->JumpActors(true);
	}
	if (Open) {
		LastEntered = ID; //used as lastOpener
		SetDoorLocked(false,playsound);
	} else {
		LastTrigger = ID; //used as lastCloser
	}
	ToggleTiles(Open, playsound);
	//synchronising other data with the door state
	UpdateDoor();
	area->ActivateWallgroups(open_wg_index, open_wg_count, Flags&DOOR_OPEN);
	area->ActivateWallgroups(closed_wg_index, closed_wg_count, !(Flags&DOOR_OPEN));
}

void Door::SetPolygon(bool Open, Gem_Polygon* poly)
{
	if (Open) {
		if (open)
			delete( open );
		open = poly;
	} else {
		if (closed)
			delete( closed );
		closed = poly;
	}
}

void Door::TryDisarm(Actor *actor)
{
//first lets do this automatically succeeding
//TODO: skill check, set off
	Trapped = 0;
	TrapDetected = 0;
	LastDisarmed = actor->GetID();
	ImmediateEvent();
}

void Door::TryPickLock(Actor *actor)
{
	if (Trapped) {
		LastPickLockFailed = actor->GetID();
		LastTrigger = actor->GetID();
		TrapDetected = 1;
		//trap fired
		if (!(Flags & DOOR_RESET) ) {
			//trap removed
			Trapped = 0;
		}
		ImmediateEvent();
		return;
	}
	if (actor->GetStat(IE_LOCKPICKING)<LockDifficulty) {
		if (LockDifficulty==100) {
			core->DisplayConstantStringName(STR_DOOR_NOPICK, 0xffffff, actor);
		} else {
			core->DisplayConstantStringName(STR_DOOR_CANTPICK, 0xffffff, actor);
		}
		return;
	}
	SetDoorLocked( false, true);
	core->DisplayConstantStringName(STR_LOCKPICK_DONE, 0xfffff, actor);
	LastUnlocked = actor->GetID();
	ImmediateEvent();
	//add XP ?
}

void Door::DebugDump()
{
	printf( "Debugdump of Door %s:\n", GetScriptName() );
	printf( "Door Open: %s\n", YESNO(IsOpen()));
	printf( "Door Locked: %s\n", YESNO(Flags&DOOR_LOCKED));
	printf( "Door Trapped: %s\n", YESNO(Trapped));
	if (Trapped) {
		printf( "Trap Permanent: %s Detectable: %s\n", YESNO(Flags&DOOR_RESET), YESNO(Flags&DOOR_DETECTABLE) );
	}
	printf( "Secret door: %s (Found: %s)\n", YESNO(Flags&DOOR_SECRET),YESNO(Flags&DOOR_FOUND));
	const char *Key = GetKey();
	const char *name = "NONE";
	if (Scripts[0]) {
		name = Scripts[0]->GetName();
	}
	printf( "Script: %s, Key (%s) removed: %s\n", name, Key?Key:"NONE", YESNO(Flags&DOOR_KEY) );
}

/*******************
 * InfoPoint Class *
 *******************/

InfoPoint::InfoPoint(void)
	: Highlightable( ST_TRIGGER )
{
	Destination[0] = 0;
	EntranceName[0] = 0;
	Flags = 0;
	TrapDetectionDiff = 0;
	TrapRemovalDiff = 0;
	TrapDetected = 0;
	TrapLaunch.empty();
	Dialog[0] = 0;
}

InfoPoint::~InfoPoint(void)
{
}

//checks if the actor may use this travel trigger
//bit 1 : can use
//bit 2 : whole team
int InfoPoint::CheckTravel(Actor *actor)
{
	if (!(Flags&TRAP_RESET)) return CT_CANTMOVE; //experimental hack
	if (Flags&TRAP_DEACTIVATED) return CT_CANTMOVE;
	if (!actor->InParty && (Flags&TRAVEL_NONPC) ) return CT_CANTMOVE;
	if (Flags&TRAVEL_PARTY) {
		if (core->HasFeature(GF_TEAM_MOVEMENT) || core->GetGame()->EveryoneNearPoint(actor->GetCurrentArea(), actor->Pos, ENP_CANMOVE) ) {
			return CT_WHOLE;
		}
		return CT_GO_CLOSER;
	}
	if(actor->IsSelected() )
	{
		if(core->GetGame()->EveryoneNearPoint(actor->GetCurrentArea(), actor->Pos, ENP_CANMOVE|ENP_ONLYSELECT) )
		{
			return CT_MOVE_SELECTED;
		}
		return CT_SELECTED;
	}
	return CT_ACTIVE;
}

void InfoPoint::TryDisarm(Actor *actor)
{
//first lets do this automatically succeeding
//TODO: skill check, set off
	Trapped = 0;
	TrapDetected = 0;
	LastDisarmed = actor->GetID();
	ImmediateEvent();
}

//detect this trap, using a skill, skill could be set to 256 for 'sure'
//skill is the all around modified trap detection skill 
//a trapdetectiondifficulty of 100 means impossible detection short of a spell
void InfoPoint::DetectTrap(int skill)
{
	if (Type!=ST_TRIGGER) return;
	if (!Scripts[0]) return;
	if (Flags&(TRAP_DEACTIVATED|TRAP_INVISIBLE) ) return;
	if ((skill>=100) && (skill!=256) ) skill=100;
	if (skill/2+core->Roll(1,skill/2,0)>TrapDetectionDiff) {
		TrapDetected=1; //probably could be set to the player #?
	}
}

//trap that is visible on screen (marked by red)
//if TrapDetected is a bitflag, we could show traps selectively for
//players, really nice for multiplayer
bool InfoPoint::VisibleTrap(bool see_all)
{
	if (Type!=ST_TRIGGER) return false;
	if (!Scripts[0]) return false;
	if (Flags&(TRAP_DEACTIVATED|TRAP_INVISIBLE) ) return false;
	if (see_all) return true;
	if (TrapDetected ) return true;
	return false;
}

//trap that will fire now
bool InfoPoint::TriggerTrap(int skill, ieDword ID)
{
	if (Type!=ST_PROXIMITY) {
		return true;
	}
	//actually this could be script name[0]
	if (!Scripts[0]) {
		return false;
	}
	if (Flags&TRAP_DEACTIVATED) {
		return false;
	}
	if (Flags&TRAP_DETECTABLE) {
		TrapDetected=1; //probably too late :)
		if ((skill>=100) && (skill!=256) ) skill=100;
		if (skill/2+core->Roll(1,skill/2,0)>TrapDetectionDiff) {
			//tumble???
			return false;
		}
	}
	LastTrigger = LastEntered = ID;
	ImmediateEvent();
	if (Flags&TRAP_RESET) {
		return true;
	}
	if (Trapped) {
		Flags|=TRAP_DEACTIVATED;
	}
	return true;
}

bool InfoPoint::Entered(Actor *actor)
{
	if (outline->PointIn( actor->Pos ) ) {
		//don't trigger again for this actor
		if (!(actor->GetInternalFlag()&IF_INTRAP)) {
			goto check;
		}
	}
	if (Distance(Pos, actor->Pos)<MAX_OPERATING_DISTANCE) {
		goto check;
	}
	if (Flags&TRAP_USEPOINT) {
		if (Distance(UsePoint, actor->Pos)<MAX_OPERATING_DISTANCE) {
			goto check;
		}
	}
	return false;
check:
	if (actor->InParty || (Flags&TRAP_NPC) ) {
		//skill?
		if (TriggerTrap(0, actor->GetID()) ) {
			return true;
		}
	}
	return false;
}

void InfoPoint::DebugDump()
{
	switch (Type) {
		case ST_TRIGGER:
			printf( "Debugdump of InfoPoint Region %s:\n", GetScriptName() );
			break;
		case ST_PROXIMITY:
			printf( "Debugdump of Trap Region %s:\n", GetScriptName() );
			break;
		case ST_TRAVEL:
			printf( "Debugdump of Travel Region %s:\n", GetScriptName() );
			break;
		default:
			printf( "Debugdump of Unsupported Region %s:\n", GetScriptName() );
			break;
	}
	printf( "TrapDetected: %d, Trapped: %s\n", TrapDetected, YESNO(Trapped));
	printf( "Trap detection: %d%%, Trap removal: %d%%\n", TrapDetectionDiff,
		TrapRemovalDiff );
	const char *name = "NONE";
	if (Scripts[0]) {
		name = Scripts[0]->GetName();
	}
	printf( "Script: %s, Key: %s, Dialog: %s\n", name, KeyResRef, Dialog );
	printf( "Active: %s\n", YESNO(InternalFlags&IF_ACTIVE));
}

/*******************
 * Container Class *
 *******************/

Container::Container(void)
	: Highlightable( ST_CONTAINER )
{
	Type = 0;
	LockDifficulty = 0;
	Flags = 0;
	TrapDetectionDiff = 0;
	TrapRemovalDiff = 0;
	Trapped = 0;
	TrapDetected = 0;
	inventory.SetInventoryType(INVENTORY_HEAP);
	// NULL should be 0 for this 
	memset (groundicons, 0, sizeof(groundicons) );
	groundiconcover = 0;
}

void Container::FreeGroundIcons()
{
	Video* video = core->GetVideoDriver();

	for (int i=0;i<MAX_GROUND_ICON_DRAWN;i++) {
		if (groundicons[i]) {
			video->FreeSprite( groundicons[i] );
			groundicons[i]=NULL;
		}
	}
	delete groundiconcover;
	groundiconcover = 0;
}

Container::~Container()
{
	FreeGroundIcons();
}

void Container::DrawPile(bool highlight, Region screen, Color tint)
{
	Video* video = core->GetVideoDriver();
	CreateGroundIconCover();
	for (int i=0;i<MAX_GROUND_ICON_DRAWN;i++) {
		if (groundicons[i]) {
			//draw it with highlight
			video->BlitGameSprite(groundicons[i],
				screen.x + Pos.x, screen.y + Pos.y,
				BLIT_TINTED | (highlight ? 0:BLIT_NOSHADOW),
				tint, groundiconcover);
		}
	}
}

// create the SpriteCover for the groundicons
void Container::CreateGroundIconCover()
{
	int xpos = 0;
	int ypos = 0;
	int width = 0;
	int height = 0;

	int i; //msvc6.0
	for (i=0;i<MAX_GROUND_ICON_DRAWN;i++) {
		if (groundicons[i]) {
			Sprite2D& spr = *groundicons[i];
			if (xpos < spr.XPos) {
				width += spr.XPos - xpos;
				xpos = spr.XPos;
			}
			if (ypos < spr.YPos) {
				height += spr.YPos - ypos;
				ypos = spr.YPos;
			}
			if (width-xpos < spr.Width-spr.XPos) {
				width = spr.Width-spr.XPos+xpos;
			}
			if (height-ypos < spr.Height-spr.YPos) {
				height = spr.Height-spr.YPos+ypos;
			}
		}
	}

	if (!groundiconcover ||
		!groundiconcover->Covers(Pos.x, Pos.y, xpos, ypos, width, height))
	{
		delete groundiconcover;
		groundiconcover = 0;
		if (width*height > 0) {
			groundiconcover = GetCurrentArea()->BuildSpriteCover
				(Pos.x, Pos.y, xpos, ypos, width, height, WantDither());
		}
	}

	// TODO: remove this checking code eventually
	for (i=0;i<MAX_GROUND_ICON_DRAWN;i++) {
		if (groundicons[i]) {
			Sprite2D& spr = *groundicons[i];
			assert(groundiconcover->Covers(Pos.x, Pos.y, spr.XPos, spr.YPos, spr.Width, spr.Height));
		}
	}
}

void Container::SetContainerLocked(bool lock)
{
	if (lock) {
		Flags|=CONT_LOCKED;
	} else {
		Flags&=~CONT_LOCKED;
	}
}

//This function doesn't exist in the original IE, destroys a container
//turning it to a ground pile
void Container::DestroyContainer()
{
	//it is already a groundpile?
	if (Type==IE_CONTAINER_PILE)
		return;
	Type = IE_CONTAINER_PILE;
	RefreshGroundIcons();
	//probably we should stop the script or trigger it, whatever
}

//Takes an item from the container's inventory and returns its pointer
CREItem *Container::RemoveItem(unsigned int idx, unsigned int count)
{
	CREItem *ret = inventory.RemoveItem(idx, count);
	//we just took the 3. or less item, groundpile changed
	if ((Type==IE_CONTAINER_PILE) && (inventory.GetSlotCount()<3)) {
		RefreshGroundIcons();
	}
	return ret;
}

//Adds an item to the container's inventory
//containers always have enough capacity (so far), thus we always return 2
int Container::AddItem(CREItem *item)
{
	inventory.AddItem(item);
	//we just added a 3. or less item, groundpile changed
	if ((Type==IE_CONTAINER_PILE) && (inventory.GetSlotCount()<4)) {
		RefreshGroundIcons();
	}
	return 2;
}

void Container::RefreshGroundIcons()
{
	int i = inventory.GetSlotCount();
	if (i>MAX_GROUND_ICON_DRAWN)
		i=MAX_GROUND_ICON_DRAWN;
	FreeGroundIcons();
	while (i--) {
		CREItem *slot = inventory.GetSlotItem(i); //borrowed reference
		Item *itm = gamedata->GetItem( slot->ItemResRef ); //cached reference
		//well, this is required in PST, needs more work if some other
		//game is broken by not using -1,0
		groundicons[i] = gamedata->GetBAMSprite( itm->GroundIcon, 0, 0 );
		gamedata->FreeItem( itm, slot->ItemResRef ); //decref
	}
}

//used for ground piles
int Container::WantDither()
{
	//if pile is highlighted, always dither it
	if (Highlight) {
		return 2; //dither me if you want
	}
	//if pile isn't highlighted, dither it if the polygon wants
	return 1;
}

bool Container::IsOpen() const
{
	if (Flags&CONT_LOCKED) {
		return false;
	}
	return true;
}

void Container::TryDisarm(Actor *actor)
{
//first lets do this automatically succeeding
//TODO: skill check, set off
	Trapped = 0;
	TrapDetected = 0;
	LastDisarmed = actor->GetID();
	ImmediateEvent();
}

void Container::TryPickLock(Actor *actor)
{
	if (Trapped) {
		LastPickLockFailed = actor->GetID();
		LastTrigger = actor->GetID();
		TrapDetected = 1;
		//trap fired
		if (!(Flags & CONT_RESET) ) {
			//trap removed
			Trapped = 0;
		}
		ImmediateEvent();
		return;
	}
	if (actor->GetStat(IE_LOCKPICKING)<LockDifficulty) {
		if (LockDifficulty==100) {
			core->DisplayConstantStringName(STR_CONT_NOPICK, 0xffffff, actor);
		} else {
			core->DisplayConstantStringName(STR_CONT_CANTPICK, 0xffffff, actor);
		}
		return;
	}
	SetContainerLocked(false);
	core->DisplayConstantStringName(STR_LOCKPICK_DONE, 0xfffff, actor);
	LastUnlocked = actor->GetID();
	ImmediateEvent();
	//add XP ?
}

void Container::DebugDump()
{
	printf( "Debugdump of Container %s\n", GetScriptName() );
	printf( "Type: %d,  LockDifficulty: %d\n", Type, LockDifficulty );
	printf( "Flags: %d, Trapped: %s, Detected: %d\n", Flags, YESNO(Trapped), TrapDetected );
	printf( "Trap detection: %d%%, Trap removal: %d%%\n", TrapDetectionDiff,
		TrapRemovalDiff );
	const char *name = "NONE";
	if (Scripts[0]) {
		name = Scripts[0]->GetName();
	}
	printf( "Script: %s, Key: %s\n", name, KeyResRef );
}
