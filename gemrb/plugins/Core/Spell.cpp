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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Spell.cpp,v 1.3 2004/05/25 16:16:31 avenger_teambg Exp $
 *
 */

#include "../../includes/win32def.h"
#include "Spell.h"
#include "Interface.h"

Spell::Spell(void)
{
}

Spell::~Spell(void)
{
  unsigned int i;

	for (i = 0; i < ext_headers.size(); i++) {
		delete( ext_headers[i] );
	}
	// FIXME: release eh->features too
	for (i = 0; i < casting_features.size(); i++) {
		delete( casting_features[i] );
	}
}
