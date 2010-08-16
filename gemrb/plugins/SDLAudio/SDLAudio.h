/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#ifndef SDLAUDIO_H
#define SDLAUDIO_H

#include "Audio.h"
#include <vector>

struct BufferedData {
	char *buf;
	unsigned int size;
};

class SDLAudio : public Audio {
public:
	SDLAudio(void);
	~SDLAudio(void);
	bool Init(void);
	unsigned int Play(const char* ResRef, int XPos = 0, int YPos = 0,  unsigned int flags = GEM_SND_RELATIVE);
	int CreateStream(Holder<SoundMgr>);
	bool Play();
	bool Stop();
	bool CanPlay();
	bool IsSpeaking();
	void ResetMusics();
	void UpdateListenerPos(int XPos, int YPos);
	void GetListenerPos(int& XPos, int& YPos);
	void UpdateVolume(unsigned int) {}

	int SetupNewStream(ieWord x, ieWord y, ieWord z, ieWord gain, bool point, bool Ambient);
	int QueueAmbient(int stream, const char* sound);
	bool ReleaseStream(int stream, bool hardstop);
	void SetAmbientStreamVolume(int stream, int gain);
	void QueueBuffer(int stream, unsigned short bits, int channels,
                short* memory, int size, int samplerate);

private:
	void FreeBuffers();

	static void music_callback(void *udata, unsigned short *stream, int len);
	static void buffer_callback(void *udata, char *stream, int len);

	int XPos, YPos;
	Holder<SoundMgr> MusicReader;

	bool MusicPlaying;
	unsigned int curr_buffer_offset;
	std::vector<BufferedData> buffers;

	int audio_rate;
	unsigned short audio_format;
	int audio_channels;

	struct SDL_mutex* OurMutex;
};

#endif
