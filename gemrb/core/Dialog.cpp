// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Dialog.h"

#include "RNG.h"

#include "GameScript/GameScript.h"
#include "Scriptable/Scriptable.h"

namespace GemRB {

enum BlitFlags : uint32_t;

Dialog::~Dialog(void)
{
	for (const auto& state : initialStates) {
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

void Dialog::FreeDialogState(Holder<DialogState> ds) const
{
	for (auto& trans : ds->transitions) {
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
