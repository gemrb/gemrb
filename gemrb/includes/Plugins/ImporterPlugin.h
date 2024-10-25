/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2022 The GemRB Project
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

#ifndef H_IMPORTER_PLUGIN
#define H_IMPORTER_PLUGIN

#include "Plugin.h"

#include "Plugins/export.h"

namespace GemRB {

template<class IMPORTER>
class GEM_PLUGIN_EXPORT ImporterPlugin final : public Plugin {
	std::shared_ptr<IMPORTER> importer = MakeImporter<IMPORTER>();

public:
	std::shared_ptr<IMPORTER> GetImporter() const noexcept
	{
		return importer;
	}

	std::shared_ptr<IMPORTER> GetImporter(DataStream* str) noexcept
	{
		if (str == nullptr) {
			return nullptr;
		}

		if (importer->Open(str) == false) {
			return nullptr;
		}

		return importer;
	}
};

}

#endif
