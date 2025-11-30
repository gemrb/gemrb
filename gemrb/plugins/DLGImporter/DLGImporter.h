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
 *
 */

#ifndef DLGIMPORTER_H
#define DLGIMPORTER_H

#include "DialogMgr.h"

namespace GemRB {

class DLGImporter : public DialogMgr {
private:
	ieDword StatesCount = 0;
	ieDword StatesOffset = 0;
	ieDword TransitionsCount = 0;
	ieDword TransitionsOffset = 0;
	ieDword StateTriggersCount = 0;
	ieDword StateTriggersOffset = 0;
	ieDword TransitionTriggersCount = 0;
	ieDword TransitionTriggersOffset = 0;
	ieDword ActionsCount = 0;
	ieDword ActionsOffset = 0;
	ieDword Flags = 0;
	ieDword Version = 0;

public:
	DLGImporter() noexcept = default;

	Holder<Dialog> GetDialog() const override;
	Holder<Condition> GetCondition(const char* string) const override;

private:
	bool Import(DataStream* stream) override;
	Holder<DialogState> GetDialogState(Dialog& dlg, unsigned int index) const;
	Holder<DialogTransition> GetTransition(unsigned int index) const;
	Holder<Condition> GetStateTrigger(unsigned int index) const;
	Holder<Condition> GetTransitionTrigger(unsigned int index) const;
	std::vector<Action*> GetAction(unsigned int index) const;
	std::vector<Holder<DialogTransition>> GetTransitions(unsigned int firstIndex,
							     unsigned int count) const;
};

}

#endif
