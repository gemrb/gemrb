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
	str = new FileStream();
	PLpos = 0;
	lastSound = 0xffffffff;
}
MUSImp::~MUSImp(){
	if(str)
		delete(str);
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
	playlist.clear();
	PLpos = 0;
	if(stricmp(name, PLName) == 0)
		return true;
	char path[_MAX_PATH];
	strcpy(path, core->GamePath);
	strcat(path, "music");
	strcat(path, SPathDelimiter);
	strcat(path, name);
	strcat(path, ".mus");
	if(!str->Open(path, true))
		return false;
	str->ReadLine(PLName, 32);
	char counts[3];
	str->ReadLine(counts, 3);
	int count = atoi(counts);
	while(count != 0) {
		char line[64], var1[10], var2[10], var3[10], var4[10];
		str->ReadLine(line, 64);
		int c = sscanf(line, "%[^ ]%*[ ]%[^ ]%*[ ]%[^ ]%*[ ]%[^ ]", var1, var2, var3, var4);
		switch(c) {
			case 1:
				{
					PLString pls;
					strcpy(pls.PLFile, var1);
					strcpy(pls.PLLoop, "");
					strcpy(pls.PLTag, "");
					strcpy(pls.PLEnd, "");
					char FName[_MAX_PATH];
      		strcpy(FName, core->GamePath);
      		strcat(FName, "music");
      		strcat(FName, SPathDelimiter);
      		strcat(FName, PLName);
      		strcat(FName, SPathDelimiter);
      		strcat(FName, PLName);
      		strcat(FName, pls.PLFile);
      		strcat(FName, ".acm");
					pls.soundID = core->GetSoundMgr()->LoadFile(FName);
					playlist.push_back(pls);
				}
			break;
			
			case 3:
				{
					PLString pls;
					strcpy(pls.PLFile, var1);
					strcpy(pls.PLLoop, "");
					strcpy(pls.PLTag, var2);
					strcpy(pls.PLEnd, var3);
					char FName[_MAX_PATH];
      		strcpy(FName, core->GamePath);
      		strcat(FName, "music");
      		strcat(FName, SPathDelimiter);
      		strcat(FName, PLName);
      		strcat(FName, SPathDelimiter);
      		strcat(FName, PLName);
      		strcat(FName, pls.PLFile);
      		strcat(FName, ".acm");
					pls.soundID = core->GetSoundMgr()->LoadFile(FName);
					playlist.push_back(pls);
				}
			break;

			case 4:
				{
					PLString pls;
					strcpy(pls.PLFile, var1);
					strcpy(pls.PLLoop, var2);
					strcpy(pls.PLTag, var3);
					strcpy(pls.PLEnd, var4);
					char FName[_MAX_PATH];
      		strcpy(FName, core->GamePath);
      		strcat(FName, "music");
      		strcat(FName, SPathDelimiter);
      		strcat(FName, PLName);
      		strcat(FName, SPathDelimiter);
      		strcat(FName, PLName);
      		strcat(FName, pls.PLFile);
      		strcat(FName, ".acm");
					pls.soundID = core->GetSoundMgr()->LoadFile(FName);
					playlist.push_back(pls);
				}
			break;

			default:
				{

				}
			break;
		}
		count--;
	}
}
/** Start the PlayList Music Execution */
void MUSImp::Start()
{
	if(!Playing) {
		PLpos = 0;
		core->GetSoundMgr()->Play(playlist[PLpos].soundID);
		lastSound = playlist[PLpos].soundID;
		Playing = true;
	}
}
/** Ends the Current PlayList Execution */
void MUSImp::End()
{
	if(Playing) {
		if(playlist[PLpos].PLEnd[0] != 0) {
			core->GetSoundMgr()->Stop(lastSound);
			for(int i = 0; i < playlist.size(); i++) {
				if(stricmp(playlist[i].PLFile, playlist[PLpos].PLEnd) == 0) {
					core->GetSoundMgr()->Play(playlist[i].soundID);
					return;
				}
			}
			char FName[_MAX_PATH];
			strcpy(FName, core->GamePath);
			strcat(FName, "music");
			strcat(FName, SPathDelimiter);
			strcat(FName, PLName);
			strcat(FName, SPathDelimiter);
			strcat(FName, PLName);
			strcat(FName, playlist[PLpos].PLEnd);
			strcat(FName, ".acm");
			core->GetSoundMgr()->Stop(lastSound);
			lastSound = core->GetSoundMgr()->LoadFile(FName);
			core->GetSoundMgr()->Play(lastSound);
		}
		else
			core->GetSoundMgr()->Stop(lastSound);
	}
}
/** Switches the current PlayList while playing the current one */
void MUSImp::SwitchPlayList(const char * name){
}
/** Plays the Next Entry */
void MUSImp::PlayNext()
{
	if(playlist[PLpos].PLLoop[0] != 0) {
		printf("Looping...\n");
		for(int i = 0; i < playlist.size(); i++) {
			if(stricmp(playlist[i].PLFile, playlist[PLpos].PLLoop) == 0) {
				PLpos = i;
				break;
			}
		}
		lastSound = playlist[PLpos].soundID;
		core->GetSoundMgr()->Play(lastSound);
	}
	else {
		printf("Next Track...\n");
		PLpos++;
		lastSound = playlist[PLpos].soundID;
		core->GetSoundMgr()->Play(lastSound);
	}
}
