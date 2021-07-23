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

#include "System/DataStream.h"

#include <stdexcept>

namespace GemRB {

/**
 * @class Plugin
 * Base class for all GemRB plugins
 */

class GEM_EXPORT Plugin : public Held<Plugin>
{
public:
	// declaring these here so we dont have to GEM_EXPORT every Held<T> for MSVC to be happy
	Plugin() = default;
	~Plugin() = default;
};

template <class IMPORTER>
class GEM_EXPORT ImporterPlugin final : public Plugin
{
	IMPORTER* importer = new IMPORTER;
public:
	~ImporterPlugin() {
		delete importer;
	}

	IMPORTER* GetImporter(DataStream* str) {
		if (str == nullptr) {
			return nullptr;
		}

		struct StreamHandle {
			DataStream* str;
			StreamHandle(DataStream* str) noexcept
			: str(str) {}
			
			~StreamHandle() noexcept {
				delete str;
			}
		} handle(str);

		if (importer->Open(str) == false) {
			return nullptr;
		}
		return importer;
	}
};

}

#endif
