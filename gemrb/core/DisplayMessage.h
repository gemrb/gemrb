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

/**
 * @file DisplayMessage.h
 * Declaration of the DisplayMessage class used for displaying messages in
 * game message window
 */

#ifndef DISPLAYMESSAGE_H
#define DISPLAYMESSAGE_H

#include "exports.h"
#include "strrefs.h"
#include "StringMgr.h"
#include "RGBAColor.h"

#include <string>
#include <map>

namespace GemRB {

/** 
 * Indices for externalized GUI colors in colors.2da
 **/
enum class GUIColors {
	FIRST_COLOR,
	TOOLTIP = 0,
	TOOLTIPBG,
	MAPICNBG,
	MAPTXTBG,
	ALTDOOR,
	HOVERDOOR,
	HIDDENDOOR,
	ALTCONTAINER,
	HOVERCONTAINER,
	WHITE,
	RED,
	LIGHTGREY,
	XPCHANGE,
	GOLD,
	DIALOG,
	DIALOGPARTY,
	FLOAT_TXT_ACTOR,
	FLOAT_TXT_INFO,
	FLOAT_TXT_OTHER,
	LAST_COLOR
};

class Scriptable;

class GEM_EXPORT DisplayMessage
{
private:
	struct StrRefs {
		std::string loadedTable;
		ieStrRef table[STRREF_COUNT];
		int flags[STRREF_COUNT]{};
		std::map<int, std::pair<ieStrRef, ieStrRef>> extraRefs;

		StrRefs();
		bool LoadTable(const std::string& name);
		ieStrRef Get(size_t, const Scriptable*) const;
	};
	static StrRefs SRefs;
	static void LoadStringRefs();
	
	static bool EnableRollFeedback();
	static String ResolveStringRef(ieStrRef);
	
	/** returns the speaker's color and name */
	Color GetSpeakerColor(String& name, const Scriptable *&speaker) const;
	/** displays a string in the textarea, starting with speaker's name */
	void DisplayStringName(String text, const Color &color, const Scriptable *speaker) const;
	/** displays a string in the textarea */
	void DisplayString(String text, const Color &color, Scriptable *target) const;
	std::map<GUIColors, std::string> GetAllColors() const;
	const std::map<GUIColors, std::string> GUIColorNames = DisplayMessage::GetAllColors();

public:
	static ieStrRef GetStringReference(size_t, const Scriptable* = nullptr);
	static bool HasStringReference(size_t);

	DisplayMessage();
	Color GetColor(const GUIColors color) const;
	const std::string& GetColorName(GUIColors idx) const;
	/** displays any string in the textarea */
	void DisplayMarkupString(String txt) const;	
	/** displays a string constant in the textarea */
	void DisplayConstantString(size_t stridx, GUIColors color, Scriptable *speaker = nullptr) const;
	/** displays a string constant followed by a number in the textarea */
	void DisplayConstantStringValue(size_t stridx, GUIColors color, ieDword value) const;	
	/** displays actor name - action : parameter */
	void DisplayConstantStringName(size_t stridx, GUIColors color, const Scriptable *speaker) const;
	void DisplayConstantStringName(size_t stridx, const Color &color, const Scriptable *speaker) const;
	/** displays a string constant in the textarea, starting with actor, and ending with target */
	void DisplayConstantStringAction(size_t stridx, GUIColors color, const Scriptable *actor, const Scriptable *target) const;	
	/** displays a string in the textarea, starting with speaker's name */
	void DisplayStringName(String text, GUIColors color, const Scriptable *speaker) const;
	void DisplayStringName(ieStrRef stridx, GUIColors color, const Scriptable *speaker, STRING_FLAGS flags) const;
	void DisplayStringName(ieStrRef stridx, const Color &color, const Scriptable *speaker, STRING_FLAGS flags) const;
	/** displays a string constant in the textarea, starting with speaker's name, also replaces one numeric value (it is a format string) */
	void DisplayConstantStringNameValue(size_t stridx, GUIColors color, const Scriptable *speaker, int value) const;
	/** displays actor name - action : parameter */
	void DisplayConstantStringNameString(size_t stridx, GUIColors color, size_t stridx2, const Scriptable *actor) const;
	/** displays a string in the textarea */
	void DisplayString(const String& text) const;
	void DisplayString(ieStrRef stridx, GUIColors color, STRING_FLAGS flags) const;
	void DisplayString(const String& text, GUIColors color, Scriptable *target) const;
	void DisplayString(ieStrRef stridx, const Color &color, STRING_FLAGS flags) const;	
	/** iwd2 hidden roll debugger */
	template<typename ...ARGS>
	void DisplayRollStringName(ieStrRef stridx, GUIColors color, const Scriptable *speaker, ARGS&& ...args) const {
		if (EnableRollFeedback()) {
			String fmt = ResolveStringRef(stridx);
			String formatted = fmt::format(fmt, std::forward<ARGS>(args)...);
			DisplayStringName(std::move(formatted), color, speaker);
		}
	}
};

extern GEM_EXPORT DisplayMessage * displaymsg;

}

#endif
