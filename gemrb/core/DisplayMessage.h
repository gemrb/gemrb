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
#include "ie_types.h"
#include "strrefs.h"
#include "System/String.h"
#include "RGBAColor.h"

#include <string>

namespace GemRB {

enum { DMC_DIALOG, DMC_DIALOGPARTY, DMC_GOLD, DMC_RED, DMC_BG2XPGREEN, DMC_LIGHTGREY, DMC_WHITE };

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
	/** displays a string constant in the textarea */
	void DisplayConstantString(int stridx, const Color &color, Scriptable *speaker=NULL) const;
	/** displays actor name - action : parameter */
	void DisplayConstantStringNameString(int stridx, const Color &color, int stridx2, const Scriptable *actor) const;
	/** displays a string constant followed by a number in the textarea */
	void DisplayConstantStringValue(int stridx, const Color &color, ieDword value) const;
	/** displays a string constant in the textarea, starting with speaker's name */
	void DisplayConstantStringName(int stridx, const Color &color, const Scriptable *speaker) const;
	/** displays a string constant in the textarea, starting with speaker's name, also replaces one numeric value (it is a format string) */
	void DisplayConstantStringNameValue(int stridx, const Color &color, const Scriptable *speaker, int value) const;
	/** displays a string constant in the textarea, starting with actor, and ending with target */
	void DisplayConstantStringAction(int stridx, const Color &color, const Scriptable *actor, const Scriptable *target) const;
	/** displays a string in the textarea, starting with speaker's name */
	void DisplayStringName(int stridx, const Color &color, const Scriptable *speaker, ieDword flags) const;
	void DisplayStringName(const String& text, const Color &color, const Scriptable *speaker) const;
	/** iwd2 hidden roll debugger */
	void DisplayRollStringName(int stridx, const Color &color, const Scriptable *speaker, ...) const;	
public:
	static ieStrRef GetStringReference(size_t);
	static bool HasStringReference(size_t);

public:
	DisplayMessage();
	/** returns the speaker's color and name */
	Color GetSpeakerColor(String& name, const Scriptable *&speaker) const;
	/** displays any string in the textarea */
	void DisplayMarkupString(const String& txt) const;
	/** displays a string constant in the textarea */
	void DisplayConstantString(int stridx, unsigned char color, Scriptable *speaker=NULL) const;
	/** displays a string constant followed by a number in the textarea that takes color as an enum */
	void DisplayConstantStringValue(int stridx,  unsigned char color, ieDword value) const;	
	/** displays a string constant in the textarea, starting with speaker's name */
	void DisplayConstantStringName(int stridx, unsigned char color, const Scriptable *speaker) const;
	/** displays a string constant in the textarea, starting with actor, and ending with target */
	void DisplayConstantStringAction(int stridx, unsigned char color, const Scriptable *actor, const Scriptable *target) const;
	/** displays a string in the textarea */
	void DisplayString(const String& text) const;
	void DisplayString(int stridx, const Color &color, ieDword flags) const;
	void DisplayString(const String& text, const Color &color, Scriptable *target) const;
	void DisplayString(int stridx, unsigned char color, ieDword flags) const;
	void DisplayString(const String& text, unsigned char color, Scriptable *target) const;
	/** displays a string in the textarea, starting with speaker's name */
	void DisplayStringName(const String& text, unsigned char color, const Scriptable *speaker) const;
	void DisplayStringName(int stridx, unsigned char color, const Scriptable *speaker, ieDword flags) const;
	/** iwd2 hidden roll debugger */
	void DisplayRollStringName(int stridx, unsigned char color, const Scriptable *speaker, ...) const;
};

extern GEM_EXPORT DisplayMessage * displaymsg;

}

#endif
