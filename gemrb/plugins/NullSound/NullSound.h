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
	~NullSound(void) override;
	bool Init(void) override;
	Holder<SoundHandle> Play(StringView ResRef, SFXChannel channel,
		const Point&, unsigned int flags = 0, tick_t *length = nullptr) override;
	int CreateStream(ResourceHolder<SoundMgr>) override;
	bool Play() override;
	bool Stop() override;
	bool Pause() override;
	bool Resume() override;
	bool CanPlay() override;
	void ResetMusics() override;
	void UpdateListenerPos(const Point&) override;
	Point GetListenerPos() override;
	void UpdateVolume(unsigned int) override {}

	int SetupNewStream(int x, int y, int z, ieWord gain, bool point, int ambientRange) override;
	tick_t QueueAmbient(int stream, const ResRef& sound) override;
	bool ReleaseStream(int stream, bool hardstop) override;
	void SetAmbientStreamVolume(int stream, int gain) override;
	void SetAmbientStreamPitch(int stream, int pitch) override;
	void QueueBuffer(int stream, unsigned short bits, int channels,
				short* memory, int size, int samplerate) override;

private:
	Point pos;
};

}

#endif
