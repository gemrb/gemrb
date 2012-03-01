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

#include "win32def.h"

#include <cstring>

namespace GemRB {

Factory::Factory(void)
{
}

Factory::~Factory(void)
{
	for (unsigned int i = 0; i < fobjects.size(); i++) {
		delete( fobjects[i] );
	}
}

void Factory::AddFactoryObject(FactoryObject* fobject)
{
	fobjects.push_back( fobject );
}

int Factory::IsLoaded(const char* ResRef, SClass_ID type) const
{
	for (unsigned int i = 0; i < fobjects.size(); i++) {
		if (fobjects[i]->SuperClassID == type) {
			if (strnicmp( fobjects[i]->ResRef, ResRef, 8 ) == 0) {
				return i;
			}
		}
	}
	return -1;
}

FactoryObject* Factory::GetFactoryObject(int pos) const
{
	return fobjects[pos];
}

void Factory::FreeObjects(void)
{
	for (unsigned int i = 0; i < fobjects.size(); i++) {
		delete( fobjects[i] );
	}
}

}
