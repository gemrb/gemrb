// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef PROIMPORTER_H
#define PROIMPORTER_H

#include "Projectile.h"
#include "ProjectileMgr.h"

namespace GemRB {


class PROImporter : public ProjectileMgr {
private:
	DataStream* str = nullptr;
	int version = 0;

public:
	PROImporter() noexcept = default;
	PROImporter(const PROImporter&) = delete;
	~PROImporter() override;
	PROImporter& operator=(const PROImporter&) = delete;
	bool Open(DataStream* stream) override;
	void GetProjectile(std::unique_ptr<Projectile>& s) override;

private:
	Holder<ProjectileExtension> GetAreaExtension();
};


}

#endif
