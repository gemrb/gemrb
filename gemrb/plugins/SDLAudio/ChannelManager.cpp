/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2025 The GemRB Project
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
 */

#include "ChannelManager.h"

#include "SDLAudio.h"

#include "Logging/Logging.h"

namespace GemRB {

std::recursive_mutex ChannelManager::stateMutex = {};
bool ChannelManager::initialized = false;
std::unordered_map<int, uint64_t> ChannelManager::channelMap = {};

bool ChannelManager::Init(int numChannels, int numReserved)
{
	std::unique_lock<std::recursive_mutex> lock { stateMutex };

	if (initialized) {
		return false;
	}

	int result = Mix_AllocateChannels(numChannels);
	if (result < numChannels) {
		Log(WARNING, "SDLAudio", "Unable to fully allocate the required amount of channels.");
	}

	Mix_ReserveChannels(numReserved);
	Mix_ChannelFinished(&finishedCallback);

	numChannels = result;
	initialized = true;

	return true;
}

int ChannelManager::Request(const SDLSoundSourceHandle& source, int channel, Mix_Chunk* chunk, bool loop)
{
	std::unique_lock<std::recursive_mutex> lock { stateMutex };

	int loops = loop ? -1 : 0;
	auto nextChannel = Mix_PlayChannel(channel, chunk, loops);
	if (nextChannel == -1) {
		return -1;
	}

	channelMap.emplace(nextChannel, source.GetID());

	return nextChannel;
}

bool ChannelManager::IsMyChannel(const SDLSoundSourceHandle& source, int channel)
{
	auto it = channelMap.find(channel);

	return it != channelMap.cend() && it->second == source.GetID();
}

void ChannelManager::finishedCallback(int channel)
{
	std::unique_lock<std::recursive_mutex> lock { stateMutex };
	channelMap.erase(channel);
}

}
