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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Ambient.cpp,v 1.2 2004/08/09 18:24:28 avenger_teambg Exp $
 *
 */
 
#include "Ambient.h"

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

const char *Ambient::getName() const { return name; }
const Point &Ambient::getOrigin() const { return origin; }
ieWord Ambient::getRadius() const { return radius; }
ieWord Ambient::getHeight() const { return height; }
ieWord Ambient::getGain() const { return gain; }
char *Ambient::getSound(ieDword i)
{
	if(i<sounds.size()) return sounds[i];
	return NULL;
}
ieDword Ambient::getInterval() const { return interval; }
ieDword Ambient::getPerset() const { return perset; }
ieDword Ambient::getAppearance() const { return appearance; }
ieDword Ambient::getFlags() const { return flags; }
void Ambient::setActive() { flags |= IE_AMBI_ENABLED; }
void Ambient::setInactive() { flags &= ~IE_AMBI_ENABLED; }

