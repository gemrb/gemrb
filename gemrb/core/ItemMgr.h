// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

/**
 * @file ItemMgr.h
 * Declares ItemMgr class, loader for Item objects
 * @author The GemRB Project
 */


#ifndef ITEMMGR_H
#define ITEMMGR_H

#include "Plugin.h"
#include "TableMgr.h"

namespace GemRB {

class Item;

/**
 * @class ItemMgr
 * Abstract loader for Item objects
 */

class GEM_EXPORT ItemMgr : public ImporterBase {
protected:
	AutoTable tooltipTable; // tooltips (duh)
	AutoTable exclusionTable; // a table of items that are mutually exclusive
	AutoTable dialogTable; // dialogs attached to items (conversables such as Lilarcor)

public:
	ItemMgr(void);
	virtual Item* GetItem(Item* s) = 0;
};

}

#endif
