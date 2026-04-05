// SPDX-FileCopyrightText: 2025 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef H_SDLAUDIO_CHANNEL_MANAGER
#define H_SDLAUDIO_CHANNEL_MANAGER

#include <SDL_mixer.h>
#include <mutex>
#include <unordered_map>

namespace GemRB {

class SDLSoundSourceHandle;

/* We need this to know if some source handle still has a channel assigned
 * that it actually no longer owns (sound stopped) or worse, has been 
 * recylced by SDL for another source, so that they compete for one channel.
 */

class ChannelManager {
public:
	ChannelManager() = delete;

	static bool Init(int numChannels, int numReserved);
	static bool IsMyChannel(const SDLSoundSourceHandle&, int);
	static int Request(const SDLSoundSourceHandle&, int, Mix_Chunk*, bool);

private:
	static std::recursive_mutex stateMutex;
	static bool initialized;
	static std::unordered_map<int, uint64_t> channelMap;

	static void finishedCallback(int channel);
};

}

#endif
