// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef DIALOG_H
#define DIALOG_H

#include "exports.h"
#include "ie_types.h"

#include "Holder.h"

#include <vector>

namespace GemRB {

#define IE_DLG_TR_TEXT    0x01
#define IE_DLG_TR_TRIGGER 0x02
#define IE_DLG_TR_ACTION  0x04
#define IE_DLG_TR_FINAL   0x08
#define IE_DLG_TR_JOURNAL 0x10
// unknown 0x20 ("Interrupt is an interjection"; only [so far] visible result is duplication of responses)
#define IE_DLG_UNSOLVED   0x40
#define IE_DLG_ADDJOURNAL 0x80 // Add Journal note (works implicitly — bg2Sections[0] is the default)
#define IE_DLG_SOLVED     0x100
#define IE_DLG_IMMEDIATE  0x200 // 1=Immediate execution of script actions, 0=Delayed execution of script actions (BGEE)
// TODO: implement EE extensions
// bit 10: Clear actions (BGEE)

#define IE_DLG_QUEST_GROUP 0x4000 // this is a GemRB extension

class Action;
class Condition;
class Scriptable;

struct DialogTransition {
	ieDword Flags;
	ieStrRef textStrRef;
	ieStrRef journalStrRef;
	Holder<Condition> condition;
	std::vector<Action*> actions;
	ResRef Dialog;
	ieDword stateIndex;
};

struct DialogState {
	ieStrRef StrRef;
	std::vector<Holder<DialogTransition>> transitions;
	unsigned int transitionsCount;
	Holder<Condition> condition;
	unsigned int weight;
};

class GEM_EXPORT Dialog {
public:
	Dialog() noexcept = default;
	Dialog(const Dialog&) = delete;
	Dialog(Dialog&&) noexcept = default;
	~Dialog();
	Dialog& operator=(const Dialog&) = delete;

private:
	void FreeDialogState(Holder<DialogState> ds) const;

public:
	Holder<DialogState> GetState(unsigned int index) const;
	int FindFirstState(Scriptable* target) const;
	int FindRandomState(Scriptable* target) const;

public:
	ResRef resRef;
	ieDword Flags = 0; // freeze flags (bg2)
	unsigned int TopLevelCount = 0;
	std::vector<unsigned int> Order;
	std::vector<Holder<DialogState>> initialStates;
};

}

#endif
