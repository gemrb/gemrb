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

#define SFX_CHAN_UNKNOWN	((unsigned int) -1)

Audio::Audio(void)
{
	ambim = NULL;
	// create the built-in default channels
	CreateChannel("NARRATIO");
	CreateChannel("AREA_AMB");
	CreateChannel("ACTIONS");
	CreateChannel("SWINGS");
	CreateChannel("CASTING");
	CreateChannel("GUI");
	CreateChannel("DIALOG");
	CreateChannel("CHARACT0");
	CreateChannel("CHARACT1");
	CreateChannel("CHARACT2");
	CreateChannel("CHARACT3");
	CreateChannel("CHARACT4");
	CreateChannel("CHARACT5");
	CreateChannel("CHARACT6");
	CreateChannel("CHARACT7");
	CreateChannel("CHARACT8");
	CreateChannel("CHARACT9");
	CreateChannel("MONSTER");
	CreateChannel("HITS");
	CreateChannel("MISSILE");
	CreateChannel("AMBIENTL");
	CreateChannel("AMBIENTN");
	CreateChannel("WALKINGC");
	CreateChannel("WALKINGM");
	CreateChannel("ARMOR");
}

Audio::~Audio(void)
{
}

unsigned int Audio::CreateChannel(const char *name)
{
	channels.push_back(Channel(name));
	return channels.size() - 1;
}

void Audio::SetChannelVolume(const char *name, int volume)
{
	if (volume > 100) {
		volume = 100;
	} else if (volume < 0) {
		volume = 0;
	}

	unsigned int channel = GetChannel(name);
	if (channel == SFX_CHAN_UNKNOWN) {
		channel = CreateChannel(name);
	}
	channels[channel].setVolume(volume);
}

void Audio::SetChannelReverb(const char *name, float reverb)
{
	if (reverb > 1.0f) {
		reverb = 1.0f;
	} else if (reverb < 0.0f) {
		reverb = 0.0f;
	}

	unsigned int channel = GetChannel(name);
	if (channel == SFX_CHAN_UNKNOWN) {
		channel = CreateChannel(name);
	}
	channels[channel].setReverb(reverb);
}

unsigned int Audio::GetChannel(const char *name) const
{
	for (std::vector<Channel>::const_iterator c = channels.begin(); c != channels.end(); ++c) {
		if (strcmp((*c).getName(), name) == 0) {
			return c - channels.begin();
		}
	}
	return SFX_CHAN_UNKNOWN;
}

int Audio::GetVolume(unsigned int channel) const
{
	if (channel >= channels.size()) {
		return 100;
	}
	return channels[channel].getVolume();
}

float Audio::GetReverb(unsigned int channel) const
{
	if (channel >= channels.size()) {
		return 0.0f;
	}
	return channels[channel].getReverb();
}

SoundHandle::~SoundHandle()
{
}

}
