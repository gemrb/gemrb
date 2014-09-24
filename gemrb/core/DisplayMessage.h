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

#include <cstdlib>

namespace GemRB {

#define DMC_WHITE 0xf0f0f0
#define DMC_RED 0xff0000
#define DMC_LIGHTGREY 0xd7d7be
#define DMC_BG2XPGREEN 0xbcefbc
#define DMC_GOLD 0xc0c000
#define DMC_DIALOG DMC_WHITE
#define DMC_DIALOGPARTY 0x8080FF

class Scriptable;

class GEM_EXPORT DisplayMessage
{
private:
	bool ReadStrrefs();

public:
	DisplayMessage(void);

	/** returns a string reference from a string reference index constant */
	ieStrRef GetStringReference(int stridx) const;
	/** returns true if a string reference for a string reference index constant exists */
	bool HasStringReference(int stridx) const;
	/** returns the speaker's color and name */
	unsigned int GetSpeakerColor(char *&name, const Scriptable *&speaker) const;
	/** displays any string in the textarea */
	void DisplayString(const char *txt, Scriptable *speaker=NULL) const;
	/** displays a string constant in the textarea */
	void DisplayConstantString(int stridx, unsigned int color, Scriptable *speaker=NULL) const;
	/** displays actor name - action : parameter */
	void DisplayConstantStringNameString(int stridx, unsigned int color, int stridx2, const Scriptable *actor) const;
	/** displays a string constant followed by a number in the textarea */
	void DisplayConstantStringValue(int stridx, unsigned int color, ieDword value) const;
	/** displays a string constant in the textarea, starting with speaker's name */
	void DisplayConstantStringName(int stridx, unsigned int color, const Scriptable *speaker) const;
	/** displays a string constant in the textarea, starting with speaker's name, also replaces one numeric value (it is a format string) */
	void DisplayConstantStringNameValue(int stridx, unsigned int color, const Scriptable *speaker, int value) const;
	/** displays a string constant in the textarea, starting with actor, and ending with target */
	void DisplayConstantStringAction(int stridx, unsigned int color, const Scriptable *actor, const Scriptable *target) const;
	/** displays a string in the textarea */
	void DisplayString(int stridx, unsigned int color, ieDword flags) const;
	void DisplayString(const char *text, unsigned int color, Scriptable *target) const;
	/** displays a string in the textarea, starting with speaker's name */
	void DisplayStringName(int stridx, unsigned int color, const Scriptable *speaker, ieDword flags) const;
	void DisplayStringName(const char *text, unsigned int color, const Scriptable *speaker) const;
	/** iwd2 hidden roll debugger */
	void DisplayRollStringName(int stridx, unsigned int color, const Scriptable *speaker, ...) const;
};

extern GEM_EXPORT DisplayMessage * displaymsg;

}

#endif
