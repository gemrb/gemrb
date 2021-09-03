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

#include "Dialog.h"

#include "GameScript/GameScript.h"
#include "RNG.h"

namespace GemRB {

Dialog::Dialog(void)
{
	TopLevelCount = 0;
	Flags = 0;
	Order = NULL;
	initialStates = NULL;
}

Dialog::~Dialog(void)
{
	if (initialStates) {
		for (unsigned int i = 0; i < TopLevelCount; i++) {
			if (initialStates[i]) {
				FreeDialogState( initialStates[i] );
			}
		}
		free(initialStates);
	}
	if (Order) free(Order);
}

DialogState* Dialog::GetState(unsigned int index) const
{
	if (index >= TopLevelCount) {
		return NULL;
	}
	return initialStates[index];
}

void Dialog::FreeDialogState(DialogState* ds)
{
	for (unsigned int i = 0; i < ds->transitionsCount; i++) {
		DialogTransition *trans = ds->transitions[i];
		for (auto& action : trans->actions) {
			action->Release();
		}
		if (trans->condition)
			delete trans->condition;
		delete trans;
	}
	free( ds->transitions );
	if (ds->condition) {
		delete ds->condition;
	}
	delete ds;
}

int Dialog::FindFirstState(Scriptable* target) const
{
	for (unsigned int i = 0; i < TopLevelCount; i++) {
		const Condition *cond = GetState(Order[i])->condition;
		if (cond && cond->Evaluate(target)) {
			return Order[i];
		}
	}
	return -1;
}

int Dialog::FindRandomState(Scriptable* target) const
{
	unsigned int max = TopLevelCount;
	if (!max) return -1;
	unsigned int pick = RAND(0u, max-1);
	for (unsigned int i = pick; i < max; i++) {
		const Condition *cond = GetState(i)->condition;
		if (cond && cond->Evaluate(target)) {
			return i;
		}
	}
	for (unsigned int i = 0; i < pick; i++) {
		const Condition *cond = GetState(i)->condition;
		if (cond && cond->Evaluate(target)) {
			return i;
		}
	}
	return -1;
}

}
