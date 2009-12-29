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
 * $Id$
 *
 */

#ifndef CLASSDESC_H
#define CLASSDESC_H

#include "../Core/Class_ID.h"
#include "../../includes/SClassID.h"
#include "../../includes/exports.h"

#define CLASS_ID_MASK     0x0fffffff
#define ALLOW_CONCURRENT  0x80000000

class GEM_EXPORT ClassDesc {
public:
	ClassDesc(void);
	virtual ~ClassDesc(void);
	virtual void* Create() = 0;
	virtual int BeginCreate();
	virtual int EndCreate();
	virtual const char* ClassName(void) = 0;
	virtual SClass_ID SuperClassID(void) = 0;
	virtual Class_ID ClassID(void) = 0;
	virtual SClass_ID SubClassID(void);
	virtual const char* InternalName(void);
};

#endif
