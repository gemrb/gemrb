/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2004 The GemRB Project
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/ACMImporter/AmbientMgrAL.cpp,v 1.1 2004/08/29 01:19:01 divide Exp $
 *
 */

#include <limits.h>
#include <cmath>
#include <cassert>
#include "../Core/Ambient.h"
#include "../Core/Interface.h"
#include "ACMImp.h"
#include "AmbientMgrAL.h"

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
	core->GetSoundMgr()->UpdateVolume( GEM_SND_VOL_AMBIENTS );
	
	player = SDL_CreateThread(&play, (void *) this);
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
	
	ALfloat listen[3];
	alGetListenerfv( AL_POSITION, listen );
	Point listener;
	listener.x = (short) listen[0];
	listener.y = (short) listen[1];
	
	unsigned int timeslice = ((core->GetGame()->GameTime / 60 + 30) / 60 - 1) % 24;
	
	for (std::vector<AmbientSource *>::iterator it = ambientSources.begin(); it != ambientSources.end(); ++it) {
		unsigned int newdelay = (*it)->tick(ticks, listener, timeslice);
		if (newdelay < delay)
			delay = newdelay;
	}
	return delay;
}

AmbientMgrAL::AmbientSource::AmbientSource(Ambient *a)
: ambient(a), lastticks(0), enqueued(0)
{
	alGenSources( 1, &source );
	
	ALfloat position[] = { (float) a->getOrigin().x, (float) a->getOrigin().y, (float) a->getHeight() };
	alSourcefv( source, AL_POSITION, position );
	alSourcef( source, AL_GAIN, 0.01f * a->getGain() );
	alSourcei( source, AL_REFERENCE_DISTANCE, REFERENCE_DISTANCE );
	alSourcei( source, AL_ROLLOFF_FACTOR, (a->getFlags() & IE_AMBI_POINT) ? 1 : 0 );
	
/*	ALint state, queued, processed;
	alGetSourcei( source, AL_SOURCE_STATE, &state );
	alGetSourcei( source, AL_BUFFERS_QUEUED, &queued );
	alGetSourcei( source, AL_BUFFERS_PROCESSED, &processed );
	printf("ambient %s: source %x, state %x, queued %d, processed %d\n", ambient->getName().c_str(), source, state, queued, processed);
	if (!alIsSource( source )) printf("hey, it's not a source!\n");*/
	
	// preload sounds
	unsigned int i=a->sounds.size();
	buffers.reserve(i);
	buflens.reserve(i);
	while(i--) {
		int timelen;
		ALuint buffer = ACMImp::LoadSound(a->sounds[i], &timelen);
		if (!buffer) {
			printf("Invalid SoundResRef: %.8s, Dequeueing...\n",a->sounds[i]);
			free(a->sounds[i]);
		        a->sounds.erase(a->sounds.begin() + i);
		} else {
			buffers.push_back(buffer);
			buflens.push_back(timelen);
		}
		
	}
/*	
	// use OpenAL to loop in this special case so we don't have to
	if ((buffers.size() == 1) && (a->getInterval() == 0)) {
		alSourcei( source, AL_LOOPING, 1 );
		alSourcei( source, AL_BUFFER, buffers[0] );
	}*/
}

AmbientMgrAL::AmbientSource::~AmbientSource()
{
	alSourceStop( source );	// legal nop if not playing
//	printf("deleting source %x\n", source);
	alDeleteSources( 1, &source );
	for (std::vector<ALuint>::iterator it = buffers.begin(); it != buffers.end(); ++it) {
		alDeleteBuffers( 1, &(*it) );
	}
}

unsigned int AmbientMgrAL::AmbientSource::tick(unsigned int ticks, Point listener, unsigned int timeslice)
{
	ALint state;
/*	ALint queued, processed;
	alGetSourcei( source, AL_SOURCE_STATE, &state );
	alGetSourcei( source, AL_BUFFERS_QUEUED, &queued );
	alGetSourcei( source, AL_BUFFERS_PROCESSED, &processed );
	printf("ambient %s: source %x, state %x, queued %d, processed %d\n", ambient->getName().c_str(), source, state, queued, processed);
	if (!alIsSource( source )) printf("hey, it's not a source!\n");*/
	
	if ((! (ambient->getFlags() & IE_AMBI_ENABLED)) || (! ambient->getAppearance()&(1<<timeslice))) {
		// don't really stop the source, since we don't want to stop playing abruptly in the middle of
		// a sample (do we?), and it would end playing by itself in a while (Divide)
		//this is correct (Avenger)
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
		return delay;
	}
	
	dequeProcessed();
	
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
		--leftNum;
		leftMS -= len;
		enqueued += len;
	}
	
	// oh, and don't forget to push play
	alGetSourcei( source, AL_SOURCE_STATE, &state );
	if (AL_PLAYING != state) { // play on playing source would rewind it
		alSourcePlay( source );
	}
	
	return delay;
}

/* dequeues already processed buffers */
void AmbientMgrAL::AmbientSource::dequeProcessed()
{
	ALint processed;
	alGetSourcei( source, AL_BUFFERS_PROCESSED, &processed );
	if (0 == processed) return;
	ALuint * buffers = (ALuint *) malloc ( processed * sizeof(ALuint) );
	alSourceUnqueueBuffers( source, processed, buffers );
	free(buffers);
	// do not destroy buffers since we reuse them
}

/* enqueues a random sound and returns its length */
unsigned int AmbientMgrAL::AmbientSource::enqueue()
{
	int index = rand() % buffers.size();
	/* yeah, yeah, I know what rand(3) says... but we don't need much randomness here and this is fast
	 * (fast to write, also ;-)
	 */
	
	alSourceQueueBuffers( source, 1, &(buffers[index]) );
	
	return buflens[index];
}

bool AmbientMgrAL::AmbientSource::isHeard(const Point &listener) const
{
	float xdist = listener.x - ambient->getOrigin().x;
	float ydist = listener.y - ambient->getOrigin().y;
	float dist = sqrt(xdist * xdist + ydist * ydist);
	return dist < ambient->getRadius();
}

void AmbientMgrAL::AmbientSource::hardStop()
{
	alSourceStop( source );
	dequeProcessed();
}

void AmbientMgrAL::UpdateVolume(unsigned short volume)
{
	SDL_mutexP( mutex );
	for (std::vector<AmbientSource *>::iterator it = ambientSources.begin(); it != ambientSources.end(); ++it) {
		(*it) -> SetVolume( volume );
	}
	SDL_mutexV( mutex );
}

/* sets the overall volume (in percent)
 * the final volume is affected by the specific ambient gain
 */
void AmbientMgrAL::AmbientSource::SetVolume(unsigned short volume)
{
	alSourcef( source, AL_GAIN, 0.0001f * ambient->getGain() * volume );
}

