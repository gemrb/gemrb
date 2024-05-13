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

#include "EnumIndex.h"
#include "MapReverb.h"
#include "Plugin.h"
#include "Resource.h"
#include "Holder.h"

#include <string>

namespace GemRB {

#define GEM_SND_SPATIAL 1
#define GEM_SND_LOOPING 2
#define GEM_SND_SPEECH  4 // STRING_FLAGS::SPEECH
#define GEM_SND_QUEUE   8
#define GEM_SND_EFX    16

#define GEM_SND_VOL_MUSIC    1
#define GEM_SND_VOL_AMBIENTS 2

enum class SFXChannel : unsigned int {
	Narrator,
	MainAmbient, // AREA_AMB in the 2da
	Actions,
	Swings,
	Casting,
	GUI,
	Dialog,
	Char0,
	Char1, // all the other CharN are used derived from Char0
	Char2,
	Char3,
	Char4,
	Char5,
	Char6,
	Char7,
	Char8,
	Char9,
	Monster,
	Hits,
	Missile,
	AmbientLoop,
	AmbientOther,
	WalkChar,
	WalkMonster,
	Armor,

	count
};

class AmbientMgr;
class SoundMgr;
class MapReverb;

class GEM_EXPORT SoundHandle {
public:
	virtual bool Playing() = 0;
	virtual void SetPos(const Point&) = 0;
	virtual void Stop() = 0;
	virtual void StopLooping() = 0;
	virtual ~SoundHandle() = default;
};

class GEM_EXPORT Channel {
public:
	Channel() = default;
	explicit Channel(std::string str)
	: name(std::move(str))
	{}

	const std::string& getName() const { return name; }
	int getVolume() const { return volume; }
	void setVolume(int vol) { volume = vol; }
	float getReverb() const { return reverb; }
	void setReverb(float r) { reverb = r; }

private:
	std::string name;
	int volume = 100; // 1-100
	float reverb = 0.0f;
};

class GEM_EXPORT Audio : public Plugin {
public:
	static const TypeID ID;
public:
	virtual bool Init(void) = 0;
	virtual Holder<SoundHandle> Play(
		StringView ResRef,
		SFXChannel channel,
		const Point&,
		unsigned int flags = 0,
		tick_t *length = nullptr
	) = 0;
	Holder<SoundHandle> PlayMB(
		const String& resource,
		SFXChannel channel,
		const Point&,
		unsigned int flags = 0,
		tick_t* length = nullptr);
	Holder<SoundHandle> Play(StringView ResRef, SFXChannel channel, tick_t* length = nullptr)
			{ return Play(ResRef, channel, Point(), 0, length); }
	
	virtual AmbientMgr* GetAmbientMgr() { return ambim; }
	virtual void UpdateVolume(unsigned int flags = GEM_SND_VOL_MUSIC | GEM_SND_VOL_AMBIENTS) = 0;
	virtual bool CanPlay() = 0;
	virtual void ResetMusics() = 0;
	virtual bool Play() = 0;
	virtual bool Stop() = 0;
	virtual bool Pause() = 0;
	virtual bool Resume() = 0;
	virtual int CreateStream(ResourceHolder<SoundMgr>) = 0;
	virtual void UpdateListenerPos(const Point&) = 0;
	virtual Point GetListenerPos() = 0;
	virtual bool ReleaseStream(int stream, bool HardStop=false ) = 0;
	virtual int SetupNewStream(int x, int y, int z,
				ieWord gain, bool point, int ambientRange) = 0;
	virtual tick_t QueueAmbient(int stream, const ResRef& sound) = 0;
	virtual void SetAmbientStreamVolume(int stream, int volume) = 0;
	virtual void SetAmbientStreamPitch(int stream, int pitch) = 0;
	virtual void QueueBuffer(int stream, unsigned short bits,
				int channels, short* memory, int size, int samplerate) = 0;
	virtual void UpdateMapAmbient(const MapReverbProperties&) {};

	void UpdateChannel(const std::string& name, int idx, int volume, float reverb);
	SFXChannel GetChannel(const std::string& name) const;
	int GetVolume(SFXChannel channel) const;
	float GetReverb(SFXChannel channel) const;
protected:
	AmbientMgr* ambim = nullptr;
	EnumArray<SFXChannel, Channel> channels;
};

}

#endif // AUDIO_H_INCLUDED
