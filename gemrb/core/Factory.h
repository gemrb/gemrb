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

#ifndef FACTORY_H
#define FACTORY_H

#include "exports.h"

#include "FactoryObject.h"

#include <memory>

namespace GemRB {

class GEM_EXPORT Factory {
public:
	using object_t = std::shared_ptr<FactoryObject>;

	Factory() noexcept = default;
	Factory(const Factory&) = delete;
	Factory& operator=(const Factory&) = delete;

	void AddFactoryObject(object_t fobject);
	int IsLoaded(const ResRef& resRef, SClass_ID type) const;
	object_t GetFactoryObject(int pos) const;

private:
	std::vector<object_t> fobjects;
};

}

#endif
