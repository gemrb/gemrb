// SPDX-FileCopyrightText: 2025 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef H_AUDIO_SETTINGS
#define H_AUDIO_SETTINGS

#include "EnumIndex.h"
#include "Orientation.h"
#include "PlaybackConfig.h"

namespace GemRB {

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

enum class AudioPreset {
	Dialog,
	EnvVoice,
	ScreenAction,
	Spatial,
	SpatialVoice
};

class GEM_EXPORT AudioSettings {
public:
	AudioPlaybackConfig ConfigPresetByChannel(SFXChannel channel, const Point& point) const;
	AudioPlaybackConfig ConfigPresetDialog(SFXChannel channel = SFXChannel::Dialog) const;
	AudioPlaybackConfig ConfigPresetDirectional(SFXChannel channel, const Point& point, orient_t orientation) const;
	AudioPlaybackConfig ConfigPresetMusic() const;
	AudioPlaybackConfig ConfigPresetMovie() const;
	AudioPlaybackConfig ConfigPresetPointAmbient(int gain, const Point& point, uint16_t range, bool loop) const;
	AudioPlaybackConfig ConfigPresetMainAmbient(int gain, bool loop) const;
	AudioPlaybackConfig ConfigPresetScreenAction(SFXChannel channel) const;
	AudioPlaybackConfig ConfigPresetEnvVoice(SFXChannel channel) const;
	AudioPlaybackConfig ConfigPresetSpatialVoice(SFXChannel channel, const Point& point) const;

	int GetMusicVolume() const;
	int GetAmbientVolume() const;

	void SetScreenSize(Size screenSize);
	void UpdateChannel(StringView, int volume);

	static SFXChannel GetChannelByName(StringView channelName);

private:
	EnumArray<SFXChannel, int> channels;
	Size screenSize { 640, 480 };

	int GetChannelHeight(SFXChannel channel) const;
	int GetVolumeByChannel(SFXChannel channel) const;
	int GetVolumeBySetting(StringView setting) const;
	bool UseEnvironmentalAudio() const;
};

}

#endif
