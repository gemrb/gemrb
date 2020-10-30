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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#include "TLKImporter.h"

#include "win32def.h"

#include "Audio.h"
#include "Calendar.h"
#include "DialogHandler.h"
#include "Game.h"
#include "Interface.h"
#include "TableMgr.h"
#include "GUI/GameControl.h"
#include "Scriptable/Actor.h"

using namespace GemRB;

struct gt_type
{
	int type;
	ieStrRef male;
	ieStrRef female;
};

TLKImporter::TLKImporter(void)
{
	gtmap.RemoveAll(NULL);
	gtmap.SetType(GEM_VARIABLES_POINTER);

	if (core->HasFeature(GF_CHARNAMEISGABBER)) {
		charname=-1;
	}

	AutoTable tm("gender");
	int gtcount = 0;
	if (tm) {
		gtcount = tm->GetRowCount();
	}
	for(int i=0;i<gtcount;i++) {
		ieVariable key;

		strnuprcpy(key, tm->GetRowName(i), sizeof(ieVariable)-1 );
		gt_type *entry = new gt_type;
		entry->type = atoi(tm->QueryField(i,0));
		entry->male = atoi(tm->QueryField(i,1));
		entry->female = atoi(tm->QueryField(i,2));
		gtmap.SetAt(key, (void *) entry);
	}
}

static void ReleaseGtEntry(void *poi)
{
	delete (gt_type *) poi;
}

TLKImporter::~TLKImporter(void)
{
	delete str;
	
	gtmap.RemoveAll(ReleaseGtEntry);

	CloseAux();
}

void TLKImporter::CloseAux()
{
	if (OverrideTLK) {
		delete OverrideTLK;
	}
	OverrideTLK = NULL;
}

void TLKImporter::OpenAux()
{
	CloseAux();
	OverrideTLK = new CTlkOverride();
	if (OverrideTLK && !OverrideTLK->Init()) {
		CloseAux();
		Log(ERROR, "TlkImporter", "Cannot open tlk override!");
	}
}

bool TLKImporter::Open(DataStream* stream)
{
	if (stream == NULL) {
		return false;
	}
	delete str;
	str = stream;
	char Signature[8];
	str->Read( Signature, 8 );
	if (strncmp( Signature, "TLK\x20V1\x20\x20", 8 ) != 0) {
		Log(ERROR, "TLKImporter", "Not a valid TLK File.");
		return false;
	}
	str->ReadWord( &Language ); // English is 0
	str->ReadDword( &StrRefCount );
	str->ReadDword( &Offset );
	if (StrRefCount >= STRREF_START) {
		Log(ERROR, "TLKImporter", "Too many strings (%d), increase STRREF_START.", StrRefCount);
		return false;
	}
	return true;
}

//when copying the token, skip spaces
inline const char* mystrncpy(char* dest, const char* source, int maxlength,
	char delim)
{
	while (*source && ( *source != delim ) && maxlength--) {
		char chr = *source++;
		if (chr!=' ') *dest++ = chr;
	}
	*dest = 0;
	return source;
}

/* -1	 - GABBER
		0	 - PROTAGONIST
		1-9 - PLAYERx
*/
static inline Actor *GetActorFromSlot(int slot)
{
	if (slot==-1) {
		const GameControl *gc = core->GetGameControl();
		if (gc) {
			return gc->dialoghandler->GetSpeaker();
		}
		return NULL;
	}
	const Game *game = core->GetGame();
	if (!game) {
		return NULL;
	}
	if (slot==0) {
		return game->GetPC(0, false); //protagonist
	}
	return game->FindPC(slot);
}

char *TLKImporter::Gabber() const
{
	const Actor *act = core->GetGameControl()->dialoghandler->GetSpeaker();
	if (act) {
		return strdup(act->LongName);
	}
	return strdup("?");
}

char *TLKImporter::CharName(int slot) const
{
	const Actor *act = GetActorFromSlot(slot);
	if (act) {
		return strdup(act->LongName);
	}
	return strdup("?");
}

int TLKImporter::ClassStrRef(int slot) const
{
	int clss = 0;
	const Actor *act = GetActorFromSlot(slot);
	if (act) {
		clss = act->GetActiveClass();
	}

	AutoTable tab("classes");
	if (!tab) {
		return -1;
	}
	int row = tab->FindTableValue("ID", clss, 0);
	return atoi(tab->QueryField(row,0) );
}

int TLKImporter::RaceStrRef(int slot) const
{
	int race = 0;
	const Actor *act = GetActorFromSlot(slot);
	if (act) {
		race=act->GetStat(IE_RACE);
	}

	AutoTable tab("races");
	if (!tab) {
		return -1;
	}
	int row = tab->FindTableValue(3, race, 0);
	return atoi(tab->QueryField(row,0) );
}

int TLKImporter::GenderStrRef(int slot, int malestrref, int femalestrref)
{
	const Actor *act = GetActorFromSlot(slot);
	if (act && (act->GetStat(IE_SEX)==SEX_FEMALE) ) {
		return femalestrref;
	}
	return malestrref;
}

//if this function returns -1 then it is not a built in token, dest may be NULL
int TLKImporter::BuiltinToken(const char* Token, char* dest)
{
	char* Decoded = NULL;
	gt_type *entry = NULL;

	//these are gender specific tokens, they are customisable by gender.2da
	if (gtmap.Lookup(Token, (void *&) entry) ) {
		Decoded = GetCString( GenderStrRef(entry->type, entry->male, entry->female) );
		goto exit_function;
	}

	//these are hardcoded, all engines are the same or don't use them
	if (!strcmp( Token, "DAYANDMONTH")) {
		ieDword dayandmonth=0;
		core->GetDictionary()->Lookup("DAYANDMONTH",dayandmonth);
		//preparing sub-tokens
		core->GetCalendar()->GetMonthName((int) dayandmonth);
		Decoded = GetCString( 15981, 0 );
		goto exit_function;
	}

	if (!strcmp( Token, "FIGHTERTYPE" )) {
		Decoded = GetCString( 10174, 0 );
		goto exit_function;
	}
	if (!strcmp( Token, "CLASS" )) {
		//allow this to be used by direct setting of the token
		int strref = ClassStrRef(-1);
		if (strref<=0) return -1;
		Decoded = GetCString( strref, 0);
		goto exit_function;
	}

	if (!strcmp( Token, "RACE" )) {
		Decoded = GetCString( RaceStrRef(-1), 0);
		goto exit_function;
	}

	// handle Player10 (max for MaxPartySize), then the rest
	if (!strncmp(Token, "PLAYER10", 8)) {
		Decoded = CharName(10);
		goto exit_function;
	}
	if (!strncmp( Token, "PLAYER", 6 )) {
		Decoded = CharName(Token[strlen(Token)-1]-'1');
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
	if (!strcmp( Token, "PRO_CLASS" )) {
		Decoded = GetCString( ClassStrRef(0), 0);
		goto exit_function;
	}
	if (!strcmp( Token, "PRO_RACE" )) {
		Decoded = GetCString( RaceStrRef(0), 0);
		goto exit_function;
	}
	if (!strcmp( Token, "MAGESCHOOL" )) {
		ieDword row = 0; //default value is 0 (generalist)
		//this is subject to change, the row number in magesch.2da
		core->GetDictionary()->Lookup( "MAGESCHOOL", row ); 
		AutoTable tm("magesch");
		if (tm) {
			const char* value = tm->QueryField( row, 2 );
			Decoded = GetCString( atoi( value ), 0 );
			goto exit_function;
		}
	}
	if (!strcmp( Token, "TM" )) {
		Decoded = strdup("\x99");
		goto exit_function;
	}

	return -1;	//not decided

	exit_function:
	if (Decoded) {
		size_t TokenLength = strlen(Decoded);
		if (dest) {
			memcpy( dest, Decoded, TokenLength );
		}
		//Decoded is always a copy
		free( Decoded );
		return static_cast<int>(TokenLength);
	}
	return -1;
}

bool TLKImporter::ResolveTags(char* dest, const char* source, int Length)
{
	char Token[MAX_VARIABLE_LENGTH + 1];
	int NewLength = 0;
	for (int i = 0; source[i]; i++) {
		if (source[i] == '<') {
			i = (int) (mystrncpy( Token, source + i + 1, MAX_VARIABLE_LENGTH, '>' ) - source);
			int TokenLength = BuiltinToken( Token, dest + NewLength );
			if (TokenLength == -1) {
				TokenLength = core->GetTokenDictionary()->GetValueLength( Token );
				if (TokenLength) {
					if (TokenLength + NewLength > Length) return false;
					core->GetTokenDictionary()->Lookup( Token, dest + NewLength, TokenLength );
				}
			}
			NewLength += TokenLength;
		} else if (source[i] == '[') {
			// voice actor directives
			const char* tmppoi = strchr(source + i + 1, ']');
			if (!tmppoi) break;
			i = (int) (tmppoi - source);
			if (NewLength > Length) return false;
		} else {
			dest[NewLength++] = source[i];
			if (NewLength > Length) return false;
		}
	}
	dest[NewLength] = 0;
	return true;
}

bool TLKImporter::GetNewStringLength(const char* string, int& Length)
{
	char Token[MAX_VARIABLE_LENGTH + 1];
	bool lChange = false;
	int NewLength = 0;
	for (int i = 0; i < Length; i++) {
		if (string[i] == '<') {
			// token
			lChange = true;
			i = (int) (mystrncpy( Token, string + i + 1, MAX_VARIABLE_LENGTH, '>' ) - string);
			int TokenLength = BuiltinToken( Token, NULL );
			if (TokenLength == -1) {
				NewLength += core->GetTokenDictionary()->GetValueLength( Token );
			} else {
				NewLength += TokenLength;
			}
		} else if (string[i] == '[') {
			// voice actor directives
			lChange = true;
			const char* tmppoi = strchr(string + i + 1, ']');
			if (!tmppoi) break;
			i += (int) (tmppoi - string) - i;
		} else {
			NewLength++;
		}
	}
	Length = NewLength;
	return lChange;
}

ieStrRef TLKImporter::UpdateString(ieStrRef strref, const char *newvalue)
{
	if (!OverrideTLK) {
		Log(ERROR, "TLKImporter", "Custom string is not supported by this game format.");
		return 0xffffffff;
	}

	return OverrideTLK->UpdateString(strref, newvalue);
}

String* TLKImporter::GetString(ieStrRef strref, ieDword flags)
{
	char* cstr = GetCString(strref, flags);
	String* string = StringFromCString(cstr);
	free(cstr);
	return string;
}

char* TLKImporter::GetCString(ieStrRef strref, ieDword flags)
{
	char* string;
	bool empty = !(flags & IE_STR_ALLOW_ZERO) && !strref;
	ieWord type;
	int Length;
	ieResRef SoundResRef;

	if (empty || strref >= STRREF_START || (strref >= BIO_START && strref <= BIO_END)) {
		if (OverrideTLK) {
			string = OverrideTLK->ResolveAuxString(strref, Length);
		} else {
			string = (char *) malloc(1);
			Length = 0;
			string[0] = 0;
		}
		type = 0;
		SoundResRef[0]=0;
	} else {
		ieDword Volume, Pitch, StrOffset;
		ieDword l;
		if (str->Seek( 18 + ( strref * 0x1A ), GEM_STREAM_START ) == GEM_ERROR) {
			return strdup("");
		}
		str->ReadWord( &type );
		str->ReadResRef( SoundResRef );
		// volume and pitch variance fields are known to be unused at minimum in bg1
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
	if (type & 2 && flags & IE_STR_SOUND && SoundResRef[0] != 0) {
		// GEM_SND_SPEECH will stop the previous sound source
		unsigned int flag = GEM_SND_RELATIVE | (flags & (GEM_SND_SPEECH | GEM_SND_QUEUE));
		core->GetAudioDrv()->Play(SoundResRef, SFX_CHAN_DIALOG, 0, 0, flag);
	}
	if (flags & IE_STR_STRREFON) {
		char* string2 = ( char* ) malloc( Length + 13 );
		sprintf( string2, "%u: %s", strref, string );
		free( string );
		return string2;
	}
	// remove the linefeed and carriage return if requested
	if (flags & IE_STR_REMOVE_NEWLINE) {
		core->StripLine( string, Length);
	}
	return string;
}

bool TLKImporter::HasAltTLK() const
{
	// only English (language id 0) has no alt files
	return Language;
}

StringBlock TLKImporter::GetStringBlock(ieStrRef strref, unsigned int flags)
{
	bool empty = !(flags & IE_STR_ALLOW_ZERO) && !strref;
	if (empty || strref >= StrRefCount) {
		return StringBlock();
	}
	ieWord type;
	str->Seek( 18 + ( strref * 0x1A ), GEM_STREAM_START );
	str->ReadWord( &type );
	ieResRef soundRef;
	str->ReadResRef( soundRef );
	return StringBlock(GetString( strref, flags ), soundRef);
}

#include "plugindef.h"

GEMRB_PLUGIN(0xBB6F380, "TLK File Importer")
PLUGIN_CLASS(IE_TLK_CLASS_ID, TLKImporter)
END_PLUGIN()
