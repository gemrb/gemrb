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

#include "Audio.h"

namespace GemRB {

const TypeID Audio::ID = { "Audio" };

Audio::Audio(void)
{
	// create the built-in default channels (sndchann.2da)
	std::array<std::string, int(SFXChannel::count)> channelNames {
		"NARRATIO", "AREA_AMB", "ACTIONS", "SWINGS", "CASTING", "GUI", "DIALOG", "CHARACT0", "CHARACT1",
		"CHARACT2", "CHARACT3", "CHARACT4", "CHARACT5", "CHARACT6", "CHARACT7", "CHARACT8", "CHARACT9",
		"MONSTER", "HITS", "MISSILE", "AMBIENTL", "AMBIENTN", "WALKINGC", "WALKINGM", "ARMOR"
	};
	for (SFXChannel channelID : EnumIterator<SFXChannel>()) {
		channels[channelID] = Channel(channelNames[int(channelID)]);
	}
}

void Audio::SetChannelVolume(const std::string& name, int volume)
{
	if (volume > 100) {
		volume = 100;
	} else if (volume < 0) {
		volume = 0;
	}

	SFXChannel channel = GetChannel(name);
	if (channel == SFXChannel::count) {
		return; // ignore, since only the fixed set of channels are used, usable
	}
	channels[channel].setVolume(volume);
}

void Audio::SetChannelReverb(const std::string& name, float reverb)
{
	if (reverb > 1.0f) {
		reverb = 1.0f;
	} else if (reverb < 0.0f) {
		reverb = 0.0f;
	}

	SFXChannel channel = GetChannel(name);
	if (channel == SFXChannel::count) {
		return;
	}
	channels[channel].setReverb(reverb);
}

SFXChannel Audio::GetChannel(const std::string& name) const
{
	for (SFXChannel channelID : EnumIterator<SFXChannel>()) {
		if (channels[channelID].getName() == name) {
			return channelID;
		}
	}
	return SFXChannel::count;
}

int Audio::GetVolume(SFXChannel channel) const
{
	if (channel >= SFXChannel::count) {
		return 100;
	}
	return channels[channel].getVolume();
}

float Audio::GetReverb(SFXChannel channel) const
{
	if (channel >= SFXChannel::count) {
		return 0.0f;
	}
	return channels[channel].getReverb();
}

Holder<SoundHandle> Audio::PlayMB(const String& resource, SFXChannel channel, const Point& p, unsigned int flags, tick_t* length)
{
	auto mbString = MBStringFromString(resource);
	auto mbResource = StringView{mbString};

	return Play(mbResource, channel, p, flags, length);
}

}
