// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

/**
 * @file SpellMgr.h
 * Declares SpellMgr class, abstract loader for Spell objects
 * @author The GemRB Project
 */

#ifndef SPELLMGR_H
#define SPELLMGR_H

#include "Plugin.h"
#include "Spell.h"

#include "Streams/DataStream.h"

namespace GemRB {

/**
 * @class SpellMgr
 * Abstract loader for Spell objects
 */

class GEM_EXPORT SpellMgr : public Plugin {
public:
	virtual bool Open(DataStream* stream) = 0;
	virtual Spell* GetSpell(Spell* spl, bool silent = false) = 0;
};

}

#endif
