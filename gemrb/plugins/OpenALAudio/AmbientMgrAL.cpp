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

#include "OpenALAudio.h"

#include "Ambient.h"
#include "Game.h"
#include "Interface.h"

#include <cassert>
#include <climits>
#include <cmath>
#include <cstdio>

#include <SDL.h>

using namespace GemRB;

// TODO: remove last dependencies on OpenAL, and then rename and move it?

// legal nop if already reset
void AmbientMgrAL::reset()
{
	if (NULL != player){
		SDL_mutexP(mutex);
	}
	for (std::vector<AmbientSource *>::iterator it = ambientSources.begin(); it != ambientSources.end(); ++it) {
		delete (*it);
	}
	ambientSources.clear();
	AmbientMgr::reset();
	if (NULL != player) {
		SDL_CondSignal(cond);
		SDL_mutexV(mutex);
		SDL_WaitThread(player, NULL);
		player = NULL;
	}
}

void AmbientMgrAL::setAmbients(const std::vector<Ambient *> &a)
{
	AmbientMgr::setAmbients(a);
	assert(NULL == player);

	ambientSources.reserve(a.size());
	for (std::vector<Ambient *>::const_iterator it = a.begin(); it != a.end(); ++it) {
		ambientSources.push_back(new AmbientSource(*it));
	}
	core->GetAudioDrv()->UpdateVolume( GEM_SND_VOL_AMBIENTS );

#if	SDL_VERSION_ATLEAST(1, 3, 0)
	/* as of changeset 3a041d215edc SDL_CreateThread has a 'name' parameter */
	player = SDL_CreateThread(&play, "AmbientMgrAL", (void *) this);
#else
	player = SDL_CreateThread(&play, (void *) this);
#endif
}

void AmbientMgrAL::activate(const std::string &name)
{
	if (NULL != player)
		SDL_mutexP(mutex);
	AmbientMgr::activate(name);
	if (NULL != player) {
		SDL_CondSignal(cond);
		SDL_mutexV(mutex);
	}
}

void AmbientMgrAL::activate()
{
	if (NULL != player)
		SDL_mutexP(mutex);
	AmbientMgr::activate();
	if (NULL != player) {
		SDL_CondSignal(cond);
		SDL_mutexV(mutex);
	}
}

void AmbientMgrAL::deactivate(const std::string &name)
{
	if (NULL != player)
		SDL_mutexP(mutex);
	AmbientMgr::deactivate(name);
	if (NULL != player) {
		SDL_CondSignal(cond);
		SDL_mutexV(mutex);
	}
}

void AmbientMgrAL::deactivate()
{
	if (NULL != player)
		SDL_mutexP(mutex);
	AmbientMgr::deactivate();
	hardStop();
	if (NULL != player)
		SDL_mutexV(mutex);
}

void AmbientMgrAL::hardStop()
{
	for (std::vector<AmbientSource *>::iterator it = ambientSources.begin(); it != ambientSources.end(); ++it) {
		(*it)->hardStop();
	}
}

int AmbientMgrAL::play(void *am)
{
	AmbientMgrAL * ambim = (AmbientMgrAL *) am;
	SDL_mutexP(ambim->mutex);
	while (0 != ambim->ambientSources.size()) {
		if (NULL == core->GetGame()) { // we don't have any game, and we need one
			break;
		}
		unsigned int delay = ambim->tick(SDL_GetTicks());
		assert(delay > 0);
		SDL_CondWaitTimeout(ambim->cond, ambim->mutex, delay);
	}
	SDL_mutexV(ambim->mutex);
	return 0;
}

unsigned int AmbientMgrAL::tick(unsigned int ticks)
{
	unsigned int delay = 60000; // wait one minute if all sources are off

	if (!active)
		return delay;

	int xpos, ypos;
	core->GetAudioDrv()->GetListenerPos(xpos, ypos);
	Point listener;
	listener.x = (short) xpos;
	listener.y = (short) ypos;

	ieDword timeslice = 1<<(((core->GetGame()->GameTime / 60 + 30) / 60 - 1) % 24);

	for (std::vector<AmbientSource *>::iterator it = ambientSources.begin(); it != ambientSources.end(); ++it) {
		unsigned int newdelay = (*it)->tick(ticks, listener, timeslice);
		if (newdelay < delay)
			delay = newdelay;
	}
	return delay;
}

void AmbientMgrAL::UpdateVolume(unsigned short volume)
{
	SDL_mutexP( mutex );
	for (std::vector<AmbientSource *>::iterator it = ambientSources.begin(); it != ambientSources.end(); ++it) {
		(*it) -> SetVolume( volume );
	}
	SDL_mutexV( mutex );
}


AmbientMgrAL::AmbientSource::AmbientSource(const Ambient *a)
: stream(-1), ambient(a), lastticks(0), enqueued(0), loaded(false)
{
	// TODO: wait random amount of time before beginning?
}

void AmbientMgrAL::AmbientSource::ensureLoaded()
{
	// TODO: implement this after caching in ACMImp is done
	if (loaded) return;

	unsigned int i=ambient->sounds.size();
	soundrefs.reserve(i);
	while(i--) {
		// TODO: cache this sound
		// (and skip it if it turns out to be invalid)
		soundrefs.push_back(ambient->sounds[i]);
	}

	loaded = true;
}

AmbientMgrAL::AmbientSource::~AmbientSource()
{
	if (stream >= 0) {
		core->GetAudioDrv()->ReleaseStream(stream, true);
		stream = -1;
	}
}

unsigned int AmbientMgrAL::AmbientSource::tick(unsigned int ticks, Point listener, ieDword timeslice)
{
	/* if we are out of sounds do nothing */
	if(!ambient->sounds.size()) {
		return UINT_MAX;
	}
	if (loaded && soundrefs.empty()) return UINT_MAX;

	if ((! (ambient->getFlags() & IE_AMBI_ENABLED)) || (! ambient->getAppearance()&timeslice)) {
		// disabled

		if (stream >= 0) {
			// release the stream without immediately stopping it
			core->GetAudioDrv()->ReleaseStream(stream, false);
			stream = -1;
		}
		return UINT_MAX;
	}

	int delay = ambient->getInterval() * 1000;
	int left = lastticks - ticks + delay;
	if (0 < left) // we are still waiting
		return left;
	if (enqueued > 0) // we have already played that much
		enqueued += left;
	if (enqueued < 0)
		enqueued = 0;

	lastticks = ticks;
	if (0 == delay) // it's a non-stop ambient, so in any case wait only a sec
		delay = 1000;

	if (! (ambient->getFlags() & IE_AMBI_MAIN) && !isHeard( listener )) { // we are out of range
		if (delay > 500) {
			// release stream if we're inactive for a while
			core->GetAudioDrv()->ReleaseStream(stream);
			stream = -1;
		}
		return delay;
	}

	ensureLoaded();
	if (soundrefs.empty()) return UINT_MAX;

	if (stream < 0) {
		// we need to allocate a stream
		unsigned int v = 100;

		core->GetDictionary()->Lookup("Volume Ambients", v);
		v *= ambient->getGain();
		stream = core->GetAudioDrv()->SetupNewStream(ambient->getOrigin().x, ambient->getOrigin().y, ambient->getHeight(), v/100, (ambient->getFlags() & IE_AMBI_POINT), true);

		if (stream == -1) {
			// no streams available...
			// Try again later
			return delay;
		}
	}


	/* it seems that the following (commented out) is not the purpose of the perset field, as
	it leads to ambients playing non-stop and queues overfilled */
/*	int leftNum = ambient -> getPerset(); */
	int leftNum = 1;
	int leftMS = 0;
	if (0 == ambient->getInterval()) {
		leftNum = 0;
		leftMS = 1000 - enqueued; // let's have at least 1 second worth queue
	}


	while (0 < leftNum || 0 < leftMS) {
		int len = enqueue();
		if (len < 0) break;
		--leftNum;
		leftMS -= len;
		enqueued += len;
	}

	return delay;
}

/* enqueues a random sound and returns its length */
int AmbientMgrAL::AmbientSource::enqueue()
{
	if (soundrefs.empty()) return -1;
	if (stream < 0) return -1;
	int index = rand() % soundrefs.size();
	//print("Playing ambient %p, %s, %d/%ld on stream %d",(void*)this, soundrefs[index], index, soundrefs.size(), stream);
	return core->GetAudioDrv()->QueueAmbient(stream, soundrefs[index]);
}

bool AmbientMgrAL::AmbientSource::isHeard(const Point &listener) const
{
	int xdist =listener.x - ambient->getOrigin().x;
	int ydist =listener.y - ambient->getOrigin().y;
	int dist = (int) sqrt( (double) (xdist * xdist + ydist * ydist) );
	return dist < ambient->getRadius();
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
void AmbientMgrAL::AmbientSource::SetVolume(unsigned short volume)
{
	if (stream >= 0) {
		int v = volume;
		v *= ambient->getGain();
		v /= 100;
		core->GetAudioDrv()->SetAmbientStreamVolume(stream, v);
	}
}
