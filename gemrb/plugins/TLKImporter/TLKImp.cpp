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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/TLKImporter/TLKImp.cpp,v 1.26 2004/04/22 21:37:50 avenger_teambg Exp $
 *
 */

#include "../../includes/win32def.h"
#include "TLKImp.h"
#include "../Core/Interface.h"
//#include "ie_stats.h"

TLKImp::TLKImp(void)
{
	str = NULL;
	autoFree = false;
}

TLKImp::~TLKImp(void)
{
	if (str && autoFree) {
		delete( str );
	}
}

bool TLKImp::Open(DataStream* stream, bool autoFree)
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
	if (strncmp( Signature, "TLK V1  ", 8 ) != 0) {
		printf( "[TLKImporter]: Not a valid TLK File." );
		return false;
	}
	str->Seek( 2, GEM_CURRENT_POS );
	str->Read( &StrRefCount, 4 );
	str->Read( &Offset, 4 );
	return true;
}

inline char* mystrncpy(char* dest, const char* source, int maxlength,
	char delim)
{
	while (*source && ( *source != delim ) && maxlength--) {
		*dest++ = *source++;
	}
	*dest = 0;
	return dest;
}

int TLKImp::GenderStrRef(int slot, int malestrref, int femalestrref)
{
	Actor *act;

	if(slot==-1) {
		act=core->GetGameControl()->speaker;
	}
	else {
		act=core->GetGame()->FindPC(slot);
	}
	if(act && act->GetStat(IE_SEX)==2) {
		return femalestrref;
	}
	return malestrref;
}

//if this function returns -1 then it is not a built in token, dest may be NULL
int TLKImp::BuiltinToken(char* Token, char* dest)
{
	char* Decoded = NULL;
	bool freeup=true;
	int TokenLength;   //decoded token length

	//this may be hardcoded, all engines are the same or don't use it
	if (!strcmp( Token, "FIGHTERTYPE" )) {
		Decoded = GetString( 10086, 0 );
		goto exit_function;
	}
	if(!strcmp( Token, "GABBER" )) {
		//don't free this!
		Decoded=core->GetGameControl()->speaker->LongName;
		freeup=false;
		goto exit_function;
	}
	if (!strcmp( Token, "SIRMAAM" )) {
		Decoded = GetString( GenderStrRef(-1,27473,27475), 0);
		goto exit_function;
	}
	if (!strcmp( Token, "GIRLBOY" )) {
		Decoded = GetString( GenderStrRef(-1,27477,27476), 0);
		goto exit_function;
	}
	if (!strcmp( Token, "BROTHERSISTER" )) {
		Decoded = GetString( GenderStrRef(-1,27478,27479), 0);
		goto exit_function;
	}
	if (!strcmp( Token, "LADYLORD" )) {
		Decoded = GetString( GenderStrRef(-1,27481,27480), 0);
		goto exit_function;
	}
	if (!strcmp( Token, "MALEFEMALE" )) {
		Decoded = GetString( GenderStrRef(-1,27483,27482), 0);
		goto exit_function;
	}
	if (!strcmp( Token, "HESHE" )) {
		Decoded = GetString( GenderStrRef(-1,27485,27484), 0);
		goto exit_function;
	}
	if (!strcmp( Token, "HISHER" )) {
		Decoded = GetString( GenderStrRef(-1,27487,27486), 0);
		goto exit_function;
	}
	if (!strcmp( Token, "MANWOMAN" )) {
		Decoded = GetString( GenderStrRef(-1,27489,27488), 0);
		goto exit_function;
	}

	if (!strcmp( Token, "PRO_SIRMAAM" )) {
		Decoded = GetString( GenderStrRef(0,27473,27475), 0);
		goto exit_function;
	}
	if (!strcmp( Token, "PRO_GIRLBOY" )) {
		Decoded = GetString( GenderStrRef(0,27477,27476), 0);
		goto exit_function;
	}
	if (!strcmp( Token, "PRO_BROTHERSISTER" )) {
		Decoded = GetString( GenderStrRef(0,27478,27479), 0);
		goto exit_function;
	}
	if (!strcmp( Token, "PRO_LADYLORD" )) {
		Decoded = GetString( GenderStrRef(0,27481,27480), 0);
		goto exit_function;
	}
	if (!strcmp( Token, "PRO_MALEFEMALE" )) {
		Decoded = GetString( GenderStrRef(0,27483,27482), 0);
		goto exit_function;
	}
	if (!strcmp( Token, "PRO_HESHE" )) {
		Decoded = GetString( GenderStrRef(0,27485,27484), 0);
		goto exit_function;
	}
	if (!strcmp( Token, "PRO_HISHER" )) {
		Decoded = GetString( GenderStrRef(0,27487,27486), 0);
		goto exit_function;
	}
	if (!strcmp( Token, "PRO_MANWOMAN" )) {
		Decoded = GetString( GenderStrRef(0,27489,27488), 0);
		goto exit_function;
	}
	if (!strcmp( Token, "WEAPONNAME" )) {
		//this should be character dependent, we don't have a character sheet yet
	}

	if (!strcmp( Token, "MAGESCHOOL" )) {
		//this should be character dependent, we don't have a character sheet yet
		unsigned long row = 0; //default value is 0 (generalist)
		//this is subject to change, the row number in magesch.2da
		core->GetDictionary()->Lookup( "MAGESCHOOL", row ); 
		int ind = core->LoadTable( "magesch" );
		TableMgr* tm = core->GetTable( ind );
		if (tm) {
			char* value = tm->QueryField( row, 0 );
			Decoded = GetString( atoi( value ), 0 );
			goto exit_function;
		}
	}

	return -1;  //not decided

	exit_function:
	if (Decoded) {
		TokenLength = ( int ) strlen( Decoded );
		if (dest) {
			memcpy( dest, Decoded, TokenLength );
		}
		if (freeup) {
			free( Decoded );
		}
		return TokenLength;
	}
	return -1;
}

bool TLKImp::ResolveTags(char* dest, char* source, int Length)
{
	int NewLength;
	char Token[MAX_VARIABLE_LENGTH + 1];

	NewLength = 0;
	for (int i = 0; source[i]; i++) {
		if (source[i] == '<') {
			i += mystrncpy( Token, source + i + 1, MAX_VARIABLE_LENGTH, '>' ) -
				Token +
				1;
			int TokenLength = BuiltinToken( Token, dest + NewLength );
			if (TokenLength == -1) {
				TokenLength = core->GetTokenDictionary()->GetValueLength( Token );
				if (TokenLength) {
					if (TokenLength + NewLength > Length)
						return false;
					core->GetTokenDictionary()->Lookup( Token, dest + NewLength, TokenLength );
				}
			}
			NewLength += TokenLength;
		} else {
			if (source[i] == '[') {
				char* tmppoi = strchr( source + i + 1, ']' );
				if (tmppoi)
					i = tmppoi - source;
				else
					break;
			} else
				dest[NewLength++] = source[i];
			if (NewLength > Length)
				return false;
		}
	}
	dest[NewLength] = 0;
	return true;
}

bool TLKImp::GetNewStringLength(char* string, unsigned long& Length)
{
	int NewLength;
	bool lChange;
	char Token[MAX_VARIABLE_LENGTH + 1];

	lChange = false;
	NewLength = 0;
	for (unsigned int i = 0; i < Length; i++) {
		if (string[i] == '<') {
			// token
			lChange = true;
			i += mystrncpy( Token, string + i + 1, MAX_VARIABLE_LENGTH, '>' ) - Token + 1;
			int TokenLength = BuiltinToken( Token, NULL );
			if (TokenLength == -1) {
				NewLength += core->GetTokenDictionary()->GetValueLength( Token );
			} else {
				NewLength += TokenLength;
			}
		} else {
			if (string[i] == '[') {
				//voice actor directives
				lChange = true;
				char* tmppoi = strchr( string + i + 1, ']' );
				if (tmppoi)
					i += tmppoi - string - i;
				else
					break;
			} else {
				NewLength++;
			}
		}
	}
	Length = NewLength;
	return lChange;
}

char* TLKImp::GetString(unsigned long strref, int flags)
{
	if (strref >= StrRefCount) {
		char* ret = ( char* ) malloc( 1 );
		ret[0] = 0;
		return ret;
	}
	unsigned long Volume, Pitch, StrOffset, Length;
	unsigned short type;
	char SoundResRef[9];
	str->Seek( 18 + ( strref * 0x1A ), GEM_STREAM_START );
	str->Read( &type, 2 );
	str->Read( SoundResRef, 8 );
	SoundResRef[8] = 0;
	str->Read( &Volume, 4 );
	str->Read( &Pitch, 4 );
	str->Read( &StrOffset, 4 );
	str->Read( &Length, 4 );
	if (Length > 65535) {
		Length = 65535;
	} 
	char* string;

	if (type & 1) {
		str->Seek( StrOffset + Offset, GEM_STREAM_START );
		string = ( char * ) malloc( Length + 1 );
		str->Read( string, Length );
	} else {
		Length = 0;
		string = ( char * ) malloc( 1 );
	}
	string[Length] = 0;
	//tagged text, bg1 and iwd don't mark them specifically, all entries are tagged
	if (core->HasFeature( GF_ALL_STRINGS_TAGGED ) || ( type & 4 )) {
		//GetNewStringLength will look in string and return true
		//if the new Length will change due to tokens
		//if there is no new length, we are done
		while (GetNewStringLength( string, Length )) {
			char* string2 = ( char* ) malloc( Length + 1 );
			//ResolveTags will copy string to string2
			ResolveTags( string2, string, Length );
			free( string );
			string = string2;
		}
	}
	if (( type & 2 ) && ( flags & IE_STR_SOUND )) {
		//if flags&IE_STR_SOUND play soundresref
		if (SoundResRef[0] != 0)
			core->GetSoundMgr()->Play( SoundResRef );
	}
	if (flags & IE_STR_STRREFON) {
		char* string2 = ( char* ) malloc( Length + 11 );
		sprintf( string2, "%ld: %s", strref, string );
		free( string );
		return string2;
	}
	return string;
}

StringBlock TLKImp::GetStringBlock(unsigned long strref, int flags)
{
	StringBlock sb;
	if (strref >= StrRefCount) {
		sb.text = ( char * ) malloc( 1 );
		sb.text[0] = 0;
		sb.Sound[0] = 0;
		return sb;
	}
	sb.text = GetString( strref, flags );
	unsigned short type;
	str->Seek( 18 + ( strref * 0x1A ), GEM_STREAM_START );
	str->Read( &type, 2 );
	str->Read( sb.Sound, 8 );
	sb.Sound[8] = 0;
	return sb;
}
