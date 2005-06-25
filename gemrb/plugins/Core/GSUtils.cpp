/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003-2005 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/GSUtils.cpp,v 1.6 2005/06/25 08:50:19 avenger_teambg Exp $
 *
 */

#include "GSUtils.h"
#include "Interface.h"
#include "../../includes/strrefs.h"
#include "../../includes/defsounds.h"

int initialized = 0;
SymbolMgr* triggersTable;
SymbolMgr* actionsTable;
SymbolMgr* objectsTable;
TriggerFunction triggers[MAX_TRIGGERS];
ActionFunction actions[MAX_ACTIONS];
short actionflags[MAX_ACTIONS];
short triggerflags[MAX_TRIGGERS];
ObjectFunction objects[MAX_OBJECTS];
IDSFunction idtargets[MAX_OBJECT_FIELDS];
Cache SrcCache; //cache for string resources (pst)
Cache BcsCache; //cache for scripts
int ObjectIDSCount = 7;
int MaxObjectNesting = 5;
bool HasAdditionalRect = false;
ieResRef *ObjectIDSTableNames;
int ObjectFieldsCount = 7;
int ExtraParametersCount = 0;
int InDebug = 0;
int happiness[3][20];
int RandomNumValue;

void InitScriptTables()
{
	//initializing the happiness table
	int hptable = core->LoadTable( "happy" );
	TableMgr *tab = core->GetTable( hptable );
	for (int alignment=0;alignment<3;alignment++) {
		for (int reputation=0;reputation<20;reputation++) {
			happiness[alignment][reputation]=strtol(tab->QueryField(reputation,alignment), NULL, 0);
		}
	}
	core->DelTable( hptable );

	//
}

void HandleBitMod(ieDword &value1, ieDword value2, int opcode)
{
	switch(opcode) {
		case BM_AND:
			value1 = ( value1& value2 );
			break;
		case BM_OR:
			value1 = ( value1| value2 );
			break;
		case BM_XOR:
			value1 = ( value1^ value2 );
			break;
		case BM_NAND: //this is a GemRB extension
			value1 = ( value1& ~value2 );
			break;
		case BM_SET: //this is a GemRB extension
			value1 = value2;
			break;
	}
}

void DisplayStringCore(Scriptable* Sender, int Strref, int flags)
{
	printf( "Displaying string on: %s\n", Sender->GetScriptName() );
	if (flags & DS_CONST ) {
		Actor* actor = ( Actor* ) Sender;
		Strref=actor->StrRefs[Strref];
	}
	StringBlock sb = core->strings->GetStringBlock( Strref );
	if (flags & DS_HEAD) {
		Sender->DisplayHeadText( sb.text );
	}
	if (flags & DS_CONSOLE) {
			core->DisplayString( sb.text );
	}
	if (sb.Sound[0] ) {
		ieDword len = core->GetSoundMgr()->Play( sb.Sound );
		ieDword counter = ( AI_UPDATE_TIME * len ) / 1000;
		if ((counter != 0) && (flags &DS_WAIT) )
			Sender->SetWait( counter );
	}
}

int CanSee(Scriptable* Sender, Scriptable* target)
{
	Map *map;
	unsigned int range;

	if (Sender->Type == ST_ACTOR) {
		Actor* snd = ( Actor* ) Sender;
		range = snd->Modified[IE_VISUALRANGE] * 20;
		map = Sender->GetCurrentArea();
		//huh, lets hope it won't crash often
		if (!map) {
			abort();
		}
	}
	else { 
		map = Sender->GetCurrentArea();
		range = 20 * 20;
	}
	if ( target->GetCurrentArea()!=map ) {
		return 0;
	}

	if (Distance(target->Pos, Sender->Pos) > range) {
		return 0;
	}
	return map->IsVisible(target->Pos, Sender->Pos);
}

int ValidForDialogCore(Scriptable* Sender, Actor *target)
{
	if (!CanSee(Sender, target) ) {
		return 0;
	}
	
	//we should rather use STATE_SPEECHLESS_MASK
	if (target->GetStat(IE_STATE_ID)&STATE_DEAD) {
		return 0;
	}
	return 1;
}

//transfering item from Sender to target, target must be an actor
//if target can't get it, it will be dropped at its feet
int MoveItemCore(Scriptable *Sender, Scriptable *target, const char *resref, int flags)
{
	Inventory *myinv;
	Map *map;

	if (!target || target->Type!=ST_ACTOR) {
		return MIC_INVALID;
	}
	map=Sender->GetCurrentArea();
	switch(Sender->Type) {
		case ST_ACTOR:
			myinv=&((Actor *) Sender)->inventory;
			break;
		case ST_CONTAINER:
			myinv=&((Container *) Sender)->inventory;
			break;
		default:
			return MIC_INVALID;
	}
	Actor *scr = (Actor *) target;
	CREItem *item;
	scr->inventory.RemoveItem(resref, flags, &item);
	if (!item)
		return MIC_NOITEM;
	if ( 2 != myinv->AddSlotItem(item, -1)) {
		// drop it at my feet
		map->AddItemToLocation(Sender->Pos, item);
		return MIC_FULL;
	}
	return MIC_GOTITEM;
}

/* returns actors that match the [x.y.z] expression */
static Targets* EvaluateObject(Scriptable* Sender, Object* oC)
{
	Map *map=Sender->GetCurrentArea();
	Targets *tgts=NULL;

	if (oC->objectName[0]) {
		//We want the object by its name... (doors/triggers don't play here!)
		Actor* aC = map->GetActor( oC->objectName );
		if (!aC) {
			return tgts;
		}
		//Ok :) we now have our Object. Let's create a Target struct and add the object to it
		tgts = new Targets();
		tgts->AddTarget( aC, 0 );
		//return here because object name/IDS targeting are mutually exclusive
		return tgts;
	}
	//else branch, IDS targeting
	for (int j = 0; j < ObjectIDSCount; j++) {
		if (!oC->objectFields[j]) {
			continue;
		}
		IDSFunction func = idtargets[j];
		if (!func) {
			printf("Unimplemented IDS targeting opcode!\n");
			continue;
		}
		if (tgts) {
			//we already got a subset of actors
			int i = tgts->Count();
			/*premature end, filtered everything*/
			if (!i) {
				break; //leaving the loop
			}
			targetlist::iterator m;
			targettype *t = tgts->GetFirstTarget(m);
			while (t) {
				if (!func(t->actor, oC->objectFields[j] ) ) {
					t = tgts->RemoveTargetAt(m);
				} else {
					t = tgts->GetNextTarget(m);
				}
			}
		}
		else {
			//we need to get a subset of actors from the large array
			//if this gets slow, we will need some index tables
			int i = map->GetActorCount(true);
			tgts = new Targets();
			while (i--) {
				Actor *ac=map->GetActor(i,true);
				int dist = Distance(Sender->Pos, ac->Pos);
				if (ac && func(ac, oC->objectFields[j]) ) {
					tgts->AddTarget(ac, dist);
				}
			}
		}
	}
	return tgts;
}

Scriptable* GetActorFromObject(Scriptable* Sender, Object* oC)
{
	if (!oC) {
		return NULL;
	}
	Targets* tgts = EvaluateObject(Sender, oC);
	if (!tgts && oC->objectName[0]) {
		//It was not an actor... maybe it is a door?
		Scriptable * aC = Sender->GetCurrentArea()->TMap->GetDoor( oC->objectName );
		if (aC) {
			return aC;
		}
		//No... it was not a door... maybe an InfoPoint?
		aC = Sender->GetCurrentArea()->TMap->GetInfoPoint( oC->objectName );
		if (aC) {
			return aC;
		}

		//No... it was not an infopoint... maybe a Container?
		aC = Sender->GetCurrentArea()->TMap->GetContainer( oC->objectName );
		if (aC) {
			return aC;
		}
		return NULL;
	}
	//now lets do the object filter stuff, we create Targets because
	//it is possible to start from blank sheets using endpoint filters
	//like (Myself, Protagonist etc)
	if (!tgts) {
		tgts = new Targets();
	}
	for (int i = 0; i < MaxObjectNesting; i++) {
		int filterid = oC->objectFilters[i];
		if (!filterid) {
			break;
		}
		ObjectFunction func = objects[filterid];
		if (func) {
			tgts = func( Sender, tgts);
		}
		else {
			printf("[GameScript]: Unknown object filter: %d %s\n",filterid, objectsTable->GetValue(filterid) );
		}
		if (!tgts->Count()) {
			delete tgts;
			return NULL;
		}
	}
	if (tgts) {
		Scriptable *object;
		object= (Scriptable *) tgts->GetTarget(0);
		delete tgts;
		return object;
	}
	return NULL;
}

/*FIXME: what is 'base'*/
void PolymorphCopyCore(Actor *src, Actor *tar, bool /*base*/)
{
	tar->SetStat(IE_ANIMATION_ID, src->GetStat(IE_ANIMATION_ID) );
	//add more attribute copying
}

void CreateCreatureCore(Scriptable* Sender, Action* parameters, int flags)
{
	//ActorMgr* aM = ( ActorMgr* ) core->GetInterface( IE_CRE_CLASS_ID );
	DataStream* ds;

	if (flags & CC_STRING1) {
		ds = core->GetResourceMgr()->GetResource( parameters->string1Parameter, IE_CRE_CLASS_ID );
	}
	else {
		ds = core->GetResourceMgr()->GetResource( parameters->string0Parameter, IE_CRE_CLASS_ID );
	}
	//aM->Open( ds, true );
	//Actor* ab = aM->GetActor();
	//core->FreeInterface( aM );
	//GetCreature will close the datastream on its own
	Actor *ab = core->GetCreature(ds);
	int radius;
	Point pnt;

	radius=0;
	switch (flags & CC_MASK) {
		case CC_OFFSCREEN:
			{
			Region vp = core->GetVideoDriver()->GetViewport();
			radius=vp.w/2; //actually it must be further divided by the tile size, hmm 16?
			}
			//falling through
		case CC_OBJECT://use object + offset
			{
			Scriptable *tmp = GetActorFromObject( Sender, parameters->objects[1] );
			if (tmp) Sender=tmp;
			}
			//falling through
		case CC_OFFSET://use sender + offset
			pnt.x = parameters->pointParameter.x+Sender->Pos.x;
			pnt.y = parameters->pointParameter.y+Sender->Pos.y;
			break;
		default: //absolute point, but -1,-1 means AtFeet
			pnt.x = parameters->pointParameter.x;
			pnt.y = parameters->pointParameter.y;
			if (pnt.isempty()) {
				pnt.x = Sender->Pos.x;
				pnt.y = Sender->Pos.y;
			}
			break;
	}

	printf("CreateCreature: %s at [%d.%d] face:%d\n",parameters->string0Parameter, pnt.x,pnt.y,parameters->int0Parameter);
	Map *map = Sender->GetCurrentArea();
	ab->SetPosition(map, pnt, flags&CC_CHECK_IMPASSABLE, radius );
	//i think this isn't needed, the creature's stance should be set in
	//the creature, GetActor sets it correctly
	//ab->SetStance( IE_ANI_AWAKE );
	ab->SetOrientation(parameters->int0Parameter, 0 );
	map->AddActor( ab );

	//setting the deathvariable if it exists (iwd2)
	if (parameters->string1Parameter[0]) {
		ab->SetScriptName(parameters->string1Parameter);
	}
}

void CreateVisualEffectCore(Scriptable *Sender, Point &position, const char *effect)
{
//TODO: add engine specific VVC replacement methods
//stick to object flag, sounds, iterations etc.
	ScriptedAnimation* vvc = core->GetScriptedAnimation(effect, position);
	Sender->GetCurrentArea( )->AddVVCCell( vvc );
}

void ChangeAnimationCore(Actor *src, const char *resref, bool effect)
{
	DataStream* ds = core->GetResourceMgr()->GetResource( resref, IE_CRE_CLASS_ID );
	Actor *tar = core->GetCreature(ds);
	if (tar) {
		Map *map = src->GetCurrentArea();
		tar->SetPosition(map, src->Pos, 1);
		src->InternalFlags|=IF_CLEANUP;
		if (effect) {
			CreateVisualEffectCore(tar, tar->Pos,"smokepuffeffect");
		}
	}
}

void EscapeAreaCore(Actor* src, const char* resref, Point &enter, Point &exit, int flags)
{
	src->ClearActions();
	char Tmp[256];
	sprintf( Tmp, "MoveToPoint([%hd.%hd])", exit.x, exit.y );
	src->AddAction( GenerateAction( Tmp, true ) );
	src->SetWait(5);
	if (flags &EA_DESTROY) {
		src->AddAction( GenerateAction("DestroySelf()") );
	} else {
		sprintf( Tmp, "JumpToPoint(\"%s\",[%hd.%hd])", resref, enter.x, enter.y );
		src->AddAction( GenerateAction( Tmp, true ) );
	}
}

void GetPositionFromScriptable(Scriptable* scr, Point &position, bool trap)
{
	if (!trap) {
		position = scr->Pos;
		return;
	}
	switch (scr->Type) {
		case ST_AREA: case ST_GLOBAL: case ST_ACTOR:
			position = scr->Pos;
			break;
		case ST_TRIGGER: case ST_PROXIMITY: case ST_TRAVEL:
		case ST_DOOR: case ST_CONTAINER:
			position=((Highlightable *) scr)->TrapLaunch;
	}
}

static const char *GetDialog(Scriptable* scr)
{
	switch(scr->Type) {
		case ST_CONTAINER: case ST_DOOR:
		case ST_PROXIMITY: case ST_TRAVEL: case ST_TRIGGER:
			return ((Highlightable *) scr)->GetDialog();
		case ST_ACTOR:
			return ((Actor *) scr)->GetDialog(true);
		case ST_GLOBAL:case ST_AREA:
			return NULL;
	}
	return NULL;
}

static ieResRef PlayerDialogRes = "PLAYERx\0";

void BeginDialog(Scriptable* Sender, Action* parameters, int Flags)
{
	Scriptable* tar, *scr;

	if (InDebug&ID_VARIABLES) {
		printf("BeginDialog core\n");
	}
	if (Flags & BD_OWN) {
		scr = tar = GetActorFromObject( Sender, parameters->objects[1] );
	} else {
		if (Flags & BD_NUMERIC) {
			//the target was already set, this is a crude hack
			tar = core->GetGameControl()->target;
		}
		else {
			tar = GetActorFromObject( Sender, parameters->objects[1] );
		}
		scr = Sender;
	}
	if (!tar) {
		printf("[GameScript]: Target for dialog couldn't be found (Sender: %s, Type: %d).\n", Sender->GetScriptName(), Sender->Type);
		if (Sender->Type == ST_ACTOR) {
			((Actor *) Sender)->DebugDump();
		}
		//parameters->Dump();
		printf ("Target object: ");
		if (parameters->objects[1]) {
			parameters->objects[1]->Dump();
		} else {
			printf("<NULL>\n");
		}
		Sender->CurrentAction = NULL;
		return;
	}

	//target could be other than Actor, we need to handle this too!
	//if (scr->Type != ST_ACTOR) {
	//	Sender->CurrentAction = NULL;
	//	return;
	//}
	//CHECKDIST works only for mobile scriptables
	if ((Flags&BD_CHECKDIST) && (Sender->Type==ST_ACTOR) ) {
		Actor *actor = (Actor *) Sender;
		if (Distance(Sender, tar)>actor->GetStat(IE_DIALOGRANGE)*20 ) {
			GoNearAndRetry(Sender, tar);
			Sender->CurrentAction = NULL;
			return;
		}
	}
	//making sure speaker is the protagonist, player, actor
	bool swap = false;
	if (scr->Type != ST_ACTOR) swap = true;
	else if (tar->Type == ST_ACTOR) {
		if ( ((Actor *) tar)->InParty == 1) swap = true;
		else if ( (((Actor *) scr)->InParty !=1) && ((Actor *) tar)->InParty) swap = true;
	}

	GameControl* gc = core->GetGameControl();
	if (!gc) {
		printMessage( "GameScript","Dialog cannot be initiated because there is no GameControl.", YELLOW );
		Sender->CurrentAction = NULL;
		return;
	}
	//can't initiate dialog, because it is already there
	if (gc->GetDialogueFlags()&DF_IN_DIALOG) {
		if (Flags & BD_INTERRUPT) {
			//break the current dialog if possible
			gc->EndDialog(true);
		}
		//check if we could manage to break it, not all dialogs are breakable!
		if (gc->GetDialogueFlags()&DF_IN_DIALOG) {
			printMessage( "GameScript","Dialog cannot be initiated because there is already one.", YELLOW );
			Sender->CurrentAction = NULL;
			return;
		}
	}

	const char* Dialog = NULL;
	int pdtable = -1;

	switch (Flags & BD_LOCMASK) {
		case BD_STRING0:
			Dialog = parameters->string0Parameter;
			if (Flags & BD_SETDIALOG) {
				if ( scr->Type == ST_ACTOR) {
					Actor* actor = ( Actor* ) scr;
					actor->SetDialog( Dialog );
				}
			}
			break;
		case BD_SOURCE:
//			Dialog = GetDialog(scr); //actor->Dialog;
//			break;
		case BD_TARGET:
			if (swap) Dialog = GetDialog(scr);
			else Dialog = GetDialog(tar);//target->Dialog;
			break;
		case BD_RESERVED:
			//what if playerdialog was initiated from Player2?
			PlayerDialogRes[5] = '1';
			Dialog = ( const char * ) PlayerDialogRes;
			break;
		case BD_INTERACT: //using the source for the dialog
			if ( scr->Type == ST_ACTOR) {
				pdtable = core->LoadTable( "interdia" );
				const char* scriptingname = ((Actor *) scr)->GetScriptName();
				//Dialog is a borrowed reference, we cannot free pdtable while it is being used
				Dialog = core->GetTable( pdtable )->QueryField( scriptingname, "FILE" );
			}
			break;
	}

	//maybe we should remove the action queue, but i'm unsure
	Sender->CurrentAction = NULL;

	//dialog is not meaningful
	if (!Dialog || Dialog[0]=='*') {
		goto end_of_quest;
	}

	//we also need to freeze active scripts during a dialog!
	if (scr!=tar) {
		if (swap) {
			Scriptable *tmp = tar;
			tar = scr;
			scr = tmp;
		}
		if (Sender!=tar) {
			if (Flags & BD_INTERRUPT) {
				scr->ClearActions();
			} else {
				if (scr->GetNextAction()) {
					core->DisplayConstantString(STR_TARGETBUSY,0xff0000);
					goto end_of_quest;
				}
			}
		}
		
	}

	if (scr->Type==ST_ACTOR) {
		((Actor *)scr)->SetOrientation(GetOrient( tar->Pos, scr->Pos),1);
		//scr->resetAction = true; //im not sure this is needed
	}
	if (tar->Type==ST_ACTOR) {
		((Actor *)tar)->SetOrientation(GetOrient( scr->Pos, tar->Pos),1);
		//tar->resetAction = true;//nor this
	}

	if (Dialog[0]) {
		//increasing NumTimesTalkedTo
		if (Flags & BD_TALKCOUNT) {
			gc->SetDialogueFlags(DF_TALKCOUNT, BM_OR);
		}

		//actually it isn't random...
//		if (Flags & BD_INTERACT) {
//			//random dialog
//			core->GetDictionary()->SetAt("DialogChoose",(ieDword) -2);
//		} else {
			//first top level
			core->GetDictionary()->SetAt("DialogChoose",(ieDword) -1);
//		}
		gc->InitDialog( (Actor *) scr, tar, Dialog);
	}
//if pdtable was allocated, free it now, it will release Dialog
end_of_quest:
	if (pdtable!=-1) {
		core->DelTable( pdtable );
	}
}


bool GameScript::MatchActor(Scriptable *Sender, Actor* actor, Object* oC)
{
	if (!actor) {
		return false;
	}
	if (oC->objectName[0]) {
		return (stricmp( actor->GetScriptName(), oC->objectName ) == 0);
	}
	Targets *tgts = new Targets();
	for (int i = 0; i < MaxObjectNesting; i++) {
		int filterid = oC->objectFilters[i];
		if (!filterid) {
			break;
		}
		ObjectFunction func = objects[filterid];
		if (func) {
			tgts = func( Sender, tgts);
		}
		else {
			printMessage("GameScript", " ", LIGHT_RED);
			printf("Unknown object filter: %d %s\n",filterid, objectsTable->GetValue(filterid) );
		}
		if (!tgts->Count()) {
			delete tgts;
			return false;
		}
	}
	bool ret = false;

	if (tgts) {
		targetlist::iterator m;
		targettype *tt = tgts->GetFirstTarget(m);
		while (tt) {
			if (tt->actor == actor) {
				ret = true;
			}
			tt = tgts->GetNextTarget(m);
		}
	}
	delete tgts;
	return ret;
}

int GameScript::GetObjectCount(Scriptable* Sender, Object* oC)
{
	if (!oC) {
		return 0;
	}
	Targets* tgts = EvaluateObject(Sender, oC);
	int count = tgts->Count();
	delete tgts;
	return count;
}

//this function returns a value, symbol could be a numeric string or
//a symbol from idsname
static int GetIdsValue(const char *&symbol, const char *idsname)
{
	int idsfile=core->LoadSymbol(idsname);
	SymbolMgr *valHook = core->GetSymbol(idsfile);
	if (!valHook) {
		//FIXME:missing ids file!!!
		if (InDebug&ID_TRIGGERS) {
			printMessage("GameScript"," ",LIGHT_RED);
			printf("Missing IDS file %s for symbol %s!\n",idsname, symbol);
		}
		return -1;
	}
	char *newsymbol;
	int value=strtol(symbol, &newsymbol, 0);
	if (symbol!=newsymbol) {
		symbol=newsymbol;
		return value;
	}
	char symbolname[64];
	int x;
	for (x=0;isalnum(*symbol) && x<(int) sizeof(symbolname)-1;x++) {
		symbolname[x]=*symbol;
		symbol++;
	}
	symbolname[x]=0;
	return valHook->GetValue(symbolname);
}

static void ParseIdsTarget(const char *&src, Object *&object)
{
	for (int i=0;i<ObjectFieldsCount;i++) {
			object->objectFields[i]=GetIdsValue(src, ObjectIDSTableNames[i]);
		if (*src!='.') {
			break;
		}
	}
	src++; //skipping ]
}

//this will skip to the next element in the prototype of an action/trigger
#define SKIP_ARGUMENT() while (*str && ( *str != ',' ) && ( *str != ')' )) str++

static void ParseObject(const char *&str,const char *&src, Object *&object)
{
	SKIP_ARGUMENT();
	object = new Object();
	switch (*src) {
	case '"':
		//Scriptable Name
		src++;
		int i;
		for (i=0;i<(int) sizeof(object->objectName)-1 && *src!='"';i++)
		{
			object->objectName[i] = *src;
			src++;
		}
		object->objectName[i] = 0;
		src++;
		break;
	case '[':
		src++; //skipping [
		ParseIdsTarget(src, object);
		break;
	default: //nested object filters
		int Nesting=0;
		
		while (Nesting<MaxObjectNesting) {
			memmove(object->objectFilters+1, object->objectFilters, (int) sizeof(int) *(MaxObjectNesting-1) );
			object->objectFilters[0]=GetIdsValue(src,"object");
			if (*src!='(') {
				break;
			}
			src++; //skipping (
			Nesting++;
		}
		if (*src=='[') {
			ParseIdsTarget(src, object);
		}
		src+=Nesting; //skipping )
	}
}

/* this function was lifted from GenerateAction, to make it clearer */
Action* GenerateActionCore(const char *src, const char *str, int acIndex, bool autoFree)
{
	Action*newAction = new Action(autoFree);
	newAction->actionID = (unsigned short) actionsTable->GetValueIndex( acIndex );
	//this flag tells us to merge 2 consecutive strings together to get
	//a variable (context+variablename)
	int mergestrings = actionflags[newAction->actionID]&AF_MERGESTRINGS;
	int objectCount = ( newAction->actionID == 1 ) ? 0 : 1;
	int stringsCount = 0;
	int intCount = 0;
	//Here is the Action; Now we need to evaluate the parameters, if any
	if (*str!=')') while (*str) {
		if (*(str+1)!=':') {
			printf("Warning, parser was sidetracked: %s\n",str);
		}
		switch (*str) {
			default:
				printf("Invalid type: %s\n",str);
				str++;
				break;

			case 'p': //Point
				SKIP_ARGUMENT();
				src++; //Skip [
				newAction->pointParameter.x = (short) strtol( src, (char **) &src, 10 );
				src++; //Skip .
				newAction->pointParameter.y = (short) strtol( src, (char **) &src, 10 );
				src++; //Skip ]
				break;

			case 'i': //Integer
			{
				//going to the variable name
				while (*str != '*' && *str !=',' && *str != ')' ) {
					str++;
				}
				int value;
				if (*str=='*') { //there may be an IDS table
					str++;
					char idsTabName[33];
					char* tmp = idsTabName;
					while (( *str != ',' ) && ( *str != ')' )) {
						*tmp = *str;
						tmp++;
						str++;
					}
					*tmp = 0;
					if (idsTabName[0]) {
						value = GetIdsValue(src, idsTabName);
					}
					else {
						value = strtol( src, (char **) &src, 0);
					}
				}
				else { //no IDS table
					value = strtol( src, (char **) &src, 0);
				}
				if (!intCount) {
					newAction->int0Parameter = value;
				} else if (intCount == 1) {
					newAction->int1Parameter = value;
				} else {
					newAction->int2Parameter = value;
				}
			}
			break;

			case 'a':
			//Action
			 {
				SKIP_ARGUMENT();
				char action[257];
				int i = 0;
				int openParenthesisCount = 0;
				while (true) {
					if (*src == ')') {
						if (!openParenthesisCount)
							break;
						openParenthesisCount--;
					} else {
						if (*src == '(') {
							openParenthesisCount++;
						} else {
							if (( *src == ',' ) &&
								!openParenthesisCount)
								break;
						}
					}
					action[i] = *src;
					i++;
					src++;
				}
				action[i] = 0;
				Action* act = GenerateAction( action, autoFree );
				act->objects[0] = newAction->objects[0];
				newAction->objects[0] = NULL; //avoid freeing of object
				delete newAction; //freeing action
				newAction = act;
			}
			break;

			case 'o': //Object
				if (objectCount==3) {
					printf("Invalid object count!\n");
					abort();
				}
				ParseObject(str, src, newAction->objects[objectCount++]);
				break;

			case 's': //String
			{
				SKIP_ARGUMENT();
				src++;
				int i;
				char* dst;
				if (!stringsCount) {
					dst = newAction->string0Parameter;
				} else {
					dst = newAction->string1Parameter;
				}
				//skipping the context part, which
				//is to be readed later
				if (mergestrings) {
					for (i=0;i<6;i++) {
						*dst++='*';
					}
				}
				else {
					i=0;
				}
				while (*src != '"') {
					//sizeof(context+name) = 40
					if (i<40) {
						*dst++ = toupper(*src);
						i++;
					}
					src++;
				}
				*dst = 0;
				//reading the context part
				if (mergestrings) {
					str++;
					if (*str!='s') {
						printf("Invalid mergestrings:%s\n",str);
						abort();
					}
					SKIP_ARGUMENT();
					if (!stringsCount) {
						dst = newAction->string0Parameter;
					} else {
						dst = newAction->string1Parameter;
					}

					//this works only if there are no spaces
					if (*src++!='"' || *src++!=',' || *src++!='"') {
						break;
					}
					//reading the context string
					i=0;
					while (*src != '"') {
						if (i++<6) {
							*dst++ = toupper(*src);
						}
						src++;
					}
				}
				src++; //skipping "
				stringsCount++;
			}
			break;
		}
		str++;
		if (*src == ',' || *src==')')
			src++;
	}
	return newAction;
}

void GoNearAndRetry(Scriptable *Sender, Scriptable *target)
{
	Sender->AddActionInFront( Sender->CurrentAction );
	char Tmp[256];
	sprintf( Tmp, "MoveToPoint([%hd.%hd])", target->Pos.x, target->Pos.y );
	Sender->AddActionInFront( GenerateAction( Tmp, true ) );
}

void GoNearAndRetry(Scriptable *Sender, Point &p)
{
	Sender->AddActionInFront( Sender->CurrentAction );
	char Tmp[256];
	sprintf( Tmp, "MoveToPoint([%hd.%hd])", p.x, p.y );
	Sender->AddActionInFront( GenerateAction( Tmp, true ) );
}

void FreeSrc(SrcVector *poi, const ieResRef key)
{
	int res = SrcCache.DecRef((void *) poi, key, true);
	if (res<0) {
		printMessage( "GameScript", "Corrupted Src cache encountered (reference count went below zero), ", LIGHT_RED );
		printf( "Src name is: %.8s\n", key);
		abort();
	}
	if (!res) {
		delete poi;	
	}
}

SrcVector *LoadSrc(const ieResRef resname)
{
	SrcVector *src = (SrcVector *) SrcCache.GetResource(resname);
	if (src) {
		return src;
	}
	DataStream* str = core->GetResourceMgr()->GetResource( resname, IE_SRC_CLASS_ID );
	if ( !str) {
		return NULL;
	}
	ieDword size=0;
	str->ReadDword(&size);
	src = new SrcVector(size);
	SrcCache.SetAt( resname, (void *) src );
	while (size--) {
		ieDword tmp;
		str->ReadDword(&tmp);
		src->at(size)=tmp;
		str->ReadDword(&tmp);
	}
	delete ( str );
	return src;
}

#define MEMCPY(a,b) memcpy((a),(b),sizeof(a) )

static Object *ObjectCopy(Object *object)
{
	if (!object) return NULL;
	Object *newObject = new Object();
	MEMCPY( newObject->objectFields, object->objectFields );
	MEMCPY( newObject->objectFilters, object->objectFilters );
	MEMCPY( newObject->objectRect, object->objectRect );
	MEMCPY( newObject->objectName, object->objectName );
	return newObject;
}

Action *ParamCopy(Action *parameters)
{
	Action *newAction = new Action(true);
	newAction->actionID = parameters->actionID;
	newAction->int0Parameter = parameters->int0Parameter;
	newAction->int1Parameter = parameters->int1Parameter;
	newAction->int2Parameter = parameters->int2Parameter;
	newAction->pointParameter = parameters->pointParameter;
	MEMCPY( newAction->string0Parameter, parameters->string0Parameter );
	MEMCPY( newAction->string1Parameter, parameters->string1Parameter );
	for (int c=0;c<3;c++) {
		newAction->objects[c]= ObjectCopy( parameters->objects[c] );
	}
	return newAction;
}

Trigger *GenerateTriggerCore(const char *src, const char *str, int trIndex, int negate)
{
	Trigger *newTrigger = new Trigger();
	newTrigger->triggerID = (unsigned short) triggersTable->GetValueIndex( trIndex )&0x3fff;
	newTrigger->flags = (unsigned short) negate;
	int mergestrings = triggerflags[newTrigger->triggerID]&AF_MERGESTRINGS;
	int stringsCount = 0;
	int intCount = 0;
	//Here is the Trigger; Now we need to evaluate the parameters
	if (*str!=')') while (*str) {
		if (*(str+1)!=':') {
			printf("Warning, parser was sidetracked: %s\n",str);
		}
		switch (*str) {
			default:
				printf("Invalid type: %s\n",str);
				str++;
				break;

			case 'p': //Point
				SKIP_ARGUMENT();
				src++; //Skip [
				newTrigger->pointParameter.x = (short) strtol( src, (char **) &src, 10 );
				src++; //Skip .
				newTrigger->pointParameter.y = (short) strtol( src, (char **) &src, 10 );
				src++; //Skip ]
				break;

			case 'i': //Integer
			{
				//going to the variable name
				while (*str != '*' && *str !=',' && *str != ')' ) {
					str++;
				}
				int value;
				if (*str=='*') { //there may be an IDS table
					str++;
					char idsTabName[33];
					char* tmp = idsTabName;
					while (( *str != ',' ) && ( *str != ')' )) {
						*tmp = *str;
						tmp++;
						str++;
					}
					*tmp = 0;
					if (idsTabName[0]) {
						value = GetIdsValue(src, idsTabName);
					}
					else {
						value = strtol( src, (char **) &src, 0);
					}
				}
				else { //no IDS table
					value = strtol( src, (char **) &src, 0);
				}
				if (!intCount) {
					newTrigger->int0Parameter = value;
				} else if (intCount == 1) {
					newTrigger->int1Parameter = value;
				} else {
					newTrigger->int2Parameter = value;
				}
				intCount++;
			}
			break;

			case 'o': //Object
				ParseObject(str, src, newTrigger->objectParameter);
				break;

			case 's': //String
			{
				SKIP_ARGUMENT();
				src++;
				int i;
				char* dst;
				if (!stringsCount) {
					dst = newTrigger->string0Parameter;
				} else {
					dst = newTrigger->string1Parameter;
				}
				//skipping the context part, which
				//is to be readed later
				if (mergestrings) {
					for (i=0;i<6;i++) {
						*dst++='*';
					}
				}
				else {
					i=0;
				}
				while (*src != '"') {
					//sizeof(context+name) = 40
					if (i<40) {
						*dst++ = *src;
						i++;
					}
					src++;
				}
				*dst = 0;
				//reading the context part
				if (mergestrings) {
					str++;
					if (*str!='s') {
						printf("Invalid mergestrings:%s\n",str);
						abort();
					}
					SKIP_ARGUMENT();
					if (!stringsCount) {
						dst = newTrigger->string0Parameter;
					} else {
						dst = newTrigger->string1Parameter;
					}

					//this works only if there are no spaces
					if (*src++!='"' || *src++!=',' || *src++!='"') {
						break;
					}
					//reading the context string
					i=0;
					while (*src != '"') {
						if (i++<6) {
							*dst++ = *src;
						}
						src++;
					}
				}
				src++; //skipping "
				stringsCount++;
			}
			break;
		}
		str++;
		if (*src == ',' || *src==')')
			src++;
	}
	return newTrigger;
}

void SetVariable(Scriptable* Sender, const char* VarName, const char* Context, ieDword value)
{
	char newVarName[8+33];

	if (InDebug&ID_VARIABLES) {
		printf( "Setting variable(\"%s%s\", %d)\n", Context,
			VarName, value );
	}
	strncpy( newVarName, Context, 6 );
	newVarName[6]=0;
	if (strnicmp( newVarName, "MYAREA", 6 ) == 0) {
		Sender->GetCurrentArea()->locals->SetAt( VarName, value );
		return;
	}
	if (strnicmp( newVarName, "LOCALS", 6 ) == 0) {
		Sender->locals->SetAt( VarName, value );
		return;
	}
	Game *game = core->GetGame();
	if (!strnicmp(newVarName,"KAPUTZ",6) && core->HasFeature(GF_HAS_KAPUTZ) ) {
		game->kaputz->SetAt( VarName, value );
		return;
	}

	if (strnicmp(newVarName,"GLOBAL",6) ) {
		Map *map=game->GetMap(game->FindMap(newVarName));
		if (map) {
			map->locals->SetAt( VarName, value);
		}
		else if (InDebug&ID_VARIABLES) {
			printMessage("GameScript"," ",YELLOW);
			printf("Invalid variable %s %s in checkvariable\n",Context, VarName);
		}
	}
	else {
		game->locals->SetAt( VarName, ( ieDword ) value );
	}
}

void SetVariable(Scriptable* Sender, const char* VarName, ieDword value)
{
	char newVarName[8];

	if (InDebug&ID_VARIABLES) {
		printf( "Setting variable(\"%s\", %d)\n", VarName, value );
	}
	strncpy( newVarName, VarName, 6 );
	newVarName[6]=0;
	if (strnicmp( newVarName, "MYAREA", 6 ) == 0) {
		Sender->GetCurrentArea()->locals->SetAt( &VarName[6], value );
		return;
	}
	if (strnicmp( newVarName, "LOCALS", 6 ) == 0) {
		Sender->locals->SetAt( &VarName[6], value );
		return;
	}
	Game *game = core->GetGame();
	if (!strnicmp(newVarName,"KAPUTZ",6) && core->HasFeature(GF_HAS_KAPUTZ) ) {
		game->kaputz->SetAt( &VarName[6], value );
		return;
	}
	if (strnicmp(newVarName,"GLOBAL",6) ) {
		Map *map=game->GetMap(game->FindMap(newVarName));
		if (map) {
			map->locals->SetAt( &VarName[6], value);
		}
		else if (InDebug&ID_VARIABLES) {
			printMessage("GameScript"," ",YELLOW);
			printf("Invalid variable %s in setvariable\n",VarName);
		}
	}
	else {
		game->locals->SetAt( &VarName[6], ( ieDword ) value );
	}
}

ieDword CheckVariable(Scriptable* Sender, const char* VarName)
{
	char newVarName[8];
	ieDword value = 0;

	strncpy( newVarName, VarName, 6 );
	newVarName[6]=0;
	if (strnicmp( newVarName, "MYAREA", 6 ) == 0) {
		Sender->GetCurrentArea()->locals->SetAt( VarName, value );
		if (InDebug&ID_VARIABLES) {
			printf("CheckVariable %s: %d\n",VarName, value);
		}
		return value;
	}
	if (strnicmp( newVarName, "LOCALS", 6 ) == 0) {
		Sender->locals->Lookup( &VarName[6], value );
		if (InDebug&ID_VARIABLES) {
			printf("CheckVariable %s: %d\n",VarName, value);
		}
		return value;
	}
	Game *game = core->GetGame();
	if (!strnicmp(newVarName,"KAPUTZ",6) && core->HasFeature(GF_HAS_KAPUTZ) ) {
		game->kaputz->Lookup( &VarName[6], value );
		if (InDebug&ID_VARIABLES) {
			printf("CheckVariable %s: %d\n",VarName, value);
		}
		return value;
	}
	if (strnicmp(newVarName,"GLOBAL",6) ) {
		Map *map=game->GetMap(game->FindMap(newVarName));
		if (map) {
			map->locals->Lookup( &VarName[6], value);
		}
		else if (InDebug&ID_VARIABLES) {
			printMessage("GameScript"," ",YELLOW);
			printf("Invalid variable %s in checkvariable\n",VarName);
		}
	}
	else {
		game->locals->Lookup( &VarName[6], value );
	}
	if (InDebug&ID_VARIABLES) {
		printf("CheckVariable %s: %d\n",VarName, value);
	}
	return value;
}

ieDword CheckVariable(Scriptable* Sender, const char* VarName, const char* Context)
{
	char newVarName[8];
	ieDword value = 0;

	strncpy(newVarName, Context, 6);
	newVarName[6]=0;
	if (strnicmp( newVarName, "MYAREA", 6 ) == 0) {
		Sender->GetCurrentArea()->locals->SetAt( VarName, value );
		if (InDebug&ID_VARIABLES) {
			printf("CheckVariable %s%s: %d\n",Context, VarName, value);
		}
		return value;
	}
	if (strnicmp( newVarName, "LOCALS", 6 ) == 0) {
		Sender->locals->Lookup( VarName, value );
		if (InDebug&ID_VARIABLES) {
			printf("CheckVariable %s%s: %d\n",Context, VarName, value);
		}
		return value;
	}
	Game *game = core->GetGame();
	if (!strnicmp(newVarName,"KAPUTZ",6) && core->HasFeature(GF_HAS_KAPUTZ) ) {
		game->kaputz->Lookup( VarName, value );
		if (InDebug&ID_VARIABLES) {
			printf("CheckVariable %s%s: %d\n",Context, VarName, value);
		}
		return value;
	}
	if (strnicmp(newVarName,"GLOBAL",6) ) {
		Map *map=game->GetMap(game->FindMap(newVarName));
		if (map) {
			map->locals->Lookup( VarName, value);
		}
		else if (InDebug&ID_VARIABLES) {
			printMessage("GameScript"," ",YELLOW);
			printf("Invalid variable %s %s in checkvariable\n",Context, VarName);
		}
	} else {
		game->locals->Lookup( VarName, value );
	}
	if (InDebug&ID_VARIABLES) {
		printf("CheckVariable %s%s: %d\n",Context, VarName, value);
	}
	return value;
}
