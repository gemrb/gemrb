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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA	02111-1307, USA.
 *
 * $Id$
 *
 */

#include "../../includes/win32def.h"
#include "TLKImp.h"
#include "../Core/Interface.h"
#include "../Core/Audio.h"
#include "../Core/Game.h"
#include "../Core/GameControl.h"

static int *monthnames=NULL;
static int *days=NULL;
static int monthnamecount=0;
//set this to -1 if charname is gabber (iwd2)
static int charname=0;

TLKImp::TLKImp(void)
{
	if (core->HasFeature(GF_CHARNAMEISGABBER)) {
		charname=-1;
	} else {
		charname=0;
	}
	str = NULL;
	override = NULL;
	autoFree = false;
	if (monthnamecount==0) {
		int i;
		AutoTable tab("months");
		if (!tab) {
			monthnamecount=-1;
			return;
		}
		monthnamecount = tab->GetRowCount();
		monthnames = (int *) malloc(sizeof(int) * monthnamecount);
		days = (int *) malloc(sizeof(int) * monthnamecount);
		for(i=0;i<monthnamecount;i++) {
			days[i]=atoi(tab->QueryField(i,0));
			monthnames[i]=atoi(tab->QueryField(i,1));
		}
	}
}

TLKImp::~TLKImp(void)
{
	if (monthnames) free(monthnames);
	if (days) free(days);
	monthnamecount=0;
	if (str && autoFree) {
		delete( str );
	}
	CloseAux();
}

void TLKImp::CloseAux()
{
	if (override) {
		delete override;
	}
	override = NULL;
}

void TLKImp::OpenAux()
{
	CloseAux();
	override = new CTlkOverride();
	if (override) {
		if (!override->Init()) {
			CloseAux();
			printMessage("TlkImporter","Cannot open tlk override!\n", LIGHT_RED);
		}
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
	if (strncmp( Signature, "TLK\x20V1\x20\x20", 8 ) != 0) {
		printMessage( "TLKImporter","Not a valid TLK File.\n", LIGHT_RED );
		return false;
	}
	str->Seek( 2, GEM_CURRENT_POS );
	str->ReadDword( &StrRefCount );
	str->ReadDword( &Offset );
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

/* -1	 - GABBER
		0	 - PROTAGONIST
		1-9 - PLAYERx
*/
inline Actor *GetActorFromSlot(int slot)
{
	if (slot==-1) {
		GameControl *gc = core->GetGameControl();
		if (gc) {
			return gc->GetSpeaker();
		}
		return NULL;
	}
	Game *game = core->GetGame();
	if (!game) {
		return NULL;
	}
	if (slot==0) {
		return game->GetPC(0, false); //protagonist
	}
	return game->FindPC(slot);
}

char *TLKImp::Gabber()
{
	Actor *act;

	act=core->GetGameControl()->GetSpeaker();
	if (act) {
		return override->CS(act->LongName);
	}
	return override->CS("?");
}

char *TLKImp::CharName(int slot)
{
	Actor *act;

	act=GetActorFromSlot(slot);
	if (act) {
		return override->CS(act->LongName);
	}
	return override->CS("?");
}

int TLKImp::RaceStrRef(int slot)
{
	Actor *act;
	int race;

	act=GetActorFromSlot(slot);
	if (act) {
		race=act->GetStat(IE_RACE);
	} else {
		race=0;
	}

	AutoTable tab("races");
	if (!tab) {
		return -1;
	}
	return atoi(tab->QueryField(race,0) );
}

int TLKImp::GenderStrRef(int slot, int malestrref, int femalestrref)
{
	Actor *act;

	act = GetActorFromSlot(slot);
	if (act && (act->GetStat(IE_SEX)==SEX_FEMALE) ) {
		return femalestrref;
	}
	return malestrref;
}

void TLKImp::GetMonthName(int dayandmonth)
{
	int month=1;

	for(int i=0;i<monthnamecount;i++) {
		if (dayandmonth<days[i]) {
			char *tmp;
			char tmpstr[10];
			
			sprintf(tmpstr,"%d", dayandmonth+1);
			core->GetTokenDictionary()->SetAtCopy("DAY", tmpstr);

			tmp = GetString( monthnames[i] );
			core->GetTokenDictionary()->SetAt("MONTHNAME",tmp);
			//must not free tmp, SetAt doesn't copy the pointer!

			sprintf(tmpstr,"%d", month);
			core->GetTokenDictionary()->SetAtCopy("MONTH",tmpstr);
			return;
		}
		dayandmonth-=days[i];
		//ignoring single days (they are not months)
		if (days[i]!=1) month++;
	}
}

//if this function returns -1 then it is not a built in token, dest may be NULL
int TLKImp::BuiltinToken(char* Token, char* dest)
{
	char* Decoded = NULL;
	int TokenLength;	 //decoded token length

	//these are hardcoded, all engines are the same or don't use them
	if (!strcmp( Token, "DAYANDMONTH")) {
		ieDword dayandmonth=0;
		core->GetDictionary()->Lookup("DAYANDMONTH",dayandmonth);
		//preparing sub-tokens
		GetMonthName((int) dayandmonth);
		Decoded = GetString( 15981, 0 );
		goto exit_function;
	}

	if (!strcmp( Token, "FIGHTERTYPE" )) {
		Decoded = GetString( 10086, 0 );
		goto exit_function;
	}
	if (!strcmp( Token, "RACE" )) {
		Decoded = GetString( RaceStrRef(-1), 0);
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
		Decoded = GetString( GenderStrRef(-1,27482,27483), 0);
		goto exit_function;
	}
	if (!strcmp( Token, "HESHE" )) {
		Decoded = GetString( GenderStrRef(-1,27484,27485), 0);
		goto exit_function;
	}
	if (!strcmp( Token, "HISHER" )) {
		Decoded = GetString( GenderStrRef(-1,27486,27487), 0);
		goto exit_function;
	}
	if (!strcmp( Token, "HIMHER" )) {
		Decoded = GetString( GenderStrRef(-1,27488,27487), 0);
		goto exit_function;
	}
	if (!strcmp( Token, "MANWOMAN" )) {
		Decoded = GetString( GenderStrRef(-1,27489,27490), 0);
		goto exit_function;
	}
	if (!strncmp( Token, "PLAYER",6 )) {
		Decoded = CharName(Token[6]-'1');
		goto exit_function;
	}

	if (!strcmp( Token, "GABBER" )) {
		Decoded = Gabber();
		goto exit_function;
	}
	if (!strcmp( Token, "CHARNAME" )) {
		Decoded = CharName(charname);
		goto exit_function;
	}
	if (!strcmp( Token, "PRO_RACE" )) {
		Decoded = GetString( RaceStrRef(0), 0);
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
		Decoded = GetString( GenderStrRef(0,27482,27483), 0);
		goto exit_function;
	}
	if (!strcmp( Token, "PRO_HESHE" )) {
		Decoded = GetString( GenderStrRef(0,27484,27485), 0);
		goto exit_function;
	}
	if (!strcmp( Token, "PRO_HISHER" )) {
		Decoded = GetString( GenderStrRef(0,27486,27487), 0);
		goto exit_function;
	}
	if (!strcmp( Token, "PRO_HIMHER" )) {
		Decoded = GetString( GenderStrRef(0,27488,27487), 0);
		goto exit_function;
	}
	if (!strcmp( Token, "PRO_MANWOMAN" )) {
		Decoded = GetString( GenderStrRef(0,27489,27490), 0);
		goto exit_function;
	}

	if (!strcmp( Token, "MAGESCHOOL" )) {
		ieDword row = 0; //default value is 0 (generalist)
		//this is subject to change, the row number in magesch.2da
		core->GetDictionary()->Lookup( "MAGESCHOOL", row ); 
		AutoTable tm("magesch");
		if (tm) {
			const char* value = tm->QueryField( row, 2 );
			Decoded = GetString( atoi( value ), 0 );
			goto exit_function;
		}
	}

	return -1;	//not decided

	exit_function:
	if (Decoded) {
		TokenLength = ( int ) strlen( Decoded );
		if (dest) {
			memcpy( dest, Decoded, TokenLength );
		}
		//Decoded is always a copy
		free( Decoded );
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
			i += (int) (mystrncpy( Token, source + i + 1, MAX_VARIABLE_LENGTH, '>' ) - Token) + 1;
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
				const char* tmppoi = strchr( source + i + 1, ']' );
				if (tmppoi)
					i = (int) (tmppoi - source);
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

bool TLKImp::GetNewStringLength(char* string, int& Length)
{
	int NewLength;
	bool lChange;
	char Token[MAX_VARIABLE_LENGTH + 1];

	lChange = false;
	NewLength = 0;
	for (int i = 0; i < Length; i++) {
		if (string[i] == '<') {
			// token
			lChange = true;
			i += (int) (mystrncpy( Token, string + i + 1, MAX_VARIABLE_LENGTH, '>' ) - Token) + 1;
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
				const char* tmppoi = strchr( string + i + 1, ']' );
				if (tmppoi)
					i += (int) (tmppoi - string) - i;
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

char* TLKImp::GetString(ieStrRef strref, ieDword flags)
{
	char* string;
	
	if (!(flags&IE_STR_ALLOW_ZERO) && !strref) {
		goto empty;
	}
	ieWord type;
	int Length;
	ieResRef SoundResRef;

	if (strref >= StrRefCount) {
empty:
		string = override->ResolveAuxString(strref, Length);
		type = 0;
		SoundResRef[0]=0;
	} else {
		ieDword Volume, Pitch, StrOffset;
		ieDword l;
		str->Seek( 18 + ( strref * 0x1A ), GEM_STREAM_START );
		str->ReadWord( &type );
		str->ReadResRef( SoundResRef );
		str->ReadDword( &Volume );
		str->ReadDword( &Pitch );
		str->ReadDword( &StrOffset );
		str->ReadDword( &l );
		if (l > 65535) {
			Length = 65535; //safety limit, it could be a dword actually
		}
		else {
			Length = l;
		}
		
		if (type & 1) {
			str->Seek( StrOffset + Offset, GEM_STREAM_START );
			string = ( char * ) malloc( Length + 1 );
			str->Read( string, Length );
		} else {
			Length = 0;
			string = ( char * ) malloc( 1 );
		}
		string[Length] = 0; 
	}

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
		if (SoundResRef[0] != 0) {
			int xpos = 0;
			int ypos = 0;
			unsigned int flag = GEM_SND_RELATIVE | (flags&GEM_SND_SPEECH);
			//IE_STR_SPEECH will stop the previous sound source
			core->GetAudioDrv()->Play( SoundResRef, xpos, ypos, flag);
		}
	}
	if (flags & IE_STR_STRREFON) {
		char* string2 = ( char* ) malloc( Length + 11 );
		sprintf( string2, "%d: %s", strref, string );
		free( string );
		return string2;
	}
	return string;
}

StringBlock TLKImp::GetStringBlock(ieStrRef strref, unsigned int flags)
{
	StringBlock sb;

	if (!(flags&IE_STR_ALLOW_ZERO) && !strref) {
		goto empty;
	}
	if (strref >= StrRefCount) {
empty:
		sb.text = ( char * ) malloc( 1 );
		sb.text[0] = 0;
		sb.Sound[0] = 0;
		return sb;
	}
	sb.text = GetString( strref, flags );
	ieWord type;
	str->Seek( 18 + ( strref * 0x1A ), GEM_STREAM_START );
	str->ReadWord( &type );
	str->ReadResRef( sb.Sound );
	return sb;
}

void TLKImp::FreeString(char *str)
{
	free(str);
}

