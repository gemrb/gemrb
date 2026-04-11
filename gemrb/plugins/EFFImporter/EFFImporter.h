// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef EFFIMPORTER_H
#define EFFIMPORTER_H

#include "EffectMgr.h"

namespace GemRB {

class EFFImporter : public EffectMgr {
private:
	DataStream* str = nullptr;
	bool autoFree = false;
	int version = 0;

public:
	EFFImporter() noexcept = default;
	EFFImporter(const EFFImporter&) = delete;
	~EFFImporter() override;
	EFFImporter& operator=(const EFFImporter&) = delete;
	// We need this autoFree, since Effects are included inline
	// in other file types, without a size header.
	bool Open(DataStream* stream, bool autoFree = true) override;
	std::unique_ptr<Effect> GetEffect() override;
	std::unique_ptr<Effect> GetEffectV1() override;
	std::unique_ptr<Effect> GetEffectV20() override;
	void PutEffectV2(DataStream* stream, const Effect* fx) override; // used in the area and cre importer
};


}

#endif
