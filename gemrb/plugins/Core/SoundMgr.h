/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Id$
 *
 */

#ifndef SOUNDMGR_H
#define SOUNDMGR_H

#include "../../includes/ie_types.h"
#include "Plugin.h"
#include "AmbientMgr.h"

#define GEM_SND_RELATIVE 1
#define GEM_SND_SPEECH   IE_STR_SPEECH // 4
#define GEM_SND_VOL_MUSIC    1
#define GEM_SND_VOL_AMBIENTS 2

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

class GEM_EXPORT SoundMgr : public Plugin {
public:
	SoundMgr(void);
	virtual ~SoundMgr(void);
	virtual bool Init(void) = 0;
	virtual unsigned int Play(const char* ResRef, int XPos = 0, int YPos = 0, unsigned int flags = GEM_SND_RELATIVE) = 0;

	// cue music (from file). Don't start playing if stopped.
	virtual unsigned int StreamFile(const char* filename) = 0;

	// stop playing and delete MusicSource
	virtual bool Stop() = 0;
	// start cued music
	virtual bool Play() = 0;

	// able to actually play sounds?
	virtual bool CanPlay() = 0;

	// is speech playing?
	virtual bool IsSpeaking() = 0;
	virtual void ResetMusics() = 0;

	// update position of listener
	virtual void UpdateViewportPos(int XPos, int YPos) = 0;
	// update volumes (possibly on-the-fly)
	virtual void UpdateVolume( unsigned int = GEM_SND_VOL_MUSIC | GEM_SND_VOL_AMBIENTS ) {}

	virtual AmbientMgr *GetAmbientMgr() { return ambim; }
	virtual int SetupAmbientStream(ieWord x, ieWord y, ieWord z, ieWord gain, bool point) = 0; 
	virtual int QueueAmbient(int stream, const char* sound) = 0;
	virtual bool ReleaseAmbientStream(int stream, bool hardstop=false) = 0;
	virtual void SetAmbientStreamVolume(int stream, int gain) = 0;
protected:
	AmbientMgr *ambim;
};


#endif
