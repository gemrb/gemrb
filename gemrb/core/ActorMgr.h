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

#ifndef ACTORMGR_H
#define ACTORMGR_H

#include "Plugin.h"

namespace GemRB {

class Actor;
class DataStream;

class GEM_EXPORT ActorMgr : public Plugin {
public:
	ActorMgr(void);
	virtual ~ActorMgr(void);
	virtual bool Open(DataStream* stream) = 0;
	virtual Actor* GetActor(unsigned char is_in_party) = 0;
	virtual int FindSpellType(char *name, unsigned short &level, unsigned int clsmsk, unsigned int kit) = 0;

	//returns saved size, updates internal offsets before save
	virtual int GetStoredFileSize(Actor *ac) = 0;
	//saves file
	virtual int PutActor(DataStream *stream, Actor *actor, bool chr=false) = 0;
};

}

#endif
