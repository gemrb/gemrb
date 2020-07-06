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

#include <vector>

#include "globals.h"
#include "win32def.h"

#include "Plugin.h"
#include "Holder.h"

namespace GemRB {

#define GEM_SND_RELATIVE 1
#define GEM_SND_LOOPING 2
#define GEM_SND_SPEECH   IE_STR_SPEECH // 4
#define GEM_SND_QUEUE	8

#define GEM_SND_VOL_MUSIC    1
#define GEM_SND_VOL_AMBIENTS 2

#define SFX_CHAN_NARRATOR	0
#define SFX_CHAN_AREA_AMB	1
#define SFX_CHAN_ACTIONS	2
#define SFX_CHAN_SWINGS		3
#define SFX_CHAN_CASTING	4
#define SFX_CHAN_GUI		5
#define SFX_CHAN_DIALOG		6
#define SFX_CHAN_CHAR0		7
#define SFX_CHAN_CHAR1		8
#define SFX_CHAN_CHAR2		9
#define SFX_CHAN_CHAR3		10
#define SFX_CHAN_CHAR4		11
#define SFX_CHAN_CHAR5		12
#define SFX_CHAN_CHAR6		13
#define SFX_CHAN_CHAR7		14
#define SFX_CHAN_CHAR8		15
#define SFX_CHAN_CHAR9		16
#define SFX_CHAN_MONSTER	17
#define SFX_CHAN_HITS		18
#define SFX_CHAN_MISSILE	19
#define SFX_CHAN_AMB_LOOP	20
#define SFX_CHAN_AMB_OTHER 	21
#define SFX_CHAN_WALK_CHAR 	22
#define SFX_CHAN_WALK_MONSTER 23
#define SFX_CHAN_ARMOR		24

class AmbientMgr;
class SoundMgr;
class MapReverb;

class GEM_EXPORT SoundHandle : public Held<SoundHandle> {
public:
	virtual bool Playing() = 0;
	virtual void SetPos(int XPos, int YPos) = 0;
	virtual void Stop() = 0;
	virtual void StopLooping() = 0;
	virtual ~SoundHandle();
};

class GEM_EXPORT Channel {
public:
	Channel(const char *label) : volume(100), reverb(0.0f)
		{ strncpy(name, label, sizeof(name)-1); name[sizeof(name)-1] = '\0'; }
	~Channel() {}

	const char *getName() const { return name; }
	int getVolume() const { return volume; }
	void setVolume(int vol) { volume = vol; }
	float getReverb() const { return reverb; }
	void setReverb(float r) { reverb = r; }

private:
	char name[10];
	int volume; // 1-100
	float reverb;
};

class GEM_EXPORT Audio : public Plugin {
public:
	static const TypeID ID;
public:
	Audio(void);
	virtual ~Audio();
	virtual bool Init(void) = 0;
	virtual Holder<SoundHandle> Play(const char* ResRef, unsigned int channel,
				int XPos, int YPos, unsigned int flags = 0, unsigned int *length = 0) = 0;
	virtual Holder<SoundHandle> Play(const char* ResRef, unsigned int channel, unsigned int *length = 0)
			{ return Play(ResRef, channel, 0, 0, GEM_SND_RELATIVE, length); }
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
				ieWord gain, bool point, int ambientRange) = 0;
	virtual int QueueAmbient(int stream, const char* sound) = 0;
	virtual void SetAmbientStreamVolume(int stream, int volume) = 0;
	virtual void SetAmbientStreamPitch(int stream, int pitch) = 0;
	virtual void QueueBuffer(int stream, unsigned short bits,
				int channels, short* memory, int size, int samplerate) = 0;
	virtual void UpdateMapAmbient(MapReverb&) {};

	unsigned int CreateChannel(const char *name);
	void SetChannelVolume(const char *name, int volume);
	void SetChannelReverb(const char *name, float reverb);
	unsigned int GetChannel(const char *name) const;
	int GetVolume(unsigned int channel) const;
	float GetReverb(unsigned int channel) const;
protected:
	AmbientMgr* ambim;
	std::vector<Channel> channels;
};

}

#endif // AUDIO_H_INCLUDED
