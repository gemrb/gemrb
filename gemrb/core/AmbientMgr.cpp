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
#include "Audio.h"
#include "Game.h"
#include "Interface.h"
#include "RNG.h"

#include <cassert>
#include <chrono>
#include <climits>

namespace GemRB {

AmbientMgr::AmbientMgr()
{
	player = std::thread(&AmbientMgr::Play, this);
}

AmbientMgr::~AmbientMgr()
{
	playing = false;
	mutex.lock();
	for (auto ambientSource : ambientSources) {
		delete ambientSource;
	}
	ambientSources.clear();
	mutex.unlock();
	Reset();

	cond.notify_all();
	player.join();
}

void AmbientMgr::Reset()
{
	std::lock_guard<std::mutex> l(ambientsMutex);
	ambients.clear();
	AmbientsSet(ambients);
}

void AmbientMgr::UpdateVolume(unsigned short volume)
{
	std::lock_guard<std::recursive_mutex> l(mutex);
	for (const auto& source : ambientSources) {
		source->SetVolume(volume);
	}
}

void AmbientMgr::AmbientsSet(const std::vector<Ambient*>& a)
{
	std::lock_guard<std::recursive_mutex> l(mutex);
	for (auto ambientSource : ambientSources) {
		delete ambientSource;
	}
	ambientSources.resize(0);
	for (auto& source : a) {
		ambientSources.push_back(new AmbientSource(source));
	}
}

void AmbientMgr::RemoveAmbients(const std::vector<Ambient*> &oldAmbients)
{
	std::lock_guard<std::recursive_mutex> l(mutex);
	// manually deleting ambientSources as regenerating them causes a several second pause
	for (auto it = ambientSources.begin(); it != ambientSources.end(); ) {
		auto ambientSource = *it;
		bool deleted = false;
		for (const auto& ambient : oldAmbients) {
			if (ambientSource->GetAmbient() == ambient) {
				delete ambientSource;
				it = ambientSources.erase(it);
				deleted = true;
				break;
			}
		}
		if (!deleted) ++it;
	}

	for (auto it = ambients.begin(); it != ambients.end(); ) {
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


void AmbientMgr::SetAmbients(const std::vector<Ambient*> &a)
{
	std::lock_guard<std::mutex> l(ambientsMutex);
	ambients = a;
	AmbientsSet(ambients);

	core->GetAudioDrv()->UpdateVolume(GEM_SND_VOL_AMBIENTS);
	Activate();
}

void AmbientMgr::Activate(StringView name)
{
	std::lock_guard<std::recursive_mutex> l(mutex);
	//std::lock_guard<std::mutex> l(ambientsMutex);
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
	//std::lock_guard<std::mutex> l(ambientsMutex);
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

void AmbientMgr::HardStop() const
{
	for (auto source : ambientSources) {
		source->HardStop();
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

tick_t AmbientMgr::Tick(tick_t ticks) const
{
	tick_t delay = 60000; // wait one minute if all sources are off

	if (!active) {
		return delay;
	}

	Point listener = core->GetAudioDrv()->GetListenerPos();

	const Game* game = core->GetGame();
	ieDword timeslice = 0;
	if (game) {
		timeslice = SCHEDULE_MASK(game->GameTime);
	}

	std::lock_guard<std::recursive_mutex> l(mutex);
	for (auto source : ambientSources) {
		tick_t newdelay = source->Tick(ticks, listener, timeslice);
		if (newdelay < delay) delay = newdelay;
	}
	return delay;
}

// AmbientSource implementation
//
AmbientMgr::AmbientSource::~AmbientSource()
{
	if (stream >= 0) {
		core->GetAudioDrv()->ReleaseStream(stream, true);
		stream = -1;
	}
}

tick_t AmbientMgr::AmbientSource::Tick(tick_t ticks, Point listener, ieDword timeslice)
{
	// if we are out of sounds do nothing
	if (ambient->sounds.empty()) {
		return std::numeric_limits<tick_t>::max();
	}

	if (!(ambient->GetFlags() & IE_AMBI_ENABLED) || !(ambient->GetAppearance() & timeslice)) {
		// disabled

		if (stream >= 0) {
			// release the stream without immediately stopping it
			core->GetAudioDrv()->ReleaseStream(stream, false);
			stream = -1;
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
		core->GetAudioDrv()->ReleaseStream(stream);
		stream = -1;
		return nextdelay;
	}

	unsigned int channel = ambient->GetFlags() & IE_AMBI_LOOPING ? (ambient->GetFlags() & IE_AMBI_MAIN ? SFX_CHAN_AREA_AMB : SFX_CHAN_AMB_LOOP) : SFX_CHAN_AMB_OTHER;
	totalgain = ambient->GetTotalGain() * core->GetAudioDrv()->GetVolume(channel) / 100;

	unsigned int v = core->GetVariable("Volume Ambients", 100);

	if (stream < 0) {
		// we need to allocate a stream
		stream = core->GetAudioDrv()->SetupNewStream(ambient->GetOrigin().x, ambient->GetOrigin().y, 0, v * totalgain / 100, !(ambient->GetFlags() & IE_AMBI_MAIN), ambient->radius);

		if (stream == -1) {
			// no streams available...
			// Try again later
			return nextdelay;
		}
	} else if (ambient->gainVariance != 0) {
		SetVolume(v);
	}
	if (ambient->pitchVariance != 0) {
		core->GetAudioDrv()->SetAmbientStreamPitch(stream, ambient->GetTotalPitch());
	}

	tick_t length = Enqueue();

	if (interval == 0) { // continuous ambient
		nextdelay = length;
	}

	return nextdelay;
}

// enqueues a random sound and returns its length
tick_t AmbientMgr::AmbientSource::Enqueue() const
{
	if (stream < 0) return -1;
	// print("Playing ambient %s, %d/%ld on stream %d", ambient->sounds[nextref], nextref, ambient->sounds.size(), stream);
	return core->GetAudioDrv()->QueueAmbient(stream, ambient->sounds[nextref]);
}

bool AmbientMgr::AmbientSource::IsHeard(const Point &listener) const
{
	return Distance(listener, ambient->GetOrigin()) <= ambient->GetRadius();
}

void AmbientMgr::AmbientSource::HardStop()
{
	if (stream >= 0) {
		core->GetAudioDrv()->ReleaseStream(stream, true);
		stream = -1;
	}
}

// sets the overall volume (in percent)
// the final volume is affected by the specific ambient gain
void AmbientMgr::AmbientSource::SetVolume(unsigned short vol) const
{
	if (stream >= 0) {
		int v = vol * totalgain / 100;
		core->GetAudioDrv()->SetAmbientStreamVolume(stream, v);
	}
}

}
