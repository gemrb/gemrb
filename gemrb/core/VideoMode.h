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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#ifndef VIDEOMODE_H
#define VIDEOMODE_H

#include "exports.h"

namespace GemRB {

class GEM_EXPORT VideoMode {
public:
	VideoMode(void);
	~VideoMode(void);
	VideoMode(const VideoMode& vm);
	VideoMode(int w, int h, int bpp, bool fs);

private:
	int Width;
	int Height;
	int bpp;
	bool fullscreen;
public:
	void SetWidth(int w);
	int GetWidth(void) const;
	void SetHeight(int h);
	int GetHeight(void) const;
	void SetBPP(int b);
	int GetBPP(void) const ;
	void SetFullScreen(bool fs);
	bool GetFullScreen(void) const;
	bool operator==(const VideoMode& cpt) const;
	VideoMode& operator=(const VideoMode& vm);
};

}

#endif
