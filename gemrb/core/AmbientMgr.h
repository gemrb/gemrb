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

#ifndef AMBIENTMGR_H
#define AMBIENTMGR_H

#include "exports.h"
#include "win32def.h"

#include <string>
#include <vector>

namespace GemRB {

class Ambient;

class GEM_EXPORT AmbientMgr {
public:
	AmbientMgr();
	virtual ~AmbientMgr();
	virtual void reset() { ambients = std::vector<Ambient *> (); }
	virtual void setAmbients(const std::vector<Ambient *> &a) { reset(); ambients = a; activate(); }
	virtual void activate(const std::string &name);
	virtual void activate() { active = true; } // hard play ;-)
	virtual void deactivate(const std::string &name);
	virtual void deactivate() { active = false; } // hard stop
	virtual bool isActive(const std::string &name) const;
protected:
	std::vector<Ambient *> ambients;
	bool active;
};

}

#endif
