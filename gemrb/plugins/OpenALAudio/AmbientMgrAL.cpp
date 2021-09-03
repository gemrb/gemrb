/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2004 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#include "AmbientMgrAL.h"

#include "Ambient.h"
#include "Game.h"
#include "RNG.h"
#include "Interface.h"

#include <cassert>
#include <chrono>
#include <climits>
#include <cstdio>

using namespace GemRB;

// TODO: no more dependency on OpenAL, rename and move it?

AmbientMgrAL::AmbientMgrAL()
: AmbientMgr()
{
	player = std::thread(&AmbientMgrAL::play, this);
}

AmbientMgrAL::~AmbientMgrAL()
{
	playing = false;
	mutex.lock();
	for (auto ambientSource : ambientSources) {
		delete ambientSource;
	}
	ambientSources.clear();
	AmbientMgr::reset();
	mutex.unlock();
	
	cond.notify_all();
	player.join();
}

void AmbientMgrAL::ambientsSet(const std::vector<Ambient *>& a)
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

void AmbientMgrAL::activate(const std::string &name)
{
	std::lock_guard<std::recursive_mutex> l(mutex);
	AmbientMgr::activate(name);
	cond.notify_all();
}

void AmbientMgrAL::activate()
{
	std::lock_guard<std::recursive_mutex> l(mutex);
	AmbientMgr::activate();
	cond.notify_all();
}

void AmbientMgrAL::deactivate(const std::string &name)
{
	std::lock_guard<std::recursive_mutex> l(mutex);
	AmbientMgr::deactivate(name);
	cond.notify_all();
}

void AmbientMgrAL::deactivate()
{
	std::lock_guard<std::recursive_mutex> l(mutex);
	AmbientMgr::deactivate();
	hardStop();
}

void AmbientMgrAL::hardStop() const
{
	for (auto source : ambientSources) {
		source->hardStop();
	}
}

int AmbientMgrAL::play()
{
	while (playing) {
		std::unique_lock<std::recursive_mutex> l(mutex);
		tick_t time = GetTicks();
		tick_t delay = tick(time);
		assert(delay > 0);
		cond.wait_for(l, std::chrono::milliseconds(delay));
	}
	return 0;
}

tick_t AmbientMgrAL::tick(tick_t ticks) const
{
	tick_t delay = 60000; // wait one minute if all sources are off

	if (!active)
		return delay;

	Point listener = core->GetAudioDrv()->GetListenerPos();

	const Game* game = core->GetGame();
	ieDword timeslice = 0;
	if (game) {
		timeslice = SCHEDULE_MASK(game->GameTime);
	}

	std::lock_guard<std::recursive_mutex> l(mutex);
	for (auto source : ambientSources) {
		tick_t newdelay = source->tick(ticks, listener, timeslice);
		if (newdelay < delay) delay = newdelay;
	}
	return delay;
}

void AmbientMgrAL::UpdateVolume(unsigned short volume)
{
	std::lock_guard<std::recursive_mutex> l(mutex);
	for (auto source : ambientSources) {
		source->SetVolume(volume);
	}
}


AmbientMgrAL::AmbientSource::AmbientSource(const Ambient *a)
: stream(-1), ambient(a), lastticks(0), nextdelay(0), nextref(0), totalgain(0)
{
}

AmbientMgrAL::AmbientSource::~AmbientSource()
{
	if (stream >= 0) {
		core->GetAudioDrv()->ReleaseStream(stream, true);
		stream = -1;
	}
}

tick_t AmbientMgrAL::AmbientSource::tick(tick_t ticks, Point listener, ieDword timeslice)
{
	/* if we are out of sounds do nothing */
	if (ambient->sounds.empty()) {
		return std::numeric_limits<tick_t>::max();
	}

	if (!(ambient->getFlags() & IE_AMBI_ENABLED) || !(ambient->getAppearance() & timeslice)) {
		// disabled

		if (stream >= 0) {
			// release the stream without immediately stopping it
			core->GetAudioDrv()->ReleaseStream(stream, false);
			stream = -1;
		}
		return std::numeric_limits<tick_t>::max();
	}

	tick_t interval = ambient->getInterval();
	if (lastticks == 0) {
		// initialize
		lastticks = ticks;
		if (interval > 0) {
			nextdelay = ambient->getTotalInterval();
		}
	}

	if (lastticks + nextdelay > ticks) {	// keep waiting
		return lastticks + nextdelay - ticks;
	}

	lastticks = ticks;

	if (ambient->getFlags() & IE_AMBI_RANDOM) {
		nextref = RAND<size_t>(0, ambient->sounds.size() - 1);
	} else if (++nextref >= ambient->sounds.size()) {
		nextref = 0;
	}

	if (interval > 0) {
		nextdelay = ambient->getTotalInterval();
	} else {
		// let's wait a second by default if anything goes wrong
		nextdelay = 1000;
	}

	if (!(ambient->getFlags() & IE_AMBI_MAIN) && !isHeard(listener)) { // we are out of range
		// release stream if we're inactive for a while
		core->GetAudioDrv()->ReleaseStream(stream);
		stream = -1;
		return nextdelay;
	}

	unsigned int channel = ambient->getFlags() & IE_AMBI_LOOPING ? (ambient->getFlags() & IE_AMBI_MAIN ? SFX_CHAN_AREA_AMB : SFX_CHAN_AMB_LOOP) : SFX_CHAN_AMB_OTHER;
	totalgain = ambient->getTotalGain() * core->GetAudioDrv()->GetVolume(channel) / 100;

	unsigned int v = 100;
	core->GetDictionary()->Lookup("Volume Ambients", v);

	if (stream < 0) {
		// we need to allocate a stream
		stream = core->GetAudioDrv()->SetupNewStream(ambient->getOrigin().x, ambient->getOrigin().y, 0, v * totalgain / 100, !(ambient->getFlags() & IE_AMBI_MAIN), ambient->radius);

		if (stream == -1) {
			// no streams available...
			// Try again later
			return nextdelay;
		}
	} else if (ambient->gainVariance != 0) {
		SetVolume(v);
	}
	if (ambient->pitchVariance != 0) {
		core->GetAudioDrv()->SetAmbientStreamPitch(stream, ambient->getTotalPitch());
	}

	tick_t length = enqueue();

	if (interval == 0) { // continuous ambient
		nextdelay = length;
	}

	return nextdelay;
}

/* enqueues a random sound and returns its length */
tick_t AmbientMgrAL::AmbientSource::enqueue() const
{
	if (stream < 0) return -1;
	// print("Playing ambient %s, %d/%ld on stream %d", ambient->sounds[nextref], nextref, ambient->sounds.size(), stream);
	return core->GetAudioDrv()->QueueAmbient(stream, ambient->sounds[nextref]);
}

bool AmbientMgrAL::AmbientSource::isHeard(const Point &listener) const
{
	return Distance(listener, ambient->getOrigin()) <= ambient->getRadius();
}

void AmbientMgrAL::AmbientSource::hardStop()
{
	if (stream >= 0) {
		core->GetAudioDrv()->ReleaseStream(stream, true);
		stream = -1;
	}
}

/* sets the overall volume (in percent)
 * the final volume is affected by the specific ambient gain
 */
void AmbientMgrAL::AmbientSource::SetVolume(unsigned short vol) const
{
	if (stream >= 0) {
		int v = vol * totalgain / 100;
		core->GetAudioDrv()->SetAmbientStreamVolume(stream, v);
	}
}
