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
 * $Id$
 *
 */

#ifndef NULLSND_H
#define NULLSND_H

#include "../Core/SoundMgr.h"

class NullSnd : public SoundMgr {
public:
	NullSnd(void);
	~NullSnd(void);
	bool Init(void);
	unsigned int Play(const char* ResRef, int XPos = 0, int YPos = 0,  unsigned int flags = GEM_SND_RELATIVE);
	unsigned int StreamFile(const char* filename);
	bool Play();
	bool Stop();
	bool CanPlay();
	bool IsSpeaking();
	void ResetMusics();
	void UpdateListenerPos(int XPos, int YPos);
	void GetListenerPos(int& XPos, int& YPos);

	int SetupAmbientStream(ieWord x, ieWord y, ieWord z, ieWord gain, bool point);
	int QueueAmbient(int stream, const char* sound);
	bool ReleaseAmbientStream(int stream, bool hardstop);
	void SetAmbientStreamVolume(int stream, int gain);


public:
	void release(void)
	{
		delete this;
	}

private:
	int XPos, YPos;
};

#endif
