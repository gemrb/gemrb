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
 * $Id$
 *
 */

#include "../../includes/win32def.h"
#include "VideoModes.h"
#include "../../includes/errors.h"
#include <vector>

using namespace std;

VideoModes::VideoModes(void)
{
}

VideoModes::~VideoModes(void)
{
}

int VideoModes::AddVideoMode(int w, int h, int bpp, bool fs, bool checkUnique)
{
	VideoMode vm( w, h, bpp, fs );
	if (checkUnique) {
		for (unsigned int i = 0; i < modes.size(); i++) {
			if (modes[i] == vm) {
				return GEM_ERROR;
			}
		}
	}
	modes.push_back( vm );
	return GEM_OK;
}

int VideoModes::FindVideoMode(VideoMode& vm)
{
	for (unsigned int i = 0; i < modes.size(); i++) {
		if (modes[i] == vm) {
			return i;
		}
	}
	return GEM_ERROR;
}

void VideoModes::RemoveEntry(unsigned int n)
{
	if (n >= modes.size()) {
		return;
	}
	vector< VideoMode>::iterator m = modes.begin();
	m += n;
	modes.erase( m );
}

void VideoModes::Empty(void)
{
	vector< VideoMode>::iterator m;
	for (m = modes.begin(); m != modes.end(); ++m) {
		modes.erase( m );
	}
}

VideoMode VideoModes::operator[](unsigned int n)
{
	if (n >= modes.size()) {
		return VideoMode();
	}
	return modes[n];
}

unsigned int VideoModes::Count(void)
{
	return (unsigned int) modes.size();
}

