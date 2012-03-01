/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2004 The GemRB Project
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
 
#include "Ambient.h"

namespace GemRB {

Ambient::Ambient()
{
}

Ambient::~Ambient()
{
	unsigned int i=sounds.size();
	while(i--) {
		free(sounds[i]);
	}
}

void Ambient::setActive() { flags |= IE_AMBI_ENABLED; }
void Ambient::setInactive() { flags &= ~IE_AMBI_ENABLED; }

}
