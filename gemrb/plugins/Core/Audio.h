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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $Id: OpenALAudio.cpp 5035 2008-02-10 23:33:21Z wjpalenstijn $
 *
 */

#ifndef AUDIO_H_INCLUDED
#define AUDIO_H_INCLUDED

#include "../../includes/win32def.h"
#include "../../includes/globals.h"
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

class GEM_EXPORT Audio : public Plugin {
public:
    Audio(void);
    virtual ~Audio();
    virtual bool Init(void) = 0;
    virtual unsigned int Play(const char* ResRef, int XPos = 0, int YPos = 0, unsigned int flags = GEM_SND_RELATIVE) = 0;
    virtual bool IsSpeaking() = 0;
    virtual AmbientMgr* GetAmbientMgr() { return ambim; };
    virtual void UpdateVolume(unsigned int flags = GEM_SND_VOL_MUSIC | GEM_SND_VOL_AMBIENTS) = 0;
    virtual bool CanPlay() = 0;
    virtual void ResetMusics() = 0;
    virtual bool Play() = 0;
    virtual bool Stop() = 0;
    virtual int StreamFile(const char* fileName ) = 0;
    virtual void UpdateListenerPos(int XPos, int YPos ) = 0;
    virtual void GetListenerPos(int &XPos, int &YPos ) = 0;
    virtual bool ReleaseStream(int stream, bool HardStop=false ) = 0;
    virtual int SetupNewStream( ieWord x, ieWord y, ieWord z,
                ieWord gain, bool point, bool Ambient) = 0;
    virtual int QueueAmbient(int stream, const char* sound) = 0;
    virtual void SetAmbientStreamVolume(int stream, int volume) = 0;
    virtual void QueueBuffer(int stream, unsigned short bits,
                int channels, short* memory, int size, int samplerate) = 0;

protected:
    AmbientMgr* ambim;

};

#endif // AUDIO_H_INCLUDED
