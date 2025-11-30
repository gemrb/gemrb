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
	void FreeDialogState(Holder<DialogState> ds);

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
