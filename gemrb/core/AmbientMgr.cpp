/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2004 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#include "AmbientMgr.h"

#include "Ambient.h"

namespace GemRB {

AmbientMgr::AmbientMgr()
{
}

AmbientMgr::~AmbientMgr()
{
	reset();
}
 
void AmbientMgr::activate(const std::string &name)
{
	for (auto ambient : ambients) {
		if (ambient->getName() == name) {
			ambient->setActive();
			break;
		}
	}
}

void AmbientMgr::deactivate(const std::string &name)
{
	for (auto ambient : ambients) {
		if (ambient->getName() == name) {
			ambient->setInactive();
			break;
		}
	}
}

bool AmbientMgr::isActive(const std::string &name) const
{
	for (auto ambient : ambients) {
		if (ambient->getName() == name) {
			return ambient->getFlags() & IE_AMBI_ENABLED;
		}
	}
	return false;
}

}
