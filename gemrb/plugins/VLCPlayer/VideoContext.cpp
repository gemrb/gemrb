/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2012 The GemRB Project
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

#include "VLCPlayer.h"
#include "Video.h"

using namespace GemRB;

VideoContext::VideoContext(unsigned w, unsigned h, bool yuv)
: width(w), height(h), YUV(yuv)
{
	if(pthread_mutex_init(&mutex, NULL) != GEM_OK) {
		Log(ERROR, "VLC Player", "Unable to create mutex!");
	}
	
	int size = width * height;
	if (YUV) {
		planes[0] = new char[size];
		planes[1] = new char[size / 2];
		planes[2] = new char[size / 2];
	} else { // 16bit RGB
		planes[0] = new char[size * 2];
		planes[1] = NULL;
		planes[2] = NULL;
	}
}

VideoContext::~VideoContext(void)
{
	pthread_mutex_destroy(&mutex);
	delete[] (char*)planes[0];
	if (YUV) {
		delete[] (char*)planes[1];
		delete[] (char*)planes[2];
	}
}

int VideoContext::Lock()
{
	return pthread_mutex_lock(&mutex);
}

int VideoContext::Unlock()
{
	return pthread_mutex_unlock(&mutex);
}

void* VideoContext::GetPlane(unsigned idx)
{
	if (YUV) {
		if (idx <= 3) return planes[idx];
	} else {
		return planes[0];
	}
	Log(ERROR, "VLCPlayer", "Plane index out of range.");
	return NULL;
}

unsigned VideoContext::GetStride(unsigned /*idx*/)
{
	// idx is a placeholder if we decide to use the native strides
	// for now we are lazy and will make VLC convert all plane strides to width
	
	return width;
}
