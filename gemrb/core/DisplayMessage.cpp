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

#include "Interface.h"
#include "TableMgr.h"
#include "GUI/Label.h"
#include "GUI/TextArea.h"
#include "Scriptable/Actor.h"

#include <cstdarg>

namespace GemRB {

GEM_EXPORT DisplayMessage * displaymsg = NULL;

#define PALSIZE 8
static Color ActorColor[PALSIZE];
static const wchar_t* DisplayFormatName = L"[color=%06X]%ls - [/color][p][color=%06X]%ls[/color][/p]";
static const wchar_t* DisplayFormatAction = L"[color=%06X]%ls - [/color][p][color=%06X]%ls %ls[/color][/p]";
static const wchar_t* DisplayFormat = L"[p][color=%06X]%ls[/color][/p]";
static const wchar_t* DisplayFormatValue = L"[p][color=%06X]%ls: %d[/color][/p]";
static const wchar_t* DisplayFormatNameString = L"[color=%06X]%ls - [/color][p][color=%06X]%ls: %ls[/color][/p]";
static const wchar_t* DisplayFormatSimple = L"[p]%ls[/p]";

DisplayMessage::StrRefs DisplayMessage::SRefs;

DisplayMessage::StrRefs::StrRefs()
{
	memset(table, -1, sizeof(table) );
}

bool DisplayMessage::StrRefs::LoadTable(const std::string& name)
{
	AutoTable tab(name.c_str());
	if (tab) {
		for(int i=0;i<STRREF_COUNT;i++) {
			table[i]=atoi(tab->QueryField(i,0));
		}
		loadedTable = name;
		return true;
	} else {
		Log(ERROR, "DisplayMessage", "Unable to initialize DisplayMessage::StrRefs");
	}
	return false;
}

ieStrRef DisplayMessage::StrRefs::operator[](size_t idx) const
{
	if (idx < STRREF_COUNT) {
		return table[idx];
	}
	return -1;
}

void DisplayMessage::LoadStringRefs()
{
	// "strings" is the dafault table. we could, in theory, load other tables
	static const std::string stringTableName = "strings";
	if (SRefs.loadedTable != stringTableName) {
		SRefs.LoadTable(stringTableName);
	}
}

ieStrRef DisplayMessage::GetStringReference(size_t idx)
{
	return DisplayMessage::SRefs[idx];
}

bool DisplayMessage::HasStringReference(size_t idx)
{
	return DisplayMessage::SRefs[idx] != ieStrRef(-1);
}


DisplayMessage::DisplayMessage()
{
	LoadStringRefs();
}

void DisplayMessage::DisplayMarkupString(const String& Text) const
{
	TextArea *ta = core->GetMessageTextArea();
	if (ta)
		ta->AppendText( Text );
}

void DisplayMessage::DisplayString(const String& text) const
{
	size_t newlen = wcslen(DisplayFormatSimple) + text.length() + 1;
	wchar_t *newstr = (wchar_t *) malloc(newlen * sizeof(wchar_t));
	swprintf(newstr, newlen, DisplayFormatSimple, text.c_str());
	DisplayMarkupString(newstr);
	free(newstr);
}

unsigned int DisplayMessage::GetSpeakerColor(String& name, const Scriptable *&speaker) const
{
	unsigned int speaker_color;
	name = L"";

	if(!speaker) {
		return 0;
	}
	String* string = NULL;
	switch (speaker->Type) {
		case ST_ACTOR:
			string = StringFromCString(speaker->GetName(-1));
			core->GetPalette( ((Actor *) speaker)->GetStat(IE_MAJOR_COLOR) & 0xFF, PALSIZE, ActorColor );
			speaker_color = (ActorColor[4].r<<16) | (ActorColor[4].g<<8) | ActorColor[4].b;
			break;
		case ST_TRIGGER:
		case ST_PROXIMITY:
		case ST_TRAVEL:
			string = core->GetString( speaker->DialogName );
			speaker_color = 0xc0c0c0;
			break;
		default:
			speaker_color = 0x800000;
			break;
	}
	if (string) {
		name = *string;
		delete string;
	}

	return speaker_color;
}

//simply displaying a constant string
void DisplayMessage::DisplayConstantString(int stridx, unsigned int color, Scriptable *target) const
{
	if (stridx<0) return;
	String* text = core->GetString( DisplayMessage::SRefs[stridx], IE_STR_SOUND );
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
		const Color fore = { (ieByte)((color >> 16) & 0xFF), (ieByte)((color >> 8) & 0xFF), (ieByte)(color & 0xFF), (ieByte)((color >> 24) & 0xFF)};
		l->SetColor( fore, ColorBlack );
		l->SetText(text);
	} else {
		size_t newlen = wcslen( DisplayFormat) + text.length() + 12;
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
	String* text = core->GetString( DisplayMessage::SRefs[stridx], IE_STR_SOUND );
	if (!text) {
		Log(WARNING, "DisplayMessage", "Unable to display message for stridx %d", stridx);
		return;
	}

	size_t newlen = wcslen( DisplayFormatValue ) + text->length() + 10;
	wchar_t* newstr = ( wchar_t* ) malloc( newlen * sizeof(wchar_t) );
	swprintf( newstr, newlen, DisplayFormatValue, color, text->c_str(), value);

	delete text;
	DisplayMarkupString( newstr );
	free( newstr );
}

// String format is
// <charname> - blah blah : whatever
void DisplayMessage::DisplayConstantStringNameString(int stridx, unsigned int color, int stridx2, const Scriptable *actor) const
{
	if (stridx<0) return;

	String name;
	unsigned int actor_color = GetSpeakerColor(name, actor);
	String* text = core->GetString( DisplayMessage::SRefs[stridx], IE_STR_SOUND );
	if (!text) {
		Log(WARNING, "DisplayMessage", "Unable to display message for stridx %d", stridx);
		return;
	}
	String* text2 = core->GetString( DisplayMessage::SRefs[stridx2], IE_STR_SOUND );

	size_t newlen = text->length() + name.length();
	if (text2) {
		newlen += wcslen(DisplayFormatNameString) + text2->length();
	} else {
		newlen += wcslen(DisplayFormatName);
	}

	wchar_t* newstr = ( wchar_t* ) malloc( newlen * sizeof(wchar_t) );
	if (text2) {
		swprintf( newstr, newlen, DisplayFormatNameString, actor_color, name.c_str(), color, text->c_str(), text2->c_str() );
	} else {
		swprintf( newstr, newlen, DisplayFormatName, color, name.c_str(), color, text->c_str() );
	}
	delete text;
	delete text2;
	DisplayMarkupString( newstr );
	free( newstr );
}

// String format is
// <charname> - blah blah
void DisplayMessage::DisplayConstantStringName(int stridx, unsigned int color, const Scriptable *speaker) const
{
	if (stridx<0) return;
	if(!speaker) return;

	String* text = core->GetString( DisplayMessage::SRefs[stridx], IE_STR_SOUND|IE_STR_SPEECH );
	DisplayStringName(*text, color, speaker);
	delete text;
}

//Treats the constant string as a numeric format string, otherwise like the previous method
void DisplayMessage::DisplayConstantStringNameValue(int stridx, unsigned int color, const Scriptable *speaker, int value) const
{
	if (stridx<0) return;
	if(!speaker) return;

	String* text = core->GetString( DisplayMessage::SRefs[stridx], IE_STR_SOUND|IE_STR_SPEECH );
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
	if (stridx<0) return;

	unsigned int attacker_color;
	String name1, name2;

	attacker_color = GetSpeakerColor(name1, attacker);
	GetSpeakerColor(name2, target);

	String* text = core->GetString( DisplayMessage::SRefs[stridx], IE_STR_SOUND|IE_STR_SPEECH );
	if (!text) {
		Log(WARNING, "DisplayMessage", "Unable to display message for stridx %d", stridx);
		return;
	}

	size_t newlen = wcslen( DisplayFormatAction ) + name1.length() + name2.length() + text->length() + 18;
	wchar_t* newstr = ( wchar_t* ) malloc( newlen * sizeof(wchar_t));
	swprintf( newstr, newlen, DisplayFormatAction, attacker_color, name1.c_str(), color, text->c_str(), name2.c_str());
	delete text;
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
		vswprintf(tmp, sizeof(tmp)/sizeof(tmp[0]), str->c_str(), numbers);
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
	String name;
	speaker_color = GetSpeakerColor(name, speaker);

	if (name.length() == 0) {
		DisplayString(text, color, NULL);
	} else {
		size_t newlen = wcslen(DisplayFormatName) + name.length() + text.length() + 18;
		wchar_t* newstr = (wchar_t *) malloc(newlen * sizeof(wchar_t));
		swprintf(newstr, newlen, DisplayFormatName, speaker_color, name.c_str(), color, text.c_str());
		DisplayMarkupString(newstr);
		free(newstr);
	}
}
}
