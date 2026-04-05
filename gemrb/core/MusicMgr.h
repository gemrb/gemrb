// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef MUSICMGR_H
#define MUSICMGR_H

#include "ie_types.h"

#include "Plugin.h"

namespace GemRB {

class GEM_EXPORT MusicMgr : public Plugin {
public:
	/** Ends the Current PlayList Execution */
	virtual void End(void) = 0;
	virtual void HardEnd(void) = 0;
	/** Start the PlayList Music Execution */
	virtual void Start(void) = 0;
	/** Initializes the PlayList Manager */
	virtual bool Init() = 0;
	/** Loads a PlayList for playing */
	virtual bool OpenPlaylist(const ieVariable& name) = 0;
	/** Switches the current PlayList while playing the current one, return nonzero on error */
	virtual int SwitchPlayList(const ieVariable& name, bool Hard) = 0;
	/** Plays the Next Entry */
	virtual void PlayNext() = 0;
	/** Returns whether music is currently playing */
	virtual bool IsPlaying() = 0;
	/** Returns whether given playlist is currently loaded */
	virtual bool IsCurrentPlayList(const ieVariable& name) = 0;
};

}

#endif
