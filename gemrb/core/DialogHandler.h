/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#ifndef DIALOGHANDLER_H
#define DIALOGHANDLER_H

#include "exports.h"

#include "Dialog.h"
#include "Scriptable/Scriptable.h"

namespace GemRB {

class Control;

class GEM_EXPORT DialogHandler {
public:
	DialogHandler();
	~DialogHandler();

	Scriptable *GetTarget();
	Actor *GetSpeaker();
	bool InDialog(const Scriptable *scr) const { return IsSpeaker(scr) || IsTarget(scr); }
	bool IsSpeaker(const Scriptable *scr) const { return scr->GetGlobalID() == speakerID; }
	bool IsTarget(const Scriptable *scr) const { return scr->GetGlobalID() == targetID; }
	void SetSpeaker(const Scriptable *scr) { speakerID = scr->GetGlobalID(); }
	void SetTarget(const Scriptable *scr) { targetID = scr->GetGlobalID(); }

	bool InitDialog(Scriptable* speaker, Scriptable* target, const char* dlgref);
	void EndDialog(bool try_to_break=false);
	bool DialogChoose(unsigned int choose);
	bool DialogChoose(Control *ctrl);

private:
	/** this function safely retrieves an Actor by ID */
	Actor *GetActorByGlobalID(ieDword ID);
	void UpdateJournalForTransition(DialogTransition *tr);

	DialogState* ds;
	Dialog* dlg;

	ieDword speakerID;
	ieDword targetID;
	ieDword originalTargetID;

	int initialState;
	int previousX, previousY;
};

}

#endif
