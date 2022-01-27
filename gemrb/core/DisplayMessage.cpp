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

namespace GemRB {

GEM_EXPORT DisplayMessage * displaymsg = NULL;

static const auto DisplayFormatName = FMT_STRING(L"[color={:08X}]{} - [/color][p][color={:08X}]{}[/color][/p]");
static const auto DisplayFormatAction = FMT_STRING(L"[color={:08X}]{} - [/color][p][color={:08X}]{} {}[/color][/p]");
static const auto DisplayFormat = FMT_STRING(L"[p][color={:08X}]{}[/color][/p]");
static const auto DisplayFormatValue = FMT_STRING(L"[p][color={:08X}]{}: {}[/color][/p]");
static const auto DisplayFormatNameString = FMT_STRING(L"[color={:08X}]{} - [/color][p][color={:08X}]{}: {}[/color][/p]");
static const auto DisplayFormatSimple = FMT_STRING(L"[p]{}[/p]");

DisplayMessage::StrRefs DisplayMessage::SRefs;

bool DisplayMessage::EnableRollFeedback()
{
	ieDword feedback = 0;
	core->GetDictionary()->Lookup("EnableRollFeedback", feedback);
	return bool(feedback);
}

String DisplayMessage::ResolveStringRef(ieStrRef stridx)
{
	return core->GetString(stridx, STRING_FLAGS::RESOLVE_TAGS);
}

DisplayMessage::StrRefs::StrRefs()
{
	memset(table, -1, sizeof(table) );
}

bool DisplayMessage::StrRefs::LoadTable(const std::string& name)
{
	AutoTable tab = gamedata->LoadTable(name.c_str());
	if (tab) {
		for(int i=0;i<STRREF_COUNT;i++) {
			table[i] = tab->QueryFieldAsStrRef(i,0);
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
	return ieStrRef::INVALID;
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

void DisplayMessage::DisplayMarkupString(String Text) const
{
	TextArea *ta = core->GetMessageTextArea();
	if (ta)
		ta->AppendText(std::move(Text));
}

void DisplayMessage::DisplayString(const String& text) const
{
	DisplayMarkupString(fmt::format(DisplayFormatSimple, text));
}

Color DisplayMessage::GetSpeakerColor(String& name, const Scriptable *&speaker) const
{
	if(!speaker) {
		name = L"";
		return {};
	}

	Color speaker_color {0x80, 0, 0, 0xff};
	String string;
	// NOTE: name color was hardcoded to a limited list in the originals;
	// the 1PP mod tackled this restriction by altering the exe to use a bigger list.
	// We just generate a colour by looking at the existing palette instead.
	switch (speaker->Type) {
		case ST_ACTOR:
			string = Scriptable::As<Actor>(speaker)->GetName(-1);
			{
				auto pal16 = core->GetPalette16(((const Actor *) speaker)->GetStat(IE_MAJOR_COLOR));
				// cmleat4 from dark horizons sets all the colors to pitch black, so work around too dark results
				if (pal16[4].r + pal16[4].g + pal16[4].b < 75) {
					pal16[4].r = 75;
					pal16[4].g = 75;
					pal16[4].b = 75;
				}
				speaker_color = pal16[4];
			}
			break;
		case ST_TRIGGER:
		case ST_PROXIMITY:
		case ST_TRAVEL:
			string = core->GetString( speaker->DialogName );
			speaker_color = Color(0xc0, 0xc0, 0xc0, 0xff);
			break;
		default:
			break;
	}
	
	name = string;
	return speaker_color;
}

//simply displaying a constant string
void DisplayMessage::DisplayConstantString(size_t stridx, const Color &color, Scriptable *target) const
{
	if (stridx > STRREF_COUNT) return;
	String text = core->GetString(DisplayMessage::SRefs[stridx], STRING_FLAGS::SOUND);
	DisplayString(text, color, target);
}

void DisplayMessage::DisplayString(ieStrRef stridx, const Color &color, STRING_FLAGS flags) const
{
	if (stridx == ieStrRef::INVALID) return;
	String text = core->GetString(stridx, flags);
	DisplayString(text, color, NULL);
}

void DisplayMessage::DisplayString(const String& text, const Color &color, Scriptable *target) const
{
	if (!text.length()) return;

	Label *l = core->GetMessageLabel();
	if (l) {
		l->SetColors(color, ColorBlack);
		l->SetText(text);
	}

	const TextArea* ta = core->GetMessageTextArea();
	if (ta) {
		DisplayMarkupString(fmt::format(DisplayFormat, color.Packed(), text));
	}

	if (target && l == NULL && ta == NULL) {
		// overhead text only if we dont have somewhere else for the message
		target->SetOverheadText( text );
	}
}

// String format is
// blah : whatever
void DisplayMessage::DisplayConstantStringValue(size_t stridx, const Color &color, ieDword value) const
{
	if (stridx > STRREF_COUNT) return;
	String text = core->GetString(DisplayMessage::SRefs[stridx], STRING_FLAGS::SOUND);
	DisplayMarkupString(fmt::format(DisplayFormatValue, color.Packed(), text, value));
}

// String format is
// <charname> - blah blah : whatever
void DisplayMessage::DisplayConstantStringNameString(size_t stridx, const Color &color, size_t stridx2, const Scriptable *actor) const
{
	if (stridx > STRREF_COUNT) return;

	String name;
	Color actor_color = GetSpeakerColor(name, actor);
	String text = core->GetString(DisplayMessage::SRefs[stridx], STRING_FLAGS::SOUND);
	String text2 = core->GetString(DisplayMessage::SRefs[stridx2], STRING_FLAGS::SOUND);

	if (!text2.empty()) {
		DisplayMarkupString(fmt::format(DisplayFormatNameString, actor_color.Packed(), name, color.Packed(), text, text2));
	} else {
		DisplayMarkupString(fmt::format(DisplayFormatName, color.Packed(), name, color.Packed(), text));
	}
}

// String format is
// <charname> - blah blah
void DisplayMessage::DisplayConstantStringName(size_t stridx, const Color &color, const Scriptable *speaker) const
{
	if (stridx > STRREF_COUNT) return;
	if(!speaker) return;

	String text = core->GetString(DisplayMessage::SRefs[stridx], STRING_FLAGS::SOUND | STRING_FLAGS::SPEECH);
	DisplayStringName(text, color, speaker);
}

//Treats the constant string as a numeric format string, otherwise like the previous method
void DisplayMessage::DisplayConstantStringNameValue(size_t stridx, const Color &color, const Scriptable *speaker, int value) const
{
	if (stridx > STRREF_COUNT) return;
	if(!speaker) return;

	String fmt = core->GetString(DisplayMessage::SRefs[stridx], STRING_FLAGS::SOUND | STRING_FLAGS::SPEECH | STRING_FLAGS::RESOLVE_TAGS);
	DisplayStringName(fmt::format(fmt, value), color, speaker);
}

// String format is
// <charname> - blah blah <someoneelse>
void DisplayMessage::DisplayConstantStringAction(size_t stridx, const Color &color, const Scriptable *attacker, const Scriptable *target) const
{
	if (stridx > STRREF_COUNT) return;

	String name1, name2;

	Color attacker_color = GetSpeakerColor(name1, attacker);
	GetSpeakerColor(name2, target);

	String text = core->GetString( DisplayMessage::SRefs[stridx], STRING_FLAGS::SOUND | STRING_FLAGS::SPEECH );
	DisplayMarkupString(fmt::format(DisplayFormatAction, attacker_color.Packed(), name1, color.Packed(), text, name2));
}

void DisplayMessage::DisplayStringName(ieStrRef str, const Color &color, const Scriptable *speaker, STRING_FLAGS flags) const
{
	if (str == ieStrRef::INVALID) return;

	String text = core->GetString(str, flags);
	DisplayStringName(text, color, speaker);
}

void DisplayMessage::DisplayStringName(const String& text, const Color &color, const Scriptable *speaker) const
{
	if (!text.length() || !text.compare(L" ")) return;

	String name;
	Color speaker_color = GetSpeakerColor(name, speaker);

	if (name.length() == 0) {
		DisplayString(text, color, NULL);
	} else {
		DisplayMarkupString(fmt::format(DisplayFormatName, speaker_color.Packed(), name, color.Packed(), text));
	}
}
}
