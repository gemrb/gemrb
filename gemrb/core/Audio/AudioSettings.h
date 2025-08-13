/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2025 The GemRB Project
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
 */

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
