// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ItemMgr.h"

#include "GameData.h"

namespace GemRB {

ItemMgr::ItemMgr(void)
{
	tooltipTable = gamedata->LoadTable("tooltip", true);
	exclusionTable = gamedata->LoadTable("itemexcl", true);
	dialogTable = gamedata->LoadTable("itemdial", true);
}

}
