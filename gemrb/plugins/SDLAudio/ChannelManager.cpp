// SPDX-FileCopyrightText: 2025 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
