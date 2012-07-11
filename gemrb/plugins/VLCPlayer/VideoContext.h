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

#ifndef VIDCTX_H
#define VIDCTX_H

#include <pthread.h>

namespace GemRB {
	
class VideoContext
{
private:
	pthread_mutex_t mutex;
	void* planes[3];

	bool YUV;
	unsigned width;
	unsigned height;
public:
	VideoContext(unsigned w, unsigned h, bool yuv);
	~VideoContext(void);

	int Lock();
	int Unlock();

	void* GetPlane(unsigned idx = 0);
	unsigned GetStride(unsigned idx = 0);
	bool isYUV() { return YUV; };
	unsigned Width() { return width; };
	unsigned Height() { return height; };
};

}

#endif
