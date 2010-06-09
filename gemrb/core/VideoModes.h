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

#ifndef VIDEOMODES_H
#define VIDEOMODES_H

#include "exports.h"
#include "win32def.h"

#include "VideoMode.h"

#include <vector>

class GEM_EXPORT VideoModes {
private:
	std::vector< VideoMode> modes;
public:
	VideoModes(void);
	~VideoModes(void);
	int AddVideoMode(int w, int h, int bpp, bool fs, bool checkUnique = true);
	int FindVideoMode(VideoMode& vm);
	void RemoveEntry(unsigned int n);
	void Empty(void);
	VideoMode operator[](unsigned int n);
	unsigned int Count(void);
};

#endif
