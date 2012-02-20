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

/**
 * @file Plugin.h
 * Declares Plugin class, base class for all plugins
 * @author The GemRB Project
 */

#ifndef PLUGIN_H
#define PLUGIN_H

#include "SClassID.h" // FIXME
#include "exports.h"

#include "Holder.h"
#include "TypeID.h"

#include <cstddef>

namespace GemRB {

/**
 * @class Plugin
 * Base class for all GemRB plugins
 */

class GEM_EXPORT Plugin : public Held<Plugin> {
public:
	Plugin(void);
	virtual ~Plugin(void);
};

}

#endif
