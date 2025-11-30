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

#include "RNG.h"

#include "GameScript/GameScript.h"
#include "Scriptable/Scriptable.h"

namespace GemRB {

enum BlitFlags : uint32_t;

Dialog::~Dialog(void)
{
	for (auto& state : initialStates) {
		if (state) {
			FreeDialogState(state);
		}
	}
}

Holder<DialogState> Dialog::GetState(unsigned int index) const
{
	if (index >= TopLevelCount) {
		return nullptr;
	}
	return initialStates[index];
}

void Dialog::FreeDialogState(Holder<DialogState> ds)
{
	for (unsigned int i = 0; i < ds->transitionsCount; i++) {
		Holder<DialogTransition> trans = ds->transitions[i];
		for (auto& action : trans->actions) {
			action->Release();
		}
		trans = nullptr;
	}
}

int Dialog::FindFirstState(Scriptable* target) const
{
	for (unsigned int i = 0; i < TopLevelCount; i++) {
		const Holder<Condition> cond = GetState(Order[i])->condition;
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
	unsigned int pick = RAND(0u, max - 1);
	for (unsigned int i = pick; i < max; i++) {
		const Holder<Condition> cond = GetState(i)->condition;
		if (cond && cond->Evaluate(target)) {
			return i;
		}
	}
	for (unsigned int i = 0; i < pick; i++) {
		const Holder<Condition> cond = GetState(i)->condition;
		if (cond && cond->Evaluate(target)) {
			return i;
		}
	}
	return -1;
}

}
