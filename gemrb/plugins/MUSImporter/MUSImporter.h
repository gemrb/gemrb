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

#include "Audio/MusicLoop.h"
#include "Streams/FileStream.h"

namespace GemRB {

/**MUS PlayList Importer
  *@author GemRB Development Team
  */

using MUSString = FixedSizeString<10>;
struct PLString {
	MUSString PLFile;
	MUSString PLLoop;
	MUSString PLTag;
	MUSString PLEnd;
	unsigned int soundID = 0;
};

class MUSImporter : public MusicMgr {
private:
	bool Initialized = false;
	bool Playing = false;
	ieVariable PLName;
	ieVariable PLNameNew;
	int PLpos = 0;
	int PLnext = -1;
	FileStream* str;
	std::vector<PLString> playlist;
	unsigned int lastSound = 0xffffffff;
	ResourceManager manager;

private:
	void PlayMusic(int pos);
	void PlayMusic(const ieVariable& name);

public:
	MUSImporter();
	MUSImporter(const MUSImporter&) = delete;
	~MUSImporter() override;
	MUSImporter& operator=(const MUSImporter&) = delete;
	/** Loads a PlayList for playing */
	bool OpenPlaylist(const ieVariable& name) override;
	/** Initializes the PlayList Manager */
	bool Init() override;
	/** Switches the current PlayList while playing the current one */
	int SwitchPlayList(const ieVariable& name, bool Hard) override;
	/** Ends the Current PlayList Execution */
	void End() override;
	void HardEnd() override;
	/** Start the PlayList Music Execution */
	void Start() override;
	/** Plays the Next Entry */
	void PlayNext() override;
	/** Returns whether music is currently playing */
	bool IsPlaying() override { return Playing; }
	/** Returns whether given playlist is currently loaded */
	bool IsCurrentPlayList(const ieVariable& name) override;
};

}

#endif
