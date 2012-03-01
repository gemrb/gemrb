/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003-2004 The GemRB Project
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

#ifndef AUDIO_H_INCLUDED
#define AUDIO_H_INCLUDED

#include "globals.h"
#include "win32def.h"

#include "Plugin.h"
#include "Holder.h"

namespace GemRB {

#define GEM_SND_RELATIVE 1
#define GEM_SND_LOOPING 2
#define GEM_SND_SPEECH   IE_STR_SPEECH // 4
#define GEM_SND_VOL_MUSIC    1
#define GEM_SND_VOL_AMBIENTS 2

class AmbientMgr;
class SoundMgr;

class GEM_EXPORT SoundHandle : public Held<SoundHandle> {
public:
	virtual bool Playing() = 0;
	virtual void SetPos(int XPos, int YPos) = 0;
	virtual void Stop() = 0;
	virtual void StopLooping() = 0;
	virtual ~SoundHandle();
};

class GEM_EXPORT Audio : public Plugin {
public:
	static const TypeID ID;
public:
	Audio(void);
	virtual ~Audio();
	virtual bool Init(void) = 0;
	virtual Holder<SoundHandle> Play(const char* ResRef, int XPos, int YPos, unsigned int flags = 0, unsigned int *length = 0) = 0;
	virtual Holder<SoundHandle> Play(const char* ResRef, unsigned int *length = 0) { return Play(ResRef, 0, 0, GEM_SND_RELATIVE, length); }
	virtual bool IsSpeaking() = 0;
	virtual AmbientMgr* GetAmbientMgr() { return ambim; }
	virtual void UpdateVolume(unsigned int flags = GEM_SND_VOL_MUSIC | GEM_SND_VOL_AMBIENTS) = 0;
	virtual bool CanPlay() = 0;
	virtual void ResetMusics() = 0;
	virtual bool Play() = 0;
	virtual bool Stop() = 0;
	virtual bool Pause() = 0;
	virtual bool Resume() = 0;
	virtual int CreateStream(Holder<SoundMgr>) = 0;
	virtual void UpdateListenerPos(int XPos, int YPos ) = 0;
	virtual void GetListenerPos(int &XPos, int &YPos ) = 0;
	virtual bool ReleaseStream(int stream, bool HardStop=false ) = 0;
	virtual int SetupNewStream( ieWord x, ieWord y, ieWord z,
				ieWord gain, bool point, bool Ambient) = 0;
	virtual int QueueAmbient(int stream, const char* sound) = 0;
	virtual void SetAmbientStreamVolume(int stream, int volume) = 0;
	virtual void QueueBuffer(int stream, unsigned short bits,
				int channels, short* memory, int size, int samplerate) = 0;

protected:
	AmbientMgr* ambim;

};

}

#endif // AUDIO_H_INCLUDED
