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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/DLGImporter/DLGImp.h,v 1.6 2004/08/02 21:26:54 guidoj Exp $
 *
 */

#ifndef DLGIMP_H
#define DLGIMP_H

#include "../Core/DialogMgr.h"
#include "../../includes/globals.h"

typedef struct State {
	ieStrRef StrRef;
	ieDword  FirstTransitionIndex;
	ieDword  TransitionsCount;
	ieDword  TriggerIndex;
} State;

typedef struct Transition {
	ieDword  Flags;
	ieStrRef AnswerStrRef;
	ieStrRef JournalStrRef;
	ieDword  TriggerIndex;
	ieDword  ActionIndex;
	ieResRef DLGResRef;
	ieDword  NextStateIndex;
} Transition;

typedef struct VarOffset {
	unsigned long Offset;
	unsigned long Length;
} VarOffset;

class DLGImp : public DialogMgr {
private:
	DataStream* str;
	bool autoFree;
	unsigned long StatesCount;
	unsigned long StatesOffset;
	unsigned long TransitionsCount;
	unsigned long TransitionsOffset;
	unsigned long StateTriggersCount;
	unsigned long StateTriggersOffset;
	unsigned long TransitionTriggersCount;
	unsigned long TransitionTriggersOffset;
	unsigned long ActionsCount;
	unsigned long ActionsOffset;
	unsigned long Version;

public:
	DLGImp(void);
	~DLGImp(void);
	bool Open(DataStream* stream, bool autoFree = true);
	Dialog* GetDialog();
	DialogState* GetDialogState(unsigned int index);
private:
	DialogTransition* GetTransition(unsigned int index);
	DialogString* GetStateTrigger(unsigned int index);
	DialogString* GetTransitionTrigger(unsigned int index);
	DialogString* GetAction(unsigned int index);
	char** GetStrings(char* string, unsigned long& count);
	DialogTransition** GetTransitions(unsigned long firstIndex,
		unsigned long count);
public:
	void release(void)
	{
		delete this;
	}
};

#endif
