/***************************************************************************
                          MUSImp.h  -  description
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

#ifndef MUSIMP_H
#define MUSIMP_H

#include "../Core/MusicMgr.h"
#include "../Core/FileStream.h"
#include <stdio.h>

/**MUS PlayList Importer
  *@author GemRB Developement Team
  */

typedef struct PLString {
	char PLFile[10];
	char PLLoop[10];
	char PLTag[10];
	char PLEnd[10];
	unsigned long soundID;
} PLString;
  
class MUSImp : public MusicMgr
{
private:
	bool Initialized;
	bool Playing;
	char PLName[32];
	int PLpos;
	FileStream * str;
	std::vector<PLString> playlist;
	unsigned long lastSound;
public: 
	MUSImp();
	~MUSImp();
  /** Loads a PlayList for playing */
  bool OpenPlaylist(const char * name);
  /** Initializes the PlayList Manager */
  bool Init();
  /** Switches the current PlayList while playing the current one */
  void SwitchPlayList(const char * name);
  /** Ends the Current PlayList Execution */
  void End();
  /** Start the PlayList Music Execution */
  void Start();
  /** Plays the Next Entry */
  void PlayNext();
};

#endif
