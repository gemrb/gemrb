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

/**
 * @file SpellMgr.h
 * Declares SpellMgr class, abstract loader for Spell objects
 * @author The GemRB Project
 */

#ifndef SPELLMGR_H
#define SPELLMGR_H

#include "Plugin.h"
#include "DataStream.h"
#include "Spell.h"

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

/**
 * @class SpellMgr
 * Abstract loader for Spell objects
 */

class GEM_EXPORT SpellMgr : public Plugin {
public:
	SpellMgr(void);
	virtual ~SpellMgr(void);
	virtual bool Open(DataStream* stream, bool autoFree = true) = 0;
	virtual Spell* GetSpell(Spell *spl) = 0;
};

#endif
