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
