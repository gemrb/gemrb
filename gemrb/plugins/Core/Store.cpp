/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Store.cpp,v 1.4 2004/10/09 15:27:23 avenger_teambg Exp $
 *
 */

#include "../../includes/win32def.h"
#include "Store.h"
#include "Interface.h"

Store::Store(void)
{
}

Store::~Store(void)
{
	unsigned int i;

	for (i = 0; i < items.size(); i++) {
		delete( items[i] );
	}
	for (i = 0; i < drinks.size(); i++) {
		delete( drinks[i] );
	}
	for (i = 0; i < cures.size(); i++) {
		delete( cures[i] );
	}
	free( purchased_categories );
}
