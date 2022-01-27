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

namespace GemRB {

static constexpr Color DMC_WHITE {0xf0, 0xf0, 0xf0, 0xff};
static constexpr Color DMC_RED = ColorRed;
static constexpr Color DMC_LIGHTGREY {0xd7, 0xd7, 0xbe, 0xff};
static constexpr Color DMC_BG2XPGREEN {0xbc, 0xef, 0xbc, 0xff};
static constexpr Color DMC_GOLD {0xc0, 0xc0, 0x00, 0xff};
static constexpr Color DMC_DIALOG = DMC_WHITE;
static constexpr Color DMC_DIALOGPARTY = {0x80, 0x80, 0xff, 0xff};

class Scriptable;

class GEM_EXPORT DisplayMessage
{
private:
	struct StrRefs {
		std::string loadedTable;
		ieStrRef table[STRREF_COUNT];

		StrRefs();
		bool LoadTable(const std::string& name);
		ieStrRef operator[](size_t) const;
	};
	static StrRefs SRefs;
	static void LoadStringRefs();
	
	static bool EnableRollFeedback();
	static String ResolveStringRef(ieStrRef);
	
	template<typename ...ARGS>
	void DisplayRollStringName(const String& fmt, const Color &color, const Scriptable *speaker, ARGS&& ...args) const {
		String formatted = fmt::format(fmt, std::forward<ARGS>(args)...);
		DisplayStringName(std::move(formatted), color, speaker);
	}
public:
	static ieStrRef GetStringReference(size_t);
	static bool HasStringReference(size_t);

public:
	DisplayMessage();
	/** returns the speaker's color and name */
	Color GetSpeakerColor(String& name, const Scriptable *&speaker) const;
	/** displays any string in the textarea */
	void DisplayMarkupString(String txt) const;
	/** displays a string constant in the textarea */
	void DisplayConstantString(size_t stridx, const Color &color, Scriptable *speaker=NULL) const;
	/** displays actor name - action : parameter */
	void DisplayConstantStringNameString(size_t stridx, const Color &color, size_t stridx2, const Scriptable *actor) const;
	/** displays a string constant followed by a number in the textarea */
	void DisplayConstantStringValue(size_t stridx, const Color &color, ieDword value) const;
	/** displays a string constant in the textarea, starting with speaker's name */
	void DisplayConstantStringName(size_t stridx, const Color &color, const Scriptable *speaker) const;
	/** displays a string constant in the textarea, starting with speaker's name, also replaces one numeric value (it is a format string) */
	void DisplayConstantStringNameValue(size_t stridx, const Color &color, const Scriptable *speaker, int value) const;
	/** displays a string constant in the textarea, starting with actor, and ending with target */
	void DisplayConstantStringAction(size_t stridx, const Color &color, const Scriptable *actor, const Scriptable *target) const;
	/** displays a string in the textarea */
	void DisplayString(const String& text) const;
	void DisplayString(ieStrRef stridx, const Color &color, STRING_FLAGS flags) const;
	void DisplayString(String text, const Color &color, Scriptable *target) const;
	/** displays a string in the textarea, starting with speaker's name */
	void DisplayStringName(ieStrRef stridx, const Color &color, const Scriptable *speaker, STRING_FLAGS flags) const;
	void DisplayStringName(String text, const Color &color, const Scriptable *speaker) const;
	/** iwd2 hidden roll debugger */
	template<typename ...ARGS>
	void DisplayRollStringName(ieStrRef stridx, const Color &color, const Scriptable *speaker, ARGS&& ...args) const {
		if (EnableRollFeedback()) {
			String fmt = ResolveStringRef(stridx);
			DisplayRollStringName(fmt, color, speaker, std::forward<ARGS>(args)...);
		}
	}
};

extern GEM_EXPORT DisplayMessage * displaymsg;

}

#endif
