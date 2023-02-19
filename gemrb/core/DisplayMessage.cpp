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

GEM_EXPORT DisplayMessage * displaymsg = nullptr;

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
	AutoTable tab = gamedata->LoadTable(name);
	if (tab) {
		for (int i = 0; i < STRREF_COUNT; i++) {
			table[i] = tab->QueryFieldAsStrRef(i, 0);
		}
		loadedTable = name;
	} else {
		Log(ERROR, "DisplayMessage", "Unable to initialize DisplayMessage::StrRefs");
		return false;
	}

	// only pst has flags and complications
	// they could have repurposed more verbal constants, but no, they built another layer instead
	if (tab->QueryField(0, 1) != tab->QueryDefault()) {
		for (int i = 0; i < STRREF_COUNT; i++) {
			std::string flag = tab->QueryField(i, 1);
			if (flag.length() == 1) {
				flags[i] = atoi(flag.c_str());
			} else {
				flags[i] = -1;
				const auto& parts = Explode(flag, ':');
				// two more refs
				extraRefs[i] = std::make_pair(ieStrRef(atoi(parts[0].c_str())), ieStrRef(atoi(parts[1].c_str())));
			}
		}
	}

	return true;
}

ieStrRef DisplayMessage::StrRefs::Get(size_t idx, const Scriptable* speaker) const
{
	if (idx < STRREF_COUNT) {
		if (flags[idx] == 0 || !speaker || speaker->Type != ST_ACTOR) {
			return table[idx];
		}

		// handle PST personalized strings
		const Actor* gabber = Scriptable::As<Actor>(speaker);
		if (flags[idx] == -1) {
			if (gabber->GetStat(IE_SPECIFIC) == 2) return table[idx]; // TNO
			if (gabber->GetStat(IE_SPECIFIC) == 8) return extraRefs.at(idx).first; // Annah
			return extraRefs.at(idx).second; // anyone else
		}

		// handle flags mode 1 and 2
		// figure out PC index (TNO, Morte, Annah, Dakkon, FFG, Nordom, Ignus, Vhailor), anyone else
		// then use it to calculate presonalized feedback strings
		int pcOffset;
		int specific = gabber->GetStat(IE_SPECIFIC);
		const std::array<int, 8> spec2offset = { 0, 7, 5, 6, 4, 3, 2, 1 };
		if (specific >= 2 && specific <= 9) {
			pcOffset = spec2offset[specific - 2];
		} else if (flags[idx] == 2) {
			pcOffset = spec2offset.size();
		} else { // rare, but could happen
			pcOffset = 6; // use Ignus as fallback
		}

		return ieStrRef(int(table[idx]) + pcOffset);
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

ieStrRef DisplayMessage::GetStringReference(size_t idx, const Scriptable* speaker)
{
	return SRefs.Get(idx, speaker);
}

bool DisplayMessage::HasStringReference(size_t idx)
{
	return SRefs.Get(idx, nullptr) != ieStrRef(-1);
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
	// NOTE: name color was hardcoded to a limited list in the originals;
	// the 1PP mod tackled this restriction by altering the exe to use a bigger list.
	// We just generate a colour by looking at the existing palette instead.
	switch (speaker->Type) {
		case ST_ACTOR:
			name = Scriptable::As<Actor>(speaker)->GetDefaultName();
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
			name = core->GetString( speaker->DialogName );
			speaker_color = Color(0xc0, 0xc0, 0xc0, 0xff);
			break;
		default:
			name = L"";
			break;
	}
	
	return speaker_color;
}

//simply displaying a constant string
void DisplayMessage::DisplayConstantString(size_t stridx, GUIColors color, Scriptable *target) const
{
	if (stridx > STRREF_COUNT) return;
	String text = core->GetString(SRefs.Get(stridx, target), STRING_FLAGS::SOUND);
	DisplayString(text, GetColor(color), target);
}

void DisplayMessage::DisplayString(ieStrRef stridx, const Color &color, STRING_FLAGS flags) const
{
	if (stridx == ieStrRef::INVALID) return;
	DisplayString(core->GetString(stridx, flags), color, nullptr);
}

void DisplayMessage::DisplayString(String text, const Color &color, Scriptable *target) const
{
	const TextArea* ta = core->GetMessageTextArea();
	if (ta) {
		DisplayMarkupString(fmt::format(DisplayFormat, color.Packed(), text));
	}
	
	Label *l = core->GetMessageLabel();
	if (l) {
		l->SetColors(color, ColorBlack);
		l->SetText(std::move(text));
	}

	if (target && l == nullptr && ta == nullptr) {
		// overhead text only if we dont have somewhere else for the message
		target->SetOverheadText(std::move(text));
	}
}

std::map<GUIColors, std::string> DisplayMessage::GetAllColors() const
{
	std::map<GUIColors, std::string> auxiliaryColors;
	AutoTable colorTable = gamedata->LoadTable("colors", true);
	assert(colorTable);
	TableMgr::index_t index = static_cast<TableMgr::index_t>(GUIColors::FIRST_COLOR);
	TableMgr::index_t finish = static_cast<TableMgr::index_t>(GUIColors::LAST_COLOR);
	while (index < finish) {
		auxiliaryColors[static_cast<GUIColors>(index)] = colorTable->GetRowName(index);
		index++;
	}
	return auxiliaryColors;
}

Color DisplayMessage::GetColor(GUIColors color) const
{
	const auto it = GUIColorNames.find(color);
	if (it != GUIColorNames.end()) {
		return gamedata->GetColor(it->second);
	}
	return gamedata->GetColor("");
}

void DisplayMessage::DisplayString(ieStrRef stridx, GUIColors color, STRING_FLAGS flags) const
{
	DisplayString(stridx, GetColor(color), flags);
}

void DisplayMessage::DisplayString(const String& text, GUIColors color, Scriptable *target) const
{
	DisplayString(text, GetColor(color), target);
}

// String format is
// blah : whatever
void DisplayMessage::DisplayConstantStringValue(size_t stridx, GUIColors color, ieDword value) const
{
	if (stridx > STRREF_COUNT) return;
	String text = core->GetString(SRefs.Get(stridx, nullptr), STRING_FLAGS::SOUND);
	DisplayMarkupString(fmt::format(DisplayFormatValue, GetColor(color).Packed(), text, value));
}

// String format is
// <charname> - blah blah : whatever
void DisplayMessage::DisplayConstantStringNameString(size_t stridx, GUIColors color, size_t stridx2, const Scriptable *actor) const
{
	if (stridx > STRREF_COUNT) return;

	String name;
	Color actor_color = GetSpeakerColor(name, actor);
	Color used_color = GetColor(color);
	String text = core->GetString(SRefs.Get(stridx, actor), STRING_FLAGS::SOUND);
	String text2 = core->GetString(SRefs.Get(stridx2, actor), STRING_FLAGS::SOUND);

	if (!text2.empty()) {
		DisplayMarkupString(fmt::format(DisplayFormatNameString, actor_color.Packed(), name, used_color.Packed(), text, text2));
	} else {
		DisplayMarkupString(fmt::format(DisplayFormatName, used_color.Packed(), name, used_color.Packed(), text));
	}
}

// String format is
// <charname> - blah blah
void DisplayMessage::DisplayConstantStringName(size_t stridx, const Color &color, const Scriptable *speaker) const
{
	if (stridx > STRREF_COUNT) return;
	if(!speaker) return;

	String text = core->GetString(SRefs.Get(stridx, speaker), STRING_FLAGS::SOUND | STRING_FLAGS::SPEECH);
	DisplayStringName(std::move(text), color, speaker);
}

// String format is
// <charname> - blah blah
void DisplayMessage::DisplayConstantStringName(size_t stridx, GUIColors color, const Scriptable *speaker) const
{
	DisplayConstantStringName(stridx, GetColor(color), speaker);
}

//Treats the constant string as a numeric format string, otherwise like the previous method
void DisplayMessage::DisplayConstantStringNameValue(size_t stridx, GUIColors color, const Scriptable *speaker, int value) const
{
	if (stridx > STRREF_COUNT) return;
	if(!speaker) return;
	String fmt = core->GetString(SRefs.Get(stridx, speaker), STRING_FLAGS::SOUND | STRING_FLAGS::SPEECH | STRING_FLAGS::RESOLVE_TAGS);
	DisplayStringName(fmt::format(fmt, value), GetColor(color), speaker);
}

// String format is
// <charname> - blah blah <someoneelse>
void DisplayMessage::DisplayConstantStringAction(size_t stridx, GUIColors color, const Scriptable *attacker, const Scriptable *target) const
{
	if (stridx > STRREF_COUNT) return;

	String name1, name2;

	Color attacker_color = GetSpeakerColor(name1, attacker);
	Color used_color = GetColor(color);
	GetSpeakerColor(name2, target);

	String text = core->GetString(SRefs.Get(stridx, attacker), STRING_FLAGS::SOUND | STRING_FLAGS::SPEECH);
	DisplayMarkupString(fmt::format(DisplayFormatAction, attacker_color.Packed(), name1, used_color.Packed(), text, name2));
}

void DisplayMessage::DisplayStringName(ieStrRef str, const Color &color, const Scriptable *speaker, STRING_FLAGS flags) const
{
	if (str == ieStrRef::INVALID) return;

	DisplayStringName(core->GetString(str, flags), color, speaker);
}

void DisplayMessage::DisplayStringName(String text, const Color &color, const Scriptable *speaker) const
{
	if (!text.length() || !text.compare(L" ")) return;

	String name;
	Color speaker_color = GetSpeakerColor(name, speaker);

	if (name.length() == 0) {
		DisplayString(std::move(text), color, nullptr);
	} else {
		DisplayMarkupString(fmt::format(DisplayFormatName, speaker_color.Packed(), name, color.Packed(), text));
	}
}

void DisplayMessage::DisplayStringName(ieStrRef stridx, GUIColors color, const Scriptable *speaker, STRING_FLAGS flags) const
{
	DisplayStringName(stridx, GetColor(color), speaker, flags);
}

void DisplayMessage::DisplayStringName(String text, GUIColors color, const Scriptable *speaker) const
{
	DisplayStringName(text, GetColor(color), speaker);
}

}
