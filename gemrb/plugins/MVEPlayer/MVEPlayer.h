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

#ifndef MVEPLAY_H
#define MVEPLAY_H

#include "MoviePlayer.h"

#include "globals.h"
#include "win32def.h"

namespace GemRB {

class Video;

class MVEPlay : public MoviePlayer {
	friend class MVEPlayer;
private:
	Video *video;
	bool validVideo;
	int doPlay();
	unsigned int fileRead(void* buf, unsigned int count);
	void showFrame(unsigned char* buf, unsigned int bufw,
		unsigned int bufh, unsigned int sx, unsigned int sy,
		unsigned int w, unsigned int h, unsigned int dstx,
		unsigned int dsty);
	void setPalette(unsigned char* p, unsigned start, unsigned count);
	int pollEvents();
	int setAudioStream();
	void freeAudioStream(int stream);
	void queueBuffer(int stream, unsigned short bits,
				int channels, short* memory,
				int size, int samplerate);
public:
	MVEPlay(void);
	~MVEPlay(void);
	bool Open(DataStream* stream);
	void CallBackAtFrames(ieDword cnt, ieDword *arg, ieDword *arg2);
	int Play();
};

}

#endif
