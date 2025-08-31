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

#include "AudioSettings.h"

#include "Interface.h"

namespace GemRB {

const static std::unordered_map<std::string, SFXChannel> channelEnumMap = {
	{ "NARRATIO", SFXChannel::Narrator },
	{ "AREA_AMB", SFXChannel::MainAmbient },
	{ "ACTIONS", SFXChannel::Actions },
	{ "SWINGS", SFXChannel::Swings },
	{ "CASTING", SFXChannel::Casting },
	{ "GUI", SFXChannel::GUI },
	{ "DIALOG", SFXChannel::Dialog },
	{ "CHARACT0", SFXChannel::Char0 },
	{ "CHARACT1", SFXChannel::Char1 },
	{ "CHARACT2", SFXChannel::Char2 },
	{ "CHARACT3", SFXChannel::Char3 },
	{ "CHARACT4", SFXChannel::Char4 },
	{ "CHARACT5", SFXChannel::Char5 },
	{ "CHARACT6", SFXChannel::Char6 },
	{ "CHARACT7", SFXChannel::Char7 },
	{ "CHARACT8", SFXChannel::Char8 },
	{ "CHARACT9", SFXChannel::Char9 },
	{ "MONSTER", SFXChannel::Monster },
	{ "HITS", SFXChannel::Hits },
	{ "MISSILE", SFXChannel::Missile },
	{ "AMBIENTL", SFXChannel::AmbientLoop },
	{ "AMBIENTN", SFXChannel::AmbientOther },
	{ "WALKINGC", SFXChannel::WalkChar },
	{ "WALKINGM", SFXChannel::WalkMonster },
	{ "ARMOR", SFXChannel::Armor }
};

SFXChannel AudioSettings::GetChannelByName(StringView channelName)
{
	auto it = channelEnumMap.find(std::string { channelName.c_str() });
	if (it != channelEnumMap.end()) {
		return it->second;
	}

	return SFXChannel::count;
}

void AudioSettings::SetScreenSize(Size size)
{
	screenSize = size;
}

void AudioSettings::UpdateChannel(StringView channelName, int volume)
{
	auto key = std::string { channelName.c_str() };
	auto it = channelEnumMap.find(key);
	if (it == channelEnumMap.cend()) {
		return;
	}

	auto idx = it->second;
	volume = Clamp(volume, 0, 100);
	channels[idx] = volume;
}

int AudioSettings::GetChannelHeight(SFXChannel channel) const
{
	if (channel == SFXChannel::Actions) {
		return 100;
	} else {
		return 0;
	}
}

int AudioSettings::GetVolumeByChannel(SFXChannel channel) const
{
	if (channel >= SFXChannel::count) {
		return 100;
	} else {
		return channels[channel];
	}
}

int AudioSettings::GetVolumeBySetting(StringView settingName) const
{
	return core->GetDictionary().Get(settingName, 100);
}

int AudioSettings::GetMusicVolume() const
{
	return GetVolumeBySetting("Volume Music");
}

int AudioSettings::GetAmbientVolume() const
{
	return GetVolumeBySetting("Volume Ambients");
}

bool AudioSettings::UseEnvironmentalAudio() const
{
	return core->GetDictionary().Get("Environmental Audio", 0) > 0;
}

AudioPlaybackConfig AudioSettings::ConfigPresetMusic() const
{
	AudioPlaybackConfig config;
	config.masterVolume = GetVolumeBySetting("Volume Music");

	return config;
}

AudioPlaybackConfig AudioSettings::ConfigPresetMovie() const
{
	AudioPlaybackConfig config;
	config.masterVolume = GetVolumeBySetting("Volume Movie");

	return config;
}

// Dialog: not spatial, no EFX
AudioPlaybackConfig AudioSettings::ConfigPresetDialog(SFXChannel channel) const
{
	AudioPlaybackConfig config;
	config.channelVolume = GetVolumeByChannel(channel);
	config.masterVolume = GetVolumeBySetting("Volume Voices");

	return config;
}

// Directional (casting voices)
AudioPlaybackConfig AudioSettings::ConfigPresetDirectional(SFXChannel channel, const Point& p, orient_t orientation) const
{
	auto config = ConfigPresetSpatialVoice(channel, p);
	config.directional = true;
	config.cone = 120;

	using t = std::underlying_type_t<orient_t>;
	// 0 is south, and 90 deg turn is west
	float rot = (static_cast<t>(orientation) / (1.0f * static_cast<t>(orient_t::MAX)) * 360.0f) * (M_PI / 180.0f);
	rot = rot * -1.0f - M_PI * 0.5f;
	float x = std::cos(rot);
	float y = std::sin(rot);
	config.direction = { x, y, 0.0f };

	return config;
}

// Ambient with a location and range by IE data
AudioPlaybackConfig AudioSettings::ConfigPresetPointAmbient(int gain, const Point& p, uint16_t range, bool loop) const
{
	auto channel = loop ? SFXChannel::MainAmbient : SFXChannel::AmbientOther;
	auto channelVolume = GetVolumeByChannel(channel);

	AudioPlaybackConfig config;
	config.masterVolume = GetVolumeBySetting("Volume Ambients");
	config.channelVolume = ((std::max(45, gain) * channelVolume) / 100) * 2;
	config.efx = true && UseEnvironmentalAudio();
	config.spatial = true;
	config.muteDistance = range;
	config.position = { p, GetChannelHeight(channel) };
	config.loop = loop;

	return config;
}

// General map ambient
AudioPlaybackConfig AudioSettings::ConfigPresetMainAmbient(int gain, bool loop) const
{
	auto channelVolume = GetVolumeByChannel(loop ? SFXChannel::MainAmbient : SFXChannel::AmbientOther);

	AudioPlaybackConfig config;
	config.masterVolume = GetVolumeBySetting("Volume Ambients");
	config.loop = loop;
	config.channelVolume = (gain * channelVolume / 100);

	return config;
}

// Spatial sound effects
AudioPlaybackConfig AudioSettings::ConfigPresetByChannel(SFXChannel channel, const Point& p) const
{
	auto distance = std::min(screenSize.w, screenSize.h);

	AudioPlaybackConfig config;
	config.efx = true && UseEnvironmentalAudio();
	config.spatial = true;
	config.muteDistance = distance;
	config.position = { p, GetChannelHeight(channel) };
	config.channelVolume = GetVolumeByChannel(channel);
	config.masterVolume = GetVolumeBySetting("Volume SFX");

	if (channel == SFXChannel::WalkChar || channel == SFXChannel::WalkMonster) {
		config.muteDistance = (distance * 3) / 2;
	}

	return config;
}

// UI and some other things
AudioPlaybackConfig AudioSettings::ConfigPresetScreenAction(SFXChannel channel) const
{
	AudioPlaybackConfig config;
	config.masterVolume = GetVolumeByChannel(channel);

	return config;
}

// Selection sounds
AudioPlaybackConfig AudioSettings::ConfigPresetEnvVoice(SFXChannel channel) const
{
	AudioPlaybackConfig config;
	config.efx = true && UseEnvironmentalAudio();
	config.channelVolume = GetVolumeByChannel(channel);
	config.masterVolume = GetVolumeBySetting("Volume Voices");

	return config;
}

// Spatial voices
AudioPlaybackConfig AudioSettings::ConfigPresetSpatialVoice(SFXChannel channel, const Point& p) const
{
	auto distance = std::min(screenSize.w, screenSize.h);

	AudioPlaybackConfig config;
	config.efx = true && UseEnvironmentalAudio();
	config.spatial = true;
	config.muteDistance = distance;
	config.position = { p, GetChannelHeight(channel) };
	config.channelVolume = GetVolumeByChannel(channel);
	config.masterVolume = GetVolumeBySetting("Volume Voices");

	return config;
}

}
