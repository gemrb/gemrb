/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/ACMImporter/ACMImp.h,v 1.22 2004/08/09 13:02:07 divide Exp $
 *
 */

#ifndef ACMIMP_H
#define ACMIMP_H

#include "../Core/SoundMgr.h"
#include "../Core/FileStream.h"

#include <SDL.h>
#include <SDL_thread.h>
#include <vector>

class Ambient;

#ifndef WIN32
#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alu.h>
#include <AL/alut.h>
#else
#include <al.h>
#include <alc.h>
#include <alu.h>
#include <alut.h>
#endif

#include "readers.h"

#define MAX_STREAMS  30

typedef struct AudioStream {
	ALuint Buffer;
	ALuint Source;
	int Duration;
	bool free;
	bool playing;
	CSoundReader* reader;
} AudioStream;

class ACMImp : public SoundMgr {
private:
	void clearstreams(bool free);
	static int PlayListManager(void* data);
	static ALuint LoadSound(const char *sound, int *time_length = NULL);
public:
	ACMImp(void);
	~ACMImp(void);
	bool Init(void);
	unsigned long Play(const char* ResRef, int XPos = 0, int YPos = 0, unsigned long flags = GEM_SND_RELATIVE);
	unsigned long StreamFile(const char* filename);
	bool Play();
	bool Stop();
	void ResetMusics();
	void UpdateViewportPos(int XPos, int YPos);
public:
	void release(void)
	{
		delete this;
	}
	
	class AmbientMgr : public SoundMgr::AmbientMgr {
	public:
		AmbientMgr() : SoundMgr::AmbientMgr(), mutex(SDL_CreateMutex()), 
		               player(NULL), cond(SDL_CreateCond()) { }
		~AmbientMgr() { reset(); SDL_DestroyMutex(mutex); SDL_DestroyCond(cond); }
		void reset();
		void setAmbients(const std::vector<Ambient *> &a);
		void activate(const std::string &name);
		void deactivate(const std::string &name);
	private:
		class AmbientSource {
		public:
			AmbientSource(const Ambient *a);
			~AmbientSource();
			unsigned int tick(unsigned int ticks, Point listener, unsigned int timeslice);
		private:
			ALuint source;
			std::vector<ALuint> buffers;
			std::vector<unsigned int> buflens;
			const Ambient * ambient;
			unsigned int lastticks;
			int enqueued;
			bool isHeard(const Point &listener) const;
			unsigned int enqueue();
			void dequeProcessed();
		};
		std::vector<AmbientSource *> ambientSources;
		
		static int play(void *am);
		unsigned int tick(unsigned int ticks);
		
		SDL_mutex *mutex;
		SDL_Thread *player;
		SDL_cond *cond;
	};
};

#endif
