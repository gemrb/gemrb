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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Ambient.h,v 1.3 2004/08/09 18:24:28 avenger_teambg Exp $
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
	Ambient();
	~Ambient();
	const char *getName() const;
	const Point &getOrigin() const;
	ieWord getRadius() const;
	ieWord getHeight() const;
	ieWord getGain() const;
	char *getSound(ieDword i);
	ieDword getInterval() const;
	ieDword getPerset() const;
	ieDword getAppearance() const;
	ieDword getFlags() const;
	void setActive();
	void setInactive();
    
public:
	char name[32];
	Point origin;
	ieWord radius;
	ieWord height;
	ieWord gain;	// percent
	std::vector<char *> sounds;
	ieDword interval;	// no pauses if zero
	ieDword perset;
	ieDword appearance;
	ieDword flags;

};

#endif
