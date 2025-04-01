/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2004 The GemRB Project
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
 *
 */

#include "AmbientMgr.h"

#include "Ambient.h"
#include "Game.h"
#include "Interface.h"
#include "RNG.h"

#include <cassert>
#include <chrono>

namespace GemRB {

AmbientMgr::AmbientMgr()
{
	player = std::thread(&AmbientMgr::Play, this);
}

AmbientMgr::~AmbientMgr()
{
	playing = false;
	mutex.lock();
	mutex.unlock();

	cond.notify_all();
	player.join();
}

void AmbientMgr::Reset()
{
	std::lock_guard<std::mutex> l(ambientsMutex);
	ambients.clear();
	AmbientsSet(ambients);
}

void AmbientMgr::UpdateVolume()
{
	SetVolume(core->GetAudioSettings().GetAmbientVolume());
}

void AmbientMgr::SetVolume(unsigned short volume)
{
	std::lock_guard<std::recursive_mutex> l(mutex);
	for (auto& source : ambientSources) {
		source.SetVolume(volume);
	}
}

void AmbientMgr::AmbientsSet(const std::vector<Ambient*>& a)
{
	std::lock_guard<std::recursive_mutex> l(mutex);
	ambientSources.clear();
	ambientSources.reserve(a.size());

	for (auto& source : a) {
		ambientSources.emplace_back(source, bufferCache);
	}
}

void AmbientMgr::RemoveAmbients(const std::vector<Ambient*>& oldAmbients)
{
	std::lock_guard<std::recursive_mutex> l(mutex);
	// manually deleting ambientSources as regenerating them causes a several second pause
	for (auto it = ambientSources.begin(); it != ambientSources.end();) {
		bool deleted = false;
		for (const auto& ambient : oldAmbients) {
			if (it->GetAmbient() == ambient) {
				it = ambientSources.erase(it);
				deleted = true;
				break;
			}
		}
		if (!deleted) ++it;
	}

	for (auto it = ambients.begin(); it != ambients.end();) {
		bool deleted = false;
		for (const auto& ambient : oldAmbients) {
			if (*it == ambient) {
				// memory freeing is left to the user if needed
				it = ambients.erase(it);
				deleted = true;
				break;
			}
		}
		if (!deleted) ++it;
	}
}


void AmbientMgr::SetAmbients(const std::vector<Ambient*>& a)
{
	std::lock_guard<std::mutex> l(ambientsMutex);
	ambients = a;
	AmbientsSet(ambients);

	Activate();
}

void AmbientMgr::Activate(StringView name)
{
	std::lock_guard<std::recursive_mutex> l(mutex);
	for (const auto& ambient : ambients) {
		if (ambient->GetName() == name) {
			ambient->SetActive();
			break;
		}
	}
	cond.notify_all();
}

void AmbientMgr::Activate()
{
	std::lock_guard<std::recursive_mutex> l(mutex);
	active = true;
	cond.notify_all();
}

void AmbientMgr::Deactivate(StringView name)
{
	std::lock_guard<std::recursive_mutex> l(mutex);
	for (const auto& ambient : ambients) {
		if (ambient->GetName() == name) {
			ambient->SetInactive();
			break;
		}
	}
	cond.notify_all();
}

void AmbientMgr::Deactivate()
{
	std::lock_guard<std::recursive_mutex> l(mutex);
	active = false;
	HardStop();
}

bool AmbientMgr::IsActive(StringView name) const
{
	std::lock_guard<std::mutex> l(ambientsMutex);
	for (const auto& ambient : ambients) {
		if (ambient->GetName() == name) {
			return ambient->GetFlags() & IE_AMBI_ENABLED;
		}
	}
	return false;
}

void AmbientMgr::HardStop()
{
	for (auto& source : ambientSources) {
		source.HardStop();
	}
}

int AmbientMgr::Play()
{
	while (playing) {
		std::unique_lock<std::recursive_mutex> l(mutex);
		tick_t time = GetMilliseconds();
		tick_t delay = Tick(time);
		assert(delay > 0);
		cond.wait_for(l, std::chrono::milliseconds(delay));
	}
	return 0;
}

tick_t AmbientMgr::Tick(tick_t ticks)
{
	tick_t delay = 60000; // wait one minute if all sources are off

	if (!active) {
		return delay;
	}

	auto pos = core->GetAudioDrv()->GetListenerPosition();

	const Game* game = core->GetGame();
	ieDword timeslice = 0;
	if (game) {
		timeslice = SCHEDULE_MASK(game->GameTime);
	}

	std::lock_guard<std::recursive_mutex> l(mutex);
	for (auto& source : ambientSources) {
		tick_t newdelay = source.Tick(ticks, { pos.x, pos.y }, timeslice);
		if (newdelay < delay) delay = newdelay;
	}
	return delay;
}

// AmbientSource implementation
//
tick_t AmbientMgr::AmbientSource::Tick(tick_t ticks, Point listener, ieDword timeslice)
{
	// if we are out of sounds do nothing
	if (ambient->sounds.empty()) {
		return std::numeric_limits<tick_t>::max();
	}

	if (!(ambient->GetFlags() & IE_AMBI_ENABLED) || !(ambient->GetAppearance() & timeslice)) {
		// disabled

		if (source && !source->HasFinishedPlaying()) {
			source->Stop();
		}
		return std::numeric_limits<tick_t>::max();
	}

	tick_t interval = ambient->GetInterval();
	if (lastticks == 0) {
		// initialize
		lastticks = ticks;
		if (interval > 0) {
			nextdelay = ambient->GetTotalInterval();
		}
	}

	if (lastticks + nextdelay > ticks) { // keep waiting
		return lastticks + nextdelay - ticks;
	}

	lastticks = ticks;

	if (ambient->GetFlags() & IE_AMBI_RANDOM) {
		nextref = RAND<size_t>(0, ambient->sounds.size() - 1);
	} else if (++nextref >= ambient->sounds.size()) {
		nextref = 0;
	}

	if (interval > 0) {
		nextdelay = ambient->GetTotalInterval();
	} else {
		// let's wait a second by default if anything goes wrong
		nextdelay = 1000;
	}

	if (!(ambient->GetFlags() & IE_AMBI_MAIN) && !IsHeard(listener)) { // we are out of range
		// release stream if we're inactive for a while
		if (source && !source->HasFinishedPlaying()) {
			source->Stop();
		}
		return nextdelay;
	}

	if (source && !source->HasFinishedPlaying()) {
		return nextdelay;
	}

	auto& settings = core->GetAudioSettings();
	auto volume = settings.GetAmbientVolume();
	auto loop = (ambient->GetFlags() & IE_AMBI_LOOPING) > 0;
	auto ambientGain = ambient->GetTotalGain();

	auto config =
		ambient->GetFlags() & IE_AMBI_MAIN ? settings.ConfigPresetMainAmbient(ambientGain, loop) : settings.ConfigPresetPointAmbient(ambientGain, ambient->GetOrigin(), ambient->radius, loop);
	gain = config.channelVolume;

	if (!source) {
		// we need to allocate a stream
		source = core->GetAudioDrv()->CreatePlaybackSource(config);

		if (!source) {
			// Try again later
			return nextdelay;
		}
	} else if (ambient->gainVariance != 0) {
		SetVolume(volume);
	}
	if (ambient->pitchVariance != 0) {
		source->SetPitch(ambient->GetTotalPitch());
	}

	tick_t length = Enqueue(config);

	if (interval == 0) { // continuous ambient
		nextdelay = length;
	}

	return nextdelay;
}

BufferCacheEntry AmbientMgr::AmbientSource::GetBuffer(ResRef resource, const AudioPlaybackConfig& config)
{
	auto entry = bufferCache.get().Lookup(resource);
	if (entry) {
		return *entry;
	}

	ResourceHolder<SoundMgr> acm = gamedata->GetResourceHolder<SoundMgr>(resource);
	if (!acm) {
		return {};
	}

	auto length = acm->GetLengthMs();
	auto handle = core->GetAudioDrv()->LoadSound(std::move(acm), config);
	if (!handle) {
		return {};
	}

	bufferCache.get().SetAt(resource, std::move(handle), length);

	return *bufferCache.get().Lookup(resource);
}

// enqueues a random sound and returns its length
tick_t AmbientMgr::AmbientSource::Enqueue(const AudioPlaybackConfig& config)
{
	auto cacheEntry = GetBuffer(ambient->sounds[nextref], config);
	if (!cacheEntry.handle) {
		return -1;
	}

	source->Enqueue(cacheEntry.handle);

	return cacheEntry.length;
}

bool AmbientMgr::AmbientSource::IsHeard(const Point& listener) const
{
	return Distance(listener, ambient->GetOrigin()) <= ambient->GetRadius();
}

void AmbientMgr::AmbientSource::HardStop()
{
	if (source) {
		source->Stop();
		source.reset();
	}
}

void AmbientMgr::AmbientSource::SetVolume(unsigned short volume)
{
	if (source) {
		source->SetVolume(volume * gain / 100);
	}
}

}
