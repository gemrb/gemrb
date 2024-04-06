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
#include "Game.h"

#include "Scriptable/Scriptable.h"

namespace GemRB {

class Control;

class GEM_EXPORT DialogHandler {
public:
	DialogHandler();
	DialogHandler(const DialogHandler&) = delete;
	~DialogHandler();
	DialogHandler& operator=(const DialogHandler&) = delete;

	Scriptable *GetTarget() const;
	Actor *GetSpeaker();
	bool InDialog(const Scriptable *scr) const { return IsSpeaker(scr) || IsTarget(scr); }
	bool IsSpeaker(const Scriptable *scr) const { return scr->GetGlobalID() == speakerID; }
	bool IsTarget(const Scriptable *scr) const { return scr->GetGlobalID() == targetID; }
	void SetSpeaker(const Scriptable *scr) { speakerID = scr->GetGlobalID(); }
	void SetTarget(const Scriptable *scr) { targetID = scr->GetGlobalID(); }

	bool InitDialog(Scriptable* speaker, Scriptable* target, const ResRef& dlgref, ieDword si = -1);
	void EndDialog(bool try_to_break=false);
	bool DialogChoose(unsigned int choose);

private:
	/** this function safely retrieves an Actor by ID */
	Actor *GetLocalActorByGlobalID(ieDword ID);
	void UpdateJournalForTransition(const DialogTransition* tr) const;
	void DialogChooseInitial(Scriptable* target, Actor* tgta) const;
	int DialogChooseTransition(unsigned int choose, Scriptable*& target, Actor*& tgta, Actor* speaker);

	DialogState* ds = nullptr;
	Dialog* dlg = nullptr;

	ieDword speakerID = 0;
	ieDword targetID = 0;
	ieDword originalTargetID = 0;

	int initialState = -1;
	Point prevViewPortLoc;

	std::array<JournalSection, 4> sectionMap {};
};

}

#endif
