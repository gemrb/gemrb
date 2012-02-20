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
#include "globals.h"

#include <vector>

namespace GemRB {

#define IE_DLG_TR_TEXT     0x01
#define IE_DLG_TR_TRIGGER  0x02
#define IE_DLG_TR_ACTION   0x04
#define IE_DLG_TR_FINAL    0x08
#define IE_DLG_TR_JOURNAL  0x10
#define IE_DLG_UNSOLVED    0x40
#define IE_DLG_SOLVED      0x100
#define IE_DLG_QUEST_GROUP 0x4000 // this is a GemRB extension

class Condition;
class Action;

struct DialogTransition {
	ieDword Flags;
	ieStrRef textStrRef;
	ieStrRef journalStrRef;
	Condition* condition;
	std::vector<Action*> actions;
	ieResRef Dialog;
	ieDword stateIndex;
};

struct DialogState {
	ieStrRef StrRef;
	DialogTransition** transitions;
	unsigned int transitionsCount;
	Condition* condition;
	unsigned int weight;
};

class GEM_EXPORT Dialog {
public:
	Dialog(void);
	~Dialog(void);
private:
	void FreeDialogState(DialogState* ds);
public:
	void AddState(DialogState* ds);
	DialogState* GetState(unsigned int index);
	int FindFirstState(Scriptable* target);
	int FindRandomState(Scriptable* target);

	void Release()
	{
		delete this;
	}
public:
	ieResRef ResRef;
	ieDword Flags; //freeze flags (bg2)
	unsigned int TopLevelCount;
	ieDword* Order;
	DialogState** initialStates;
};

}

#endif
