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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/DLGImporter/DLGImp.cpp,v 1.13 2004/08/02 21:41:04 guidoj Exp $
 *
 */

#include "../../includes/win32def.h"
#include "DLGImp.h"
#include "../Core/FileStream.h"
#include "../Core/Interface.h"

DLGImp::DLGImp(void)
{
	str = NULL;
	autoFree = false;
	Version = 0;
}

DLGImp::~DLGImp(void)
{
	if (str && autoFree) {
		delete( str );
	}
}

bool DLGImp::Open(DataStream* stream, bool autoFree)
{
	if (stream == NULL) {
		return false;
	}
	if (str && this->autoFree) {
		delete( str );
	}
	str = stream;
	this->autoFree = autoFree;
	char Signature[8];
	str->Read( Signature, 8 );
	if (strnicmp( Signature, "DLG V1.0", 8 ) != 0) {
		if (strnicmp( Signature, "DLGV1.09", 8 ) != 0) {
			printMessage( "DLGImporter", "Not a valid DLG File...", WHITE );
			printStatus( "ERROR", LIGHT_RED );
			Version = 0;
			return false;
		} else {
			Version = 109;
		}
	} else {
		Version = 10;
	}
	str->Read( &StatesCount, 4 );
	str->Read( &StatesOffset, 4 );
	str->Read( &TransitionsCount, 4 );
	str->Read( &TransitionsOffset, 4 );
	str->Read( &StateTriggersOffset, 4 );
	str->Read( &StateTriggersCount, 4 );
	str->Read( &TransitionTriggersOffset, 4 );
	str->Read( &TransitionTriggersCount, 4 );
	str->Read( &ActionsOffset, 4 );
	str->Read( &ActionsCount, 4 );
	if (Version == 109) {
		str->Seek( 4, GEM_CURRENT_POS );
	}
	return true;
}

Dialog* DLGImp::GetDialog()
{
	if(!Version) {
		return NULL;
	}
	Dialog* d = new Dialog();
	for (unsigned int i = 0; i < StatesCount; i++) {
		DialogState* ds = GetDialogState( i );
		d->AddState( ds );
	}
	return d;
}

DialogState* DLGImp::GetDialogState(unsigned int index)
{
	DialogState* ds = new DialogState();
	str->Seek( StatesOffset + ( index * sizeof( State ) ), GEM_STREAM_START );
	State state;
	str->Read( &state, sizeof( State ) );
	ds->StrRef = state.StrRef;
	ds->trigger = GetStateTrigger( state.TriggerIndex );
	ds->transitions = GetTransitions( state.FirstTransitionIndex,
						state.TransitionsCount );
	ds->transitionsCount = state.TransitionsCount;
	return ds;
}

DialogTransition** DLGImp::GetTransitions(unsigned int firstIndex,
	unsigned int count)
{
	DialogTransition** trans = ( DialogTransition** )
		malloc( count*sizeof( DialogTransition* ) );
	for (unsigned int i = 0; i < count; i++) {
		trans[i] = GetTransition( firstIndex + i );
	}
	return trans;
}

DialogTransition* DLGImp::GetTransition(unsigned int index)
{
	if (index >= TransitionsCount) {
		return NULL;
	}
	str->Seek( TransitionsOffset + ( index * sizeof( Transition ) ),
			GEM_STREAM_START );
	Transition trans;
	str->Read( &trans, sizeof( Transition ) );
	DialogTransition* dt = new DialogTransition();
	dt->Flags = trans.Flags;
	dt->textStrRef = trans.AnswerStrRef;
	dt->journalStrRef = trans.JournalStrRef;
	dt->trigger = GetTransitionTrigger( trans.TriggerIndex );
	dt->action = GetAction( trans.ActionIndex );
	strncpy( dt->Dialog, trans.DLGResRef, 8 );
	dt->Dialog[8] = 0;
	dt->stateIndex = trans.NextStateIndex;
	return dt;
}

DialogString* DLGImp::GetStateTrigger(unsigned int index)
{
	if (index >= StateTriggersCount) {
		return NULL;
	}
	str->Seek( StateTriggersOffset + ( index * sizeof( VarOffset ) ),
			GEM_STREAM_START );
	VarOffset offset;
	str->Read( &offset, sizeof( VarOffset ) );
	DialogString* ds = new DialogString();
	str->Seek( offset.Offset, GEM_STREAM_START );
	char* string = ( char* ) malloc( offset.Length + 1 );
	str->Read( string, offset.Length );
	string[offset.Length] = 0;
	ds->strings = GetStrings( string, ds->count );
	free( string );
	return ds;
}

DialogString* DLGImp::GetTransitionTrigger(unsigned int index)
{
	if (index >= TransitionTriggersCount) {
		return NULL;
	}
	str->Seek( TransitionTriggersOffset + ( index * sizeof( VarOffset ) ),
			GEM_STREAM_START );
	VarOffset offset;
	str->Read( &offset, sizeof( VarOffset ) );
	DialogString* ds = new DialogString();
	str->Seek( offset.Offset, GEM_STREAM_START );
	char* string = ( char* ) malloc( offset.Length + 1 );
	str->Read( string, offset.Length );
	string[offset.Length] = 0;
	ds->strings = GetStrings( string, ds->count );
	free( string );
	return ds;
}

DialogString* DLGImp::GetAction(unsigned int index)
{
	if (index >= ActionsCount) {
		return NULL;
	}
	str->Seek( ActionsOffset + ( index * sizeof( VarOffset ) ),
			GEM_STREAM_START );
	VarOffset offset;
	str->Read( &offset, sizeof( VarOffset ) );
	DialogString* ds = new DialogString();
	str->Seek( offset.Offset, GEM_STREAM_START );
	char* string = ( char* ) malloc( offset.Length + 1 );
	str->Read( string, offset.Length );
	string[offset.Length] = 0;
	ds->strings = GetStrings( string, ds->count );
	free( string );
	return ds;
}

int GetActionLength(const char* string)
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
			default:
				break;
		}
	}
	return i;
}

#define MyIsSpace(c) (((c) == ' ') || ((c) == '\n') || ((c) == '\r'))

/* this function will break up faulty script strings that lack the CRLF
   between commands, common in PST dialog */
char** DLGImp::GetStrings(char* string, unsigned int& count)
{
	int col = 0;
	int level = 0;
	bool quotes = true;
	bool ignore = false;
	char* poi = string;

	count = 0;
	while (*poi) {
/* maybe we don't need this at all
		while (MyIsSpace( *poi ))
			poi++;
		//i think we need this, 
		//think of an empty line containing only whitespace
		if(!poi) {
			break;
		}
*/
		switch (*poi++) {
			case '/':
				if(col==0) {
					if(*poi=='/') {
						poi++;
						ignore=true;
					}
				}
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
	for (unsigned int i = 0; i < count; i++) {
		while (MyIsSpace( *poi ))
			poi++;
		int len = GetActionLength( poi );
		if((*poi=='/') && (*(poi+1)=='/') ) {
			poi+=len;
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
