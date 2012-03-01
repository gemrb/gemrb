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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#ifndef AMBIENTMGRAL_H
#define AMBIENTMGRAL_H

#include "AmbientMgr.h"

#include <vector>
#include <string>

#include <SDL_thread.h>

#ifndef WIN32
#ifdef __APPLE_CC__
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#else
#include <AL/al.h>
#include <AL/alc.h>
#endif 
#else
#include <al.h>
#include <alc.h>
#endif

namespace GemRB {

class Ambient;

class AmbientMgrAL : public AmbientMgr {
public:
	AmbientMgrAL() : AmbientMgr(), mutex(SDL_CreateMutex()), 
			player(NULL), cond(SDL_CreateCond()) { }
	~AmbientMgrAL() { reset(); SDL_DestroyMutex(mutex); SDL_DestroyCond(cond); }
	void reset();
	void setAmbients(const std::vector<Ambient *> &a);
	void activate(const std::string &name);
	void activate();
	void deactivate(const std::string &name);
	void deactivate();
	void UpdateVolume(unsigned short value);
private:
	class AmbientSource {
	public:
		AmbientSource(const Ambient *a);
		~AmbientSource();
		unsigned int tick(unsigned int ticks, Point listener, ieDword timeslice);
		void hardStop();
		void SetVolume(unsigned short volume);
	private:
		int stream;
		std::vector<const char*> soundrefs;
		const Ambient* ambient;
		unsigned int lastticks;
		int enqueued;
		bool loaded;

		void ensureLoaded();
		bool isHeard(const Point &listener) const;
		int enqueue();
	};
	std::vector<AmbientSource *> ambientSources;
	
	static int play(void *am);
	unsigned int tick(unsigned int ticks);
	void hardStop();
	
	SDL_mutex *mutex;
	SDL_Thread *player;
	SDL_cond *cond;
};

}

#endif /* AMBIENTMGRAL_H */
