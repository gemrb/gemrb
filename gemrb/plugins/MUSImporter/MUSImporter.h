// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
