/***************************************************************************
                          MUSImp.cpp  -  description
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

#include "../../includes/win32def.h"
#include "MUSImp.h"
#include "../Core/Interface.h"

MUSImp::MUSImp()
{
	Initialized = false;
	Playing = false;
	pl = NULL;
}
MUSImp::~MUSImp(){
}
/** Initializes the PlayList Manager */
bool MUSImp::Init()
{
	Initialized = true;
	return true;
}
/** Loads a PlayList for playing */
bool MUSImp::OpenPlaylist(const char * name)
{
	if(stricmp(name, PLName) == 0)
		return true;
	if(pl) {
		fclose(pl);
		pl = NULL;
	}
	char path[_MAX_PATH];
	strcpy(path, core->GamePath);
	strcat(path, "music");
	strcat(path, SPathDelimiter);
	strcat(path, name);
	strcat(path, ".mus");
	pl = fopen(path, "rb");
	if(!pl)
		return false;
	int count;
	fscanf(pl, "%[^\r\n]%*[\r]\n", PLName);
	fscanf(pl, "%d%*[\r]\n", &count);
	for(int i = 0; i < count; i++) {
		char PLVal[20], PLVar[20], PLEnd[20];
		fscanf(pl, "%[^ ]%*[ ]", PLVal);
		fscanf(pl, "%[^ ^\r^\n]", PLVar);
		int ch = fgetc(pl);
		if(ch == ' ') {
			
		}
	}
}
/** Start the PlayList Music Execution */
void MUSImp::Start(){
}
/** Ends the Current PlayList Execution */
void MUSImp::End(){
}
/** Switches the current PlayList while playing the current one */
void MUSImp::SwitchPlayList(const char * name){
}
