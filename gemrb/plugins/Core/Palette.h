/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2005-2006 The GemRB Project
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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Palette.h,v 1.1 2006/01/27 17:30:59 wjpalenstijn Exp $
 */

#ifndef PALETTE_H
#define PALETTE_H


#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

class GEM_EXPORT Palette {
public:
	Palette(Color* colours, bool alpha_=false) {
		for (int i = 0; i < 256; ++i) {
			col[i] = colours[i];
		}
		alpha = alpha_;
		refcount = 1;
	}
	~Palette() { }

	Color col[256]; //< RGB or RGBA 8 bit palette
//	Uint32 syscol[256]; //< palette converted to display format
	bool alpha; //< true if this is a RGBA palette

	void IncRef() {
		refcount++;
	}

	void Release() {
		assert(refcount > 0);
		if (!--refcount)
			delete this;
	}

	bool IsShared() const {
		return (refcount > 1);
	}

private:
	unsigned int refcount;

};

#endif
