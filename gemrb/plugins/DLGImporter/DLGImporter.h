// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
