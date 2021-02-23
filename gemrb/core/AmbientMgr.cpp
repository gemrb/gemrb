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

void AmbientMgr::reset()
{
	std::lock_guard<std::mutex> l(ambientsMutex);
	ambients.clear();
	ambientsSet(ambients);
}

void AmbientMgr::setAmbients(const std::vector<Ambient *> &a)
{
	std::lock_guard<std::mutex> l(ambientsMutex);
	ambients = a;
	ambientsSet(ambients);
	activate();
}
 
void AmbientMgr::activate(const std::string &name)
{
	std::lock_guard<std::mutex> l(ambientsMutex);
	for (auto ambient : ambients) {
		if (ambient->getName() == name) {
			ambient->setActive();
			break;
		}
	}
}

void AmbientMgr::deactivate(const std::string &name)
{
	std::lock_guard<std::mutex> l(ambientsMutex);
	for (auto ambient : ambients) {
		if (ambient->getName() == name) {
			ambient->setInactive();
			break;
		}
	}
}

bool AmbientMgr::isActive(const std::string &name) const
{
	std::lock_guard<std::mutex> l(ambientsMutex);
	for (auto ambient : ambients) {
		if (ambient->getName() == name) {
			return ambient->getFlags() & IE_AMBI_ENABLED;
		}
	}
	return false;
}

}
