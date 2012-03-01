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

#ifndef NULLSOUND_H
#define NULLSOUND_H

#include "Audio.h"

namespace GemRB {

class NullSound : public Audio {
public:
	NullSound(void);
	~NullSound(void);
	bool Init(void);
	Holder<SoundHandle> Play(const char* ResRef, int XPos, int YPos,  unsigned int flags = 0, unsigned int *length = 0);
	int CreateStream(Holder<SoundMgr>);
	bool Play();
	bool Stop();
	bool Pause();
	bool Resume();
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
	int XPos, YPos;
};

}

#endif
