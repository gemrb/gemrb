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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Ambient.cpp,v 1.1 2004/08/08 05:11:32 divide Exp $
 *
 */
 
#include "Ambient.h"

Ambient::Ambient(const std::string &name, Point origin, unsigned int radius, int height, int gain, const std::vector<std::string> &sounds, unsigned int interval, unsigned int perset, const std::bitset<24> &appearance, unsigned long flags) : 
	name(name),
	origin(origin),
	radius(radius),
	height(height),
	gain(gain),
	sounds(sounds),
	interval(interval),
	perset(perset),
	appearance(appearance),
	flags(flags)
{
}


Ambient::~Ambient()
{
}
