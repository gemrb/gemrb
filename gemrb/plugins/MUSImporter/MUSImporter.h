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
 *
 */

#ifndef MUSIMPORTER_H
#define MUSIMPORTER_H

#include "MusicMgr.h"

#include "ResourceManager.h"
#include "System/FileStream.h"

#include <cstdio>

namespace GemRB {

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

class MUSImporter : public MusicMgr {
private:
	bool Initialized;
	bool Playing;
	char PLName[32];
	char PLNameNew[32];
	int PLpos, PLnext;
	FileStream* str;
	std::vector< PLString> playlist;
	unsigned int lastSound;
	ResourceManager manager;
private:
	void PlayMusic(int pos);
	void PlayMusic(char* name);
public:
	MUSImporter();
	~MUSImporter();
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
};

}

#endif
