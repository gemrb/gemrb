/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003-2004 The GemRB Project
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
 * $Id$
 *
 */

#ifndef ACMIMP_H
#define ACMIMP_H

#include "../Core/SoundMgr.h"
#include "../Core/FileStream.h"

class AmbientMgrAL;

#ifndef WIN32
#include <AL/al.h>
#include <AL/alc.h>
#else
#include <al.h>
#include <alc.h>
#endif

#include "readers.h"

#define MAX_STREAMS  30

// the distance at which sound is played at full volume
#define REFERENCE_DISTANCE 50

class ACMImp : public SoundMgr {
private:
	void clearstreams();
	static int PlayListManager(void* data);
public:
	ACMImp(void);
	~ACMImp(void);
	bool Init(void);
	unsigned int Play(const char* ResRef, int XPos = 0, int YPos = 0, unsigned int flags = GEM_SND_RELATIVE);
	unsigned int StreamFile(const char* filename);
	bool Play();
	bool Stop();
	bool CanPlay();
	bool IsSpeaking();
	void ResetMusics();
	void UpdateViewportPos(int XPos, int YPos);
	void UpdateVolume( unsigned int which = GEM_SND_VOL_MUSIC | GEM_SND_VOL_AMBIENTS );
	static ALuint LoadSound(const char *sound, int *time_length = NULL);
	
	void release(void)
	{
		delete this;
	}

private:
	int num_streams;
};

#endif
