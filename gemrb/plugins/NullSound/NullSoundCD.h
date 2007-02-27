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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Id$
 *
 */

#ifndef NULLSOUNDCD_H
#define NULLSOUNDCD_H

#include "../Core/ClassDesc.h"

class NullSoundCD : public ClassDesc {
public:
	NullSoundCD(void);
	~NullSoundCD(void);
	void* Create(void);
	const char* ClassName(void);
	SClass_ID SuperClassID(void);
	Class_ID ClassID(void);
	const char* InternalName(void);
};

static NullSoundCD NullsoundCD;

#endif
