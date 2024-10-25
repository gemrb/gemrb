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

#include "globals.h"

#include "MoviePlayer.h"
#include "mve_player.h"

namespace GemRB {

class Video;

class MVEPlay : public MoviePlayer {
	friend class MVEPlayer;
	MVEPlayer decoder;
	VideoBuffer* vidBuf;
	Holder<Palette> g_palette;

private:
	PluginHolder<Video> video;
	bool validVideo;
	int doPlay();
	unsigned int fileRead(void* buf, unsigned int count);
	void showFrame(const unsigned char* buf, unsigned int bufw, unsigned int bufh);
	void setPalette(unsigned char* p, unsigned start, unsigned count) const;
	int pollEvents();
	int setAudioStream() const;
	void freeAudioStream(int stream) const;
	void queueBuffer(int stream, unsigned short bits,
			 int channels, short* memory,
			 int size, int samplerate) const;

protected:
	bool DecodeFrame(VideoBuffer&) override;

public:
	MVEPlay() noexcept;
	bool Import(DataStream* stream) override;
};

}

#endif
