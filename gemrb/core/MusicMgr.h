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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#ifndef MUSICMGR_H
#define MUSICMGR_H

#include "Plugin.h"

namespace GemRB {

class GEM_EXPORT MusicMgr : public Plugin {
public: 
	MusicMgr();
	virtual ~MusicMgr();
	/** Ends the Current PlayList Execution */
	virtual void End(void) = 0;
	virtual void HardEnd(void) = 0;
	/** Start the PlayList Music Execution */
	virtual void Start(void) = 0;
	/** Initializes the PlayList Manager */
	virtual bool Init();
	/** Loads a PlayList for playing */
	virtual bool OpenPlaylist(const char* name) = 0;
	/** Switches the current PlayList while playing the current one, return nonzero on error */
	virtual int SwitchPlayList(const char* name, bool Hard) = 0;
	/** Plays the Next Entry */
	virtual void PlayNext() = 0;
	/** Returns whether music is currently playing */
	virtual bool IsPlaying() = 0;
	/** Returns whether given playlist is currently loaded */
	virtual bool CurrentPlayList(const char* name) = 0;
};

}

#endif
