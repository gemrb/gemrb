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
