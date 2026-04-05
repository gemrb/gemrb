// SPDX-FileCopyrightText: 2022 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef H_IMPORTER_PLUGIN
#define H_IMPORTER_PLUGIN

#include "exports.h"

#include "Plugin.h"

namespace GemRB {

template<class IMPORTER>
class GEM_EXPORT_T ImporterPlugin final : public Plugin {
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
