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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/ACMImporter/ACMImp.h,v 1.9 2003/12/02 14:56:22 balrog994 Exp $
 *
 */

#ifndef ACMIMP_H
#define ACMIMP_H

#include "../Core/SoundMgr.h"
#include "../Core/FileStream.h"

#include "../../includes/sdl/SDL.h"
#include "../../includes/sdl/SDL_thread.h"

#include "al.h"
#include "alc.h"
#include "alu.h"
#include "alut.h"

#define MAX_STREAMS  30

/*
typedef struct AudioStream {
	FSOUND_STREAM * stream;
	FSOUND_DSPUNIT * dsp;
	bool playing;
	bool end;
	bool free;
	int channel;
} AudioStream;
*/

static ALfloat ListenerPos[3];
static ALfloat ListenerVel[3];
static ALfloat ListenerOri[6];

typedef struct AudioStream {
	ALuint Buffer;
	ALuint Source;
	int Duration;
	bool free;
	bool playing;
} AudioStream;

class ACMImp : public SoundMgr
{
private:
	void clearstreams(bool free);
	/*
	static signed char __stdcall endstreamcallback(FSOUND_STREAM *stream, void *buff, int len, int param);
	static signed char __stdcall synchstreamcallback(FSOUND_STREAM *stream, void *buff, int len, int param);
	static void * __stdcall dspcallback(void *originalbuffer, void *newbuffer, int length, int param);
	*/
	static int PlayListManager(void * data);
public:
	ACMImp(void);
	~ACMImp(void);
	bool Init(void);
	unsigned long Play(const char * ResRef);
	bool AcmToWav(DataStream *inFile, const char * tmpFile, const char * outFile);
	unsigned long LoadFile(const char * filename);
	bool Play(unsigned long index);
	bool Stop(unsigned long index);
	void ResetMusics();
public:
	void release(void)
	{
		delete this;
	}
};

#endif
