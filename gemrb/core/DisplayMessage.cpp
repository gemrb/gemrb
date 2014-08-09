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
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*
*
*/

#include "DisplayMessage.h"

#include "strrefs.h"

#include "Interface.h"
#include "TableMgr.h"
#include "GUI/Label.h"
#include "GUI/TextArea.h"
#include "Scriptable/Actor.h"

#include <cstdarg>

namespace GemRB {

GEM_EXPORT DisplayMessage * displaymsg = NULL;

static int strref_table[STRREF_COUNT];

#define PALSIZE 8
static Color ActorColor[PALSIZE];
static const wchar_t* DisplayFormatName = L"[color=%06X]%s - [/color][p][color=%06X]%ls[/color][/p]";
static const wchar_t* DisplayFormatAction = L"[color=%06X]%s - [/color][p][color=%06X]%s %s[/color][/p]";
static const wchar_t* DisplayFormat = L"[p][color=%06X]%ls[/color][/p]";
static const wchar_t* DisplayFormatValue = L"[p][color=%06X]%s: %d[/color][/p]";
static const wchar_t* DisplayFormatNameString = L"[color=%06X]%s - [/color][p][color=%06X]%s: %s[/color][/p]";

DisplayMessage::DisplayMessage(void) {
	ReadStrrefs();
}

bool DisplayMessage::ReadStrrefs()
{
	int i;
	memset(strref_table,-1,sizeof(strref_table) );
	AutoTable tab("strings");
	if (!tab) {
		return false;
	}
	for(i=0;i<STRREF_COUNT;i++) {
		strref_table[i]=atoi(tab->QueryField(i,0));
	}
	return true;
}

void DisplayMessage::DisplayMarkupString(const String& Text) const
{
	assert(core->GetMessageLabel() == NULL);
	TextArea *ta = core->GetMessageTextArea();
	assert(ta);
	ta->AppendText( Text );
}

ieStrRef DisplayMessage::GetStringReference(int stridx) const
{
	return strref_table[stridx];
}

bool DisplayMessage::HasStringReference(int stridx) const
{
	return strref_table[stridx] != -1;
}

unsigned int DisplayMessage::GetSpeakerColor(const char *&name, const Scriptable *&speaker) const
{
	unsigned int speaker_color;

	if(!speaker) {
		name = "";
		return 0;
	}
	switch (speaker->Type) {
		case ST_ACTOR:
			name = speaker->GetName(-1);
			core->GetPalette( ((Actor *) speaker)->GetStat(IE_MAJOR_COLOR) & 0xFF, PALSIZE, ActorColor );
			speaker_color = (ActorColor[4].r<<16) | (ActorColor[4].g<<8) | ActorColor[4].b;
			break;
		case ST_TRIGGER: case ST_PROXIMITY: case ST_TRAVEL:
			name = core->GetCString( speaker->DialogName );
			speaker_color = 0xc0c0c0;
			break;
		default:
			name = "";
			speaker_color = 0x800000;
			break;
	}
	return speaker_color;
}


//simply displaying a constant string
void DisplayMessage::DisplayConstantString(int stridx, unsigned int color, Scriptable *target) const
{
	if (stridx<0) return;

	String* text = core->GetString( strref_table[stridx], IE_STR_SOUND );
	DisplayString(*text, color, target);
	delete text;
}

void DisplayMessage::DisplayString(int stridx, unsigned int color, ieDword flags) const
{
	if (stridx<0) return;
	String* text = core->GetString( stridx, flags);
	DisplayString(*text, color, NULL);
	delete text;

}

void DisplayMessage::DisplayString(const String& text, unsigned int color, Scriptable *target) const
{
	if (!text.length()) return;

	Label *l = core->GetMessageLabel();
	if (l) {
		const Color fore = { (color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF, (color >> 24) & 0xFF};
		l->SetColor( fore, ColorBlack );
		l->SetText(text);
	} else {
		int newlen = (int)(wcslen( DisplayFormat) + text.length() + 12);
		wchar_t* newstr = ( wchar_t* ) malloc( newlen * sizeof(wchar_t) );
		swprintf(newstr, newlen, DisplayFormat, color, text.c_str());
		TextArea* ta = core->GetMessageTextArea();
		if (ta) {
			DisplayMarkupString( newstr );
		} else if (target) {
			target->SetOverheadText( newstr );
		}
		free( newstr );
	}
}

// String format is
// blah : whatever
void DisplayMessage::DisplayConstantStringValue(int stridx, unsigned int color, ieDword value) const
{
	if (stridx<0) return;
	char* text = core->GetCString( strref_table[stridx], IE_STR_SOUND );

	int newlen = (int)(wcslen( DisplayFormat ) + strlen( text ) + 28);
	wchar_t* newstr = ( wchar_t* ) malloc( newlen * sizeof(wchar_t) );
	swprintf( newstr, newlen, DisplayFormatValue, color, text, value);

	core->FreeString( text );
	DisplayMarkupString( newstr );
	free( newstr );
}

// String format is
// <charname> - blah blah : whatever
void DisplayMessage::DisplayConstantStringNameString(int stridx, unsigned int color, int stridx2, const Scriptable *actor) const
{
	unsigned int actor_color;
	const char* name = NULL;

	if (stridx<0) return;
	actor_color = GetSpeakerColor(name, actor);
	char* text = core->GetCString( strref_table[stridx], IE_STR_SOUND );
	char* text2 = core->GetCString( strref_table[stridx2], IE_STR_SOUND );

	int newlen = (int)(wcslen( DisplayFormat ) + strlen(name) + strlen( text ) + strlen(text2) + 20);
	wchar_t* newstr = ( wchar_t* ) malloc( newlen * sizeof(wchar_t) );
	if (strlen(text2)) {
		swprintf( newstr, newlen, DisplayFormatNameString, actor_color, name, color, text, text2 );
	} else {
		swprintf( newstr, newlen, DisplayFormatName, color, name, color, text );
	}
	core->FreeString( text );
	core->FreeString( text2 );
	DisplayMarkupString( newstr );
	free( newstr );
}

// String format is
// <charname> - blah blah
void DisplayMessage::DisplayConstantStringName(int stridx, unsigned int color, const Scriptable *speaker) const
{
	if (stridx<0) return;
	if(!speaker) return;

	String* text = core->GetString( strref_table[stridx], IE_STR_SOUND|IE_STR_SPEECH );
	DisplayStringName(*text, color, speaker);
	delete text;
}

//Treats the constant string as a numeric format string, otherwise like the previous method
void DisplayMessage::DisplayConstantStringNameValue(int stridx, unsigned int color, const Scriptable *speaker, int value) const
{
	if (stridx<0) return;
	if(!speaker) return;

	String* text = core->GetString( strref_table[stridx], IE_STR_SOUND|IE_STR_SPEECH );
	//allow for a number
	size_t bufflen = text->length() + 6;
	wchar_t* newtext = ( wchar_t* ) malloc( bufflen * sizeof(wchar_t));
	swprintf( newtext, bufflen, text->c_str(), value );
	DisplayStringName(newtext, color, speaker);
	free(newtext);
	delete text;
}

// String format is
// <charname> - blah blah <someoneelse>
void DisplayMessage::DisplayConstantStringAction(int stridx, unsigned int color, const Scriptable *attacker, const Scriptable *target) const
{
	unsigned int attacker_color;
	const char *name1 = NULL, *name2 = NULL;

	if (stridx<0) return;

	GetSpeakerColor(name2, target);
	attacker_color = GetSpeakerColor(name1, attacker);

	char* text = core->GetCString( strref_table[stridx], IE_STR_SOUND|IE_STR_SPEECH );
	int newlen = (int)(wcslen( DisplayFormatAction ) + strlen(name1) + strlen(name2) + strlen( text ) + 18);
	wchar_t* newstr = ( wchar_t* ) malloc( newlen * sizeof(wchar_t));
	swprintf( newstr, newlen, DisplayFormatAction, attacker_color, name1, color,text, name2);
	core->FreeString( text );
	DisplayMarkupString( newstr );
	free( newstr );
}

// display tokenized strings like ~Open lock check. Open lock skill %d vs. lock difficulty %d (%d DEX bonus).~
void DisplayMessage::DisplayRollStringName(int stridx, unsigned int color, const Scriptable *speaker, ...) const
{
	ieDword feedback = 0;
	core->GetDictionary()->Lookup("EnableRollFeedback", feedback);
	if (feedback) {
		wchar_t tmp[200];
		va_list numbers;
		va_start(numbers, speaker);
		// fill it out
		String* str = core->GetString(stridx);
		vswprintf(tmp, sizeof(tmp), str->c_str(), numbers);
		delete str;
		displaymsg->DisplayStringName(tmp, color, speaker);
		va_end(numbers);
	}
}

void DisplayMessage::DisplayStringName(int stridx, unsigned int color, const Scriptable *speaker, ieDword flags) const
{
	if (stridx<0) return;

	String* text = core->GetString( stridx, flags);
	DisplayStringName(*text, color, speaker);
	delete text;
}

void DisplayMessage::DisplayStringName(const String& text, unsigned int color, const Scriptable *speaker) const
{
	if (!text.length()) return;

	unsigned int speaker_color;
	const char* name;
	speaker_color = GetSpeakerColor(name, speaker);

	// if there is no name, use the script name to help debugging
	if (!name[0]) {
		name = speaker->GetScriptName();
	}

	int newlen = (int)(wcslen(DisplayFormatName) + strlen(name) + text.length() + 18);
	wchar_t* newstr = (wchar_t *) malloc(newlen * sizeof(wchar_t));
	swprintf(newstr, newlen, DisplayFormatName, speaker_color, name, color, text.c_str());
	DisplayMarkupString(newstr);
	free(newstr);
}

}
