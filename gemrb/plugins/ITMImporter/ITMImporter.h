// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef ITMIMPORTER_H
#define ITMIMPORTER_H

#include "Item.h"
#include "ItemMgr.h"

namespace GemRB {

#define ITM_VER_BG   10
#define ITM_VER_PST  11
#define ITM_VER_IWD2 20

class ITMImporter : public ItemMgr {
private:
	int version = 0;

public:
	Item* GetItem(Item* s) override;

private:
	bool Import(DataStream* stream) override;
	void GetExtHeader(const Item* s, ITMExtHeader* eh);
	std::unique_ptr<Effect> GetFeature(const Item* s);
};


}

#endif
