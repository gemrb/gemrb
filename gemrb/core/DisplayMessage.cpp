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
static const char* DisplayFormatName = "[color=%06X]%s - [/color][p][color=%06X]%s[/color][/p]";
static const char* DisplayFormatAction = "[color=%06X]%s - [/color][p][color=%06X]%s %s[/color][/p]";
static const char* DisplayFormat = "[/color][p][color=%06X]%s[/color][/p]";
static const char* DisplayFormatValue = "[/color][p][color=%06X]%s: %d[/color][/p]";
static const char* DisplayFormatNameString = "[color=%06X]%s - [/color][p][color=%06X]%s: %s[/color][/p]";

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

void DisplayMessage::DisplayString(const char* Text, Scriptable *target) const
{
	Label *l = core->GetMessageLabel();
	if (l) {
		l->SetText(Text);
	}
	TextArea *ta = core->GetMessageTextArea();
	if (ta) {
		ta->AppendText( Text, -1 );
	} else {
		if(target) {
			target->DisplayHeadText(Text);
		}
	}
}

ieStrRef DisplayMessage::GetStringReference(int stridx) const
{
	return strref_table[stridx];
}

bool DisplayMessage::HasStringReference(int stridx) const
{
	return strref_table[stridx] != -1;
}

unsigned int DisplayMessage::GetSpeakerColor(char *&name, const Scriptable *&speaker) const
{
	unsigned int speaker_color;
	char *tmp;

	if(!speaker) return 0;
	switch (speaker->Type) {
		case ST_ACTOR:
			name = strdup(speaker->GetName(-1));
			core->GetPalette( ((Actor *) speaker)->GetStat(IE_MAJOR_COLOR) & 0xFF, PALSIZE, ActorColor );
			speaker_color = (ActorColor[4].r<<16) | (ActorColor[4].g<<8) | ActorColor[4].b;
			break;
		case ST_TRIGGER: case ST_PROXIMITY: case ST_TRAVEL:
			tmp = core->GetString(speaker->DialogName);
			name = strdup(tmp);
			core->FreeString(tmp);
			speaker_color = 0xc0c0c0;
			break;
		default:
			name = strdup("");
			speaker_color = 0x800000;
			break;
	}
	return speaker_color;
}


//simply displaying a constant string
void DisplayMessage::DisplayConstantString(int stridx, unsigned int color, Scriptable *target) const
{
	if (stridx<0) return;
	char* text = core->GetString( strref_table[stridx], IE_STR_SOUND );
	DisplayString(text, color, target);
	core->FreeString(text);
}

void DisplayMessage::DisplayString(int stridx, unsigned int color, ieDword flags) const
{
	if (stridx<0) return;
	char* text = core->GetString( stridx, flags);
	DisplayString(text, color, NULL);
	core->FreeString(text);
}

void DisplayMessage::DisplayString(const char *text, unsigned int color, Scriptable *target) const
{
	if (!text) return;
	int newlen = (int)(strlen( DisplayFormat) + strlen( text ) + 12);
	char* newstr = ( char* ) malloc( newlen );
	snprintf( newstr, newlen, DisplayFormat, color, text );
	DisplayString( newstr, target );
	free( newstr );
}

// String format is
// blah : whatever
void DisplayMessage::DisplayConstantStringValue(int stridx, unsigned int color, ieDword value) const
{
	if (stridx<0) return;
	char* text = core->GetString( strref_table[stridx], IE_STR_SOUND );
	int newlen = (int)(strlen( DisplayFormat ) + strlen( text ) + 28);
	char* newstr = ( char* ) malloc( newlen );
	snprintf( newstr, newlen, DisplayFormatValue, color, text, (int) value );
	core->FreeString( text );
	DisplayString( newstr );
	free( newstr );
}

// String format is
// <charname> - blah blah : whatever
void DisplayMessage::DisplayConstantStringNameString(int stridx, unsigned int color, int stridx2, const Scriptable *actor) const
{
	unsigned int actor_color;
	char *name = 0;

	if (stridx<0) return;
	actor_color = GetSpeakerColor(name, actor);
	char* text = core->GetString( strref_table[stridx], IE_STR_SOUND );
	char* text2 = core->GetString( strref_table[stridx2], IE_STR_SOUND );
	int newlen = (int)(strlen( DisplayFormat ) + strlen(name) + strlen( text ) + strlen(text2) + 20);
	char* newstr = ( char* ) malloc( newlen );
	if (strlen(text2)) {
		snprintf( newstr, newlen, DisplayFormatNameString, actor_color, name, color, text, text2 );
	} else {
		snprintf( newstr, newlen, DisplayFormatName, color, name, color, text );
	}
	free( name );
	core->FreeString( text );
	core->FreeString( text2 );
	DisplayString( newstr );
	free( newstr );
}

// String format is
// <charname> - blah blah
void DisplayMessage::DisplayConstantStringName(int stridx, unsigned int color, const Scriptable *speaker) const
{
	if (stridx<0) return;
	if(!speaker) return;

	char* text = core->GetString( strref_table[stridx], IE_STR_SOUND|IE_STR_SPEECH );
	DisplayStringName(text, color, speaker);
	core->FreeString(text);
}

//Treats the constant string as a numeric format string, otherwise like the previous method
void DisplayMessage::DisplayConstantStringNameValue(int stridx, unsigned int color, const Scriptable *speaker, int value) const
{
	if (stridx<0) return;
	if(!speaker) return;

	char* text = core->GetString( strref_table[stridx], IE_STR_SOUND|IE_STR_SPEECH );
	//allow for a number
	int bufflen = strlen(text)+6;
	char* newtext = ( char* ) malloc( bufflen );
	snprintf( newtext, bufflen, text, value );
	core->FreeString(text);
	DisplayStringName(newtext, color, speaker);
	free(newtext);
}

// String format is
// <charname> - blah blah <someoneelse>
void DisplayMessage::DisplayConstantStringAction(int stridx, unsigned int color, const Scriptable *attacker, const Scriptable *target) const
{
	unsigned int attacker_color;
	char *name1 = 0;
	char *name2 = 0;

	if (stridx<0) return;

	GetSpeakerColor(name2, target);
	attacker_color = GetSpeakerColor(name1, attacker);

	char* text = core->GetString( strref_table[stridx], IE_STR_SOUND|IE_STR_SPEECH );
	int newlen = (int)(strlen( DisplayFormatAction ) + strlen( name1 ) +
		+ strlen( name2 ) + strlen( text ) + 18);
	char* newstr = ( char* ) malloc( newlen );
	snprintf( newstr, newlen, DisplayFormatAction, attacker_color, name1, color,
		text, name2);
	free( name1 );
	free( name2 );
	core->FreeString( text );
	DisplayString( newstr );
	free( newstr );
}

// display tokenized strings like ~Open lock check. Open lock skill %d vs. lock difficulty %d (%d DEX bonus).~
void DisplayMessage::DisplayRollStringName(int stridx, unsigned int color, const Scriptable *speaker, ...) const
{
	ieDword feedback = 0;
	core->GetDictionary()->Lookup("EnableRollFeedback", feedback);
	if (feedback) {
		char tmp[200];
		va_list numbers;
		va_start(numbers, speaker);
		// fill it out
		vsnprintf(tmp, sizeof(tmp), core->GetString(stridx), numbers);
		displaymsg->DisplayStringName(tmp, color, speaker);
		va_end(numbers);
	}
}

void DisplayMessage::DisplayStringName(int stridx, unsigned int color, const Scriptable *speaker, ieDword flags) const
{
	if (stridx<0) return;

	char* text = core->GetString( stridx, flags);
	DisplayStringName(text, color, speaker);
	core->FreeString( text );
}

void DisplayMessage::DisplayStringName(const char *text, unsigned int color, const Scriptable *speaker) const
{
	unsigned int speaker_color;
	char *name = 0;

	if (!text) return;
	speaker_color = GetSpeakerColor(name, speaker);

	if (strcmp(name, "")) {
		int newlen = strlen(DisplayFormatName) + strlen(name) + strlen(text) + 18;
		char* newstr = (char *) malloc(newlen);
		snprintf(newstr, newlen, DisplayFormatName, speaker_color, name, color, text);
		DisplayString(newstr);
		free(newstr);
	} else {
		DisplayString(text, color, NULL);
	}
	free(name);
}
}
