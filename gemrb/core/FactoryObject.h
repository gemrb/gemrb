// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef FACTORYOBJECT_H
#define FACTORYOBJECT_H

#include "SClassID.h"
#include "exports.h"
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
