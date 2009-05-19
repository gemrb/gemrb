/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $Id$
 *
 */

#ifndef DLGIMP_H
#define DLGIMP_H

#include "../Core/DialogMgr.h"
#include "../../includes/globals.h"

/*
struct State {
	ieStrRef StrRef;
	ieDword  FirstTransitionIndex;
	ieDword  TransitionsCount;
	ieDword  TriggerIndex;
};
*/
/*
struct Transition {
	ieDword  Flags;
	ieStrRef AnswerStrRef;
	ieStrRef JournalStrRef;
	ieDword  TriggerIndex;
	ieDword  ActionIndex;
	ieResRef DLGResRef;
	ieDword  NextStateIndex;
};
*/
/*
struct VarOffset {
	ieDword Offset;
	ieDword Length;
};
*/
class DLGImp : public DialogMgr {
private:
	DataStream* str;
	bool autoFree;
	ieDword StatesCount;
	ieDword StatesOffset;
	ieDword TransitionsCount;
	ieDword TransitionsOffset;
	ieDword StateTriggersCount;
	ieDword StateTriggersOffset;
	ieDword TransitionTriggersCount;
	ieDword TransitionTriggersOffset;
	ieDword ActionsCount;
	ieDword ActionsOffset;
	ieDword Flags;
	ieDword Version;

public:
	DLGImp(void);
	~DLGImp(void);
	bool Open(DataStream* stream, bool autoFree = true);
	Dialog* GetDialog();
private:
	DialogState* GetDialogState(Dialog *d, unsigned int index);
	DialogTransition* GetTransition(unsigned int index);
	DialogString* GetStateTrigger(unsigned int index);
	DialogString* GetTransitionTrigger(unsigned int index);
	DialogString* GetAction(unsigned int index);
	char** GetStrings(char* string, unsigned int& count);
	DialogTransition** GetTransitions(unsigned int firstIndex,
		unsigned int count);
public:
	void release(void)
	{
		delete this;
	}
};

#endif
