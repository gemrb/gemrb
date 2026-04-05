// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Factory.h"

namespace GemRB {

void Factory::AddFactoryObject(object_t fobject)
{
	fobjects.push_back(std::move(fobject));
}

int Factory::IsLoaded(const ResRef& resref, SClass_ID type) const
{
	if (resref.IsEmpty()) {
		return -1;
	}

	for (unsigned int i = 0; i < fobjects.size(); i++) {
		if (fobjects[i]->SuperClassID == type) {
			if (fobjects[i]->resRef == resref) {
				return i;
			}
		}
	}
	return -1;
}

Factory::object_t Factory::GetFactoryObject(int pos) const
{
	return fobjects[pos];
}

}
