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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Ambient.h,v 1.1 2004/08/08 05:11:32 divide Exp $
 *
 */
 
#ifndef AMBIENT_H
#define AMBIENT_H

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

#include <string>
#include <vector>
#include <bitset>
#include <../../includes/ie_types.h>
#include <../../includes/globals.h>

#define IE_AMBI_ENABLED 1
#define IE_AMBI_POINT   2
#define IE_AMBI_MAIN    4
#define IE_AMBI_AREA    8

class GEM_EXPORT Ambient {
public:
	Ambient(const std::string &name, Point origin, unsigned int radius, int height, int gain, const std::vector<std::string> &sounds, unsigned int interval, unsigned int perset, const std::bitset<24> &appearance, unsigned long flags);
	~Ambient();
	unsigned long getFlags() const { return flags; }
	void setActive() { flags |= IE_AMBI_ENABLED; }
	void setInactive() { flags &= !IE_AMBI_ENABLED; }
	std::string getName() const { return name; }
    
private:
	std::string name;
	Point origin;
	unsigned int radius;
	int height;
	unsigned int gain;	// percent
	std::vector<std::string> sounds;
	unsigned int interval;	// no pauses if zero
	unsigned int perset;
	std::bitset<24> appearance;
	unsigned long flags;

};

#endif
