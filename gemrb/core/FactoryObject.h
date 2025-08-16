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

#ifndef FACTORYOBJECT_H
#define FACTORYOBJECT_H

#include "SClassID.h"
#include "exports-core.h"
#include "globals.h"

#include "Resource.h"

namespace GemRB {

class GEM_EXPORT FactoryObject {
public:
	SClass_ID SuperClassID;
	ResRef resRef;
	FactoryObject(const ResRef& name, SClass_ID superClassID)
		: SuperClassID(superClassID), resRef(name) {};
	virtual ~FactoryObject() noexcept = default;
};

}

#endif
