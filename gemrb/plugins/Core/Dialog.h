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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Dialog.h,v 1.9 2004/04/06 17:55:17 avenger_teambg Exp $
 *
 */

#ifndef DIALOG_H
#define DIALOG_H

#include <vector>

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

#define IE_DLG_TR_TEXT     0x01
#define IE_DLG_TR_TRIGGER  0x02
#define IE_DLG_TR_ACTION   0x04
#define IE_DLG_TR_FINAL    0x08
#define IE_DLG_TR_JOURNAL  0x10
#define IE_DLG_UNSOLVED    0x40
#define IE_DLG_SOLVED      0x100
#define IE_DLG_QUEST_GROUP 0x4000 // this is a GemRB extension

typedef struct DialogString {
	char** strings;
	unsigned long count;
} DialogString;

typedef struct DialogTransition {
	unsigned long Flags;
	unsigned long textStrRef;
	unsigned long journalStrRef;
	DialogString* trigger;
	DialogString* action;
	char Dialog[9];
	unsigned long stateIndex;
} DialogTransition;

typedef struct DialogState {
	unsigned long StrRef;
	DialogTransition** transitions;
	unsigned long transitionsCount;
	DialogString* trigger;
} DialogState;

class GEM_EXPORT Dialog {
public:
	Dialog(void);
	~Dialog(void);
private:
	std::vector< DialogState*> initialStates;
public:
	void AddState(DialogState* ds);
	DialogState* GetState(int index);
	char ResRef[9];
private:
	void FreeDialogState(DialogState* ds);
	void FreeDialogString(DialogString* ds);
public:
	int StateCount()
	{
		return initialStates.size();
	}
	void Release()
	{
		delete this;
	}
};

#endif
