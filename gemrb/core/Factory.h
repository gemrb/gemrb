// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
