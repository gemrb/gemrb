// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef DIALOGHANDLER_H
#define DIALOGHANDLER_H

#include "exports.h"

#include "Game.h"
#include "Holder.h"

#include "Scriptable/Scriptable.h"

namespace GemRB {

class Dialog;
struct DialogState;
struct DialogTransition;

class GEM_EXPORT DialogHandler {
public:
	DialogHandler();
	DialogHandler(const DialogHandler&) = delete;
	DialogHandler& operator=(const DialogHandler&) = delete;

	Scriptable* GetTarget() const;
	Actor* GetSpeaker() const;
	bool InDialog(const Scriptable* scr) const { return IsSpeaker(scr) || IsTarget(scr); }
	bool IsSpeaker(const Scriptable* scr) const;
	bool IsTarget(const Scriptable* scr) const;
	ScriptID SetSpeaker(const Scriptable* scr);
	ScriptID SetTarget(const Scriptable* scr);

	bool InitDialog(Scriptable* speaker, Scriptable* target, const ResRef& dlgref, ieDword si = -1);
	void EndDialog(bool try_to_break = false);
	bool DialogChoose(unsigned int choose);

private:
	/** this function safely retrieves an Actor by ID */
	Actor* GetLocalActorByGlobalID(ieDword ID) const;
	void UpdateJournalForTransition(const Holder<DialogTransition> tr) const;
	void DialogChooseInitial(Scriptable* target, Actor* tgta) const;
	int DialogChooseTransition(unsigned int choose, Scriptable*& target, Actor*& tgta, Actor* speaker);

	Holder<DialogState> ds = nullptr;
	Holder<Dialog> dlg = nullptr;

	ScriptID speakerID = 0;
	ScriptID targetID = 0;
	ScriptID originalTargetID = 0;

	int initialState = -1;
	Point prevViewPortLoc;

	std::array<JournalSection, 4> sectionMap {};
};

}

#endif
