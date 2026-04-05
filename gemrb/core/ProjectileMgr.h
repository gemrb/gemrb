// SPDX-FileCopyrightText: 2006 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef PROJECTILEMGR_H
#define PROJECTILEMGR_H

#include "Plugin.h"
#include "Projectile.h"

#include "Streams/DataStream.h"

namespace GemRB {

class GEM_EXPORT ProjectileMgr : public Plugin {
public:
	virtual bool Open(DataStream* stream) = 0;
	virtual Projectile* GetProjectile(Projectile*) = 0;
};

}

#endif
