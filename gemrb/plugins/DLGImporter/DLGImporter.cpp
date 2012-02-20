/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#include "DLGImporter.h"

#include "win32def.h"

#include "Interface.h"
#include "GameScript/GameScript.h"
#include "System/FileStream.h"

using namespace GemRB;

DLGImporter::DLGImporter(void)
{
	str = NULL;
	Version = 0;
}

DLGImporter::~DLGImporter(void)
{
	delete str;
}

bool DLGImporter::Open(DataStream* stream)
{
	if (stream == NULL) {
		return false;
	}
	delete str;
	str = stream;
	char Signature[8];
	str->Read( Signature, 8 );
	if (strnicmp( Signature, "DLG V1.0", 8 ) != 0) {
		Log(ERROR, "DLGImporter", "Not a valid DLG File...");
		Version = 0;
		return false;
	}
	str->ReadDword( &StatesCount );
	str->ReadDword( &StatesOffset );
	// bg2
	if (StatesOffset == 0x34 ) {
		Version = 104;
	}
	else {
		Version = 100;
	}
	str->ReadDword( &TransitionsCount );
	str->ReadDword( &TransitionsOffset );
	str->ReadDword( &StateTriggersOffset );
	str->ReadDword( &StateTriggersCount );
	str->ReadDword( &TransitionTriggersOffset );
	str->ReadDword( &TransitionTriggersCount );
	str->ReadDword( &ActionsOffset );
	str->ReadDword( &ActionsCount );
	if (Version == 104) {
		str->ReadDword( &Flags );
	}
	else {
		Flags = 0;
	}
	return true;
}

Dialog* DLGImporter::GetDialog() const
{
	if(!Version) {
		return NULL;
	}
	Dialog* d = new Dialog();
	d->Flags = Flags;
	d->TopLevelCount = StatesCount;
	d->Order = (unsigned int *) calloc (StatesCount, sizeof(unsigned int *) );
	d->initialStates = (DialogState **) calloc (StatesCount, sizeof(DialogState *) );
	for (unsigned int i = 0; i < StatesCount; i++) {
		DialogState* ds = GetDialogState( d, i );
		d->initialStates[i] = ds;
	}
	return d;
}

DialogState* DLGImporter::GetDialogState(Dialog *d, unsigned int index) const
{
	DialogState* ds = new DialogState();
	//16 = sizeof(State)
	str->Seek( StatesOffset + ( index * 16 ), GEM_STREAM_START );
	ieDword  FirstTransitionIndex;
	ieDword  TriggerIndex;
	str->ReadDword( &ds->StrRef );
	str->ReadDword( &FirstTransitionIndex );
	str->ReadDword( &ds->transitionsCount );
	str->ReadDword( &TriggerIndex );
	ds->condition = GetStateTrigger( TriggerIndex );
	ds->transitions = GetTransitions( FirstTransitionIndex, ds->transitionsCount );
	if (TriggerIndex<StatesCount)
		d->Order[TriggerIndex] = index;
	return ds;
}

DialogTransition** DLGImporter::GetTransitions(unsigned int firstIndex, unsigned int count) const
{
	DialogTransition** trans = ( DialogTransition** )
		malloc( count*sizeof( DialogTransition* ) );
	for (unsigned int i = 0; i < count; i++) {
		trans[i] = GetTransition( firstIndex + i );
	}
	return trans;
}

DialogTransition* DLGImporter::GetTransition(unsigned int index) const
{
	if (index >= TransitionsCount) {
		return NULL;
	}
	//32 = sizeof(Transition)
	str->Seek( TransitionsOffset + ( index * 32 ), GEM_STREAM_START );
	DialogTransition* dt = new DialogTransition();
	str->ReadDword( &dt->Flags );
	str->ReadDword( &dt->textStrRef );
	if (!(dt->Flags & IE_DLG_TR_TEXT)) {
		dt->textStrRef = 0xffffffff;
	}
	str->ReadDword( &dt->journalStrRef );
	if (!(dt->Flags & IE_DLG_TR_JOURNAL)) {
		dt->journalStrRef = 0xffffffff;
	}
	ieDword TriggerIndex;
	ieDword ActionIndex;
	str->ReadDword( &TriggerIndex );
	str->ReadDword( &ActionIndex );
	str->ReadResRef( dt->Dialog );
	str->ReadDword( &dt->stateIndex );
	if (dt->Flags &IE_DLG_TR_TRIGGER) {
		dt->condition = GetTransitionTrigger( TriggerIndex );
	}
	else {
		dt->condition = NULL;
	}
	if (dt->Flags & IE_DLG_TR_ACTION) {
		dt->actions = GetAction( ActionIndex );
	}
	return dt;
}

static char** GetStrings(char* string, unsigned int& count);

static Condition* GetCondition(char* string)
{
	unsigned int count;
	char **lines = GetStrings( string, count );
	Condition *condition = new Condition();
	for (size_t i = 0; i < count; ++i) {
		Trigger *trigger = GenerateTrigger(lines[i]);
		if (!trigger) {
			Log(WARNING, "DLGImporter", "Can't compile trigger: %s", lines[i]);
		} else {
			condition->triggers.push_back(trigger);
		}
		free( lines[i] );
	}
	free( lines );
	return condition;
}

Condition* DLGImporter::GetStateTrigger(unsigned int index) const
{
	if (index >= StateTriggersCount) {
		return NULL;
	}
	//8 = sizeof(VarOffset)
	str->Seek( StateTriggersOffset + ( index * 8 ), GEM_STREAM_START );
	ieDword Offset, Length;
	str->ReadDword( &Offset );
	str->ReadDword( &Length );
	//a zero length trigger counts as no trigger
	//a // comment counts as true(), so we simply ignore zero
	//length trigger text like it isn't there
	if (!Length) {
		return NULL;
	}
	str->Seek( Offset, GEM_STREAM_START );
	char* string = ( char* ) malloc( Length + 1 );
	str->Read( string, Length );
	string[Length] = 0;
	Condition *condition = GetCondition(string);
	free( string );
	return condition;
}

Condition* DLGImporter::GetTransitionTrigger(unsigned int index) const
{
	if (index >= TransitionTriggersCount) {
		return NULL;
	}
	str->Seek( TransitionTriggersOffset + ( index * 8 ), GEM_STREAM_START );
	ieDword Offset, Length;
	str->ReadDword( &Offset );
	str->ReadDword( &Length );
	str->Seek( Offset, GEM_STREAM_START );
	char* string = ( char* ) malloc( Length + 1 );
	str->Read( string, Length );
	string[Length] = 0;
	Condition *condition = GetCondition(string);
	free( string );
	return condition;
}

std::vector<Action*> DLGImporter::GetAction(unsigned int index) const
{
	if (index >= ActionsCount) {
		return std::vector<Action*>();
	}
	str->Seek( ActionsOffset + ( index * 8 ), GEM_STREAM_START );
	ieDword Offset, Length;
	str->ReadDword( &Offset );
	str->ReadDword( &Length );
	str->Seek( Offset, GEM_STREAM_START );
	char* string = ( char* ) malloc( Length + 1 );
	str->Read( string, Length );
	string[Length] = 0;
	unsigned int count;
	char ** lines = GetStrings( string, count );
	std::vector<Action*> actions;
	for (size_t i = 0; i < count; ++i) {
		Action *action = GenerateAction(lines[i]);
		if (!action) {
			Log(WARNING, "DLGImporter", "Can't compile action: %s", lines[i]);
		} else {
			action->IncRef();
			actions.push_back(action);
		}
		free( lines[i] );
	}
	free( lines );
	free( string );
	return actions;
}

static int GetActionLength(const char* string)
{
	int i;
	int level = 0;
	bool quotes = true;
	const char* poi = string;

	for (i = 0; *poi; i++) {
		switch (*poi++) {
			case '"':
				quotes = !quotes;
				break;
			case '(':
				if (quotes) {
					level++;
				}
				break;
			case ')':
				if (quotes && level) {
					level--;
					if (level == 0) {
						return i + 1;
					}
				}
				break;
			case '\r':
			case '\n':
				// force reset on newline if quotes are open
				if (!quotes) return i;
				break;
			default:
				break;
		}
	}
	return i;
}

#define MyIsSpace(c) (((c) == ' ') || ((c) == '\n') || ((c) == '\r'))

/* this function will break up faulty script strings that lack the CRLF
   between commands, common in PST dialog */
/* misc test cases (just examples, there are more):
     pst's FORGE.DLG (trigger split across two lines),
     bg2's SAHIMP02.DLG (missing quotemark in string),
     bg2's QUAYLE.DLG (missing closing bracket) */
static char** GetStrings(char* string, unsigned int& count)
{
	int col = 0;
	int level = 0;
	bool quotes = true;
	bool ignore = false;
	char* poi = string;

	count = 0;
	while (*poi) {
		switch (*poi++) {
			case '/':
				if(col==0) {
					if(*poi=='/') {
						poi++;
						ignore=true;
					}
				}
				break;
			case '"':
				quotes = !quotes;
				break;
			case '(':
				if (quotes) {
					level++;
				}
				break;
			case ')':
				if (quotes && level) {
					level--;
					if (level == 0) {
						if(!ignore) {
							count++;
						}
						ignore=false;
					}
				}
				break;
			case '\r':
			case '\n':
				// force reset on newline if quotes are open, or we had a comment
				if (!quotes || ignore) {
					level = 0;
					quotes = true;
					ignore = false;
					count++;
				}
				break;
			default:
				break;
		}
	}
	if(!count) {
		return NULL;
	}
	char** strings = ( char** ) calloc( count, sizeof( char* ) );
	if (strings == NULL) {
		count = 0;
		return strings;
	}
	poi = string;
	for (int i = 0; i < (int)count; i++) {
		while (MyIsSpace( *poi ))
			poi++;
		int len = GetActionLength( poi );
		if((*poi=='/') && (*(poi+1)=='/') ) {
			poi+=len;
			i--;
			continue;
		}
		strings[i] = ( char * ) malloc( len + 1 );
		int j;
		for (j = 0; len; poi++,len--) {
			if (isspace( *poi ))
				continue;
			strings[i][j++] = *poi;
		}
		strings[i][j] = 0;
	}
	return strings;
}

#include "plugindef.h"

GEMRB_PLUGIN(0x1970D894, "DLG File Importer")
PLUGIN_CLASS(IE_DLG_CLASS_ID, DLGImporter)
END_PLUGIN()
