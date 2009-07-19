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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $Id$
 *
 */

#ifndef MUSIMP_H
#define MUSIMP_H

#include "../Core/MusicMgr.h"
#include "../Core/FileStream.h"
#include <stdio.h>

/**MUS PlayList Importer
  *@author GemRB Development Team
  */

struct PLString {
	char PLFile[10];
	char PLLoop[10];
	char PLTag[10];
	char PLEnd[10];
	unsigned int soundID;
};

class MUSImp : public MusicMgr {
private:
	bool Initialized;
	bool Playing;
	char PLName[32];
	int PLpos, PLnext;
	FileStream* str;
	std::vector< PLString> playlist;
	unsigned int lastSound;
private:
	void PlayMusic(int pos);
	void PlayMusic(char* name);
public:
	MUSImp();
	~MUSImp();
	/** Loads a PlayList for playing */
	bool OpenPlaylist(const char* name);
	/** Initializes the PlayList Manager */
	bool Init();
	/** Switches the current PlayList while playing the current one */
	int SwitchPlayList(const char* name, bool Hard);
	/** Ends the Current PlayList Execution */
	void End();
	void HardEnd();
	/** Start the PlayList Music Execution */
	void Start();
	/** Plays the Next Entry */
	void PlayNext();
	/** Returns whether music is currently playing */
	bool IsPlaying() { return Playing; }
	/** Returns whether given playlist is currently loaded */
	bool CurrentPlayList(const char* name);
public:
	void release(void)
	{
		delete this;
	}
};

#endif
