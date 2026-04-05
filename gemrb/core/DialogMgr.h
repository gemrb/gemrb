// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef DIALOGMGR_H
#define DIALOGMGR_H

#include "Dialog.h"
#include "Holder.h"
#include "Plugin.h"

namespace GemRB {

class GEM_EXPORT DialogMgr : public ImporterBase {
public:
	virtual Holder<Dialog> GetDialog() const = 0;
	virtual Holder<Condition> GetCondition(const char* string) const = 0;
};

}

#endif
