/***************************************************************************
                          MusicMgr.h  -  description
                             -------------------
    begin                : ven ott 24 2003
    copyright            : (C) 2003 by GemRB Developement Team
    email                : Balrog994@yahoo.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef MUSICMGR_H
#define MUSICMGR_H

#include "Plugin.h"

/**Base class for Music Playlist Player Plugin
  *@author GemRB Developement Team
  */

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

class GEM_EXPORT MusicMgr : public Plugin
{
public: 
	MusicMgr();
	virtual ~MusicMgr();
  /** Ends the Current PlayList Execution */
  virtual void End(void) = 0;
  /** Start the PlayList Music Execution */
  virtual void Start(void) = 0;
  /** Initializes the PlayList Manager */
  virtual bool Init();
  /** Loads a PlayList for playing */
  virtual bool OpenPlaylist(const char *name) = 0;
  /** Switches the current PlayList while playing the current one */
  virtual void SwitchPlayList(const char * name) = 0;
};

#endif
