// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef ACTORMGR_H
#define ACTORMGR_H

#include "ie_types.h"

#include "Plugin.h"

namespace GemRB {

class Actor;

class GEM_EXPORT ActorMgr : public ImporterBase {
public:
	virtual Actor* GetActor(unsigned char is_in_party) = 0;
	virtual ieWord FindSpellType(const ResRef& name, unsigned short& level, unsigned int clsMask, unsigned int kit) const = 0;

	//returns saved size, updates internal offsets before save
	virtual int GetStoredFileSize(const Actor* ac) = 0;
	//saves file
	virtual int PutActor(DataStream* stream, const Actor* actor, bool chr = false) = 0;
};

}

#endif
