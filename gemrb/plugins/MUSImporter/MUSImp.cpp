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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/MUSImporter/MUSImp.cpp,v 1.18 2003/12/07 13:13:13 avenger_teambg Exp $
 *
 */

#include "../../includes/win32def.h"
#include "MUSImp.h"
#include "../Core/Interface.h"

static char musicsubfolder[6]="music";

void ResolveFilePath(char *FilePath)
{
	char TempFilePath[_MAX_PATH];
	char TempFileName[_MAX_PATH];
	int j, pos;

	TempFilePath[0]=FilePath[0];
	for(pos=1;FilePath[pos] && FilePath[pos]!='/';pos++)
		TempFilePath[pos]=FilePath[pos];
	TempFilePath[pos]=0;
	while(FilePath[pos]=='/') {
		pos++;
		for(j=0;FilePath[pos+j] && FilePath[pos+j]!='/';j++) {
			TempFileName[j]=FilePath[pos+j];
		}
		TempFileName[j]=0;
		pos+=j;
		char *NewName = FindInDir(TempFilePath, TempFileName);
		if(NewName) {
			strcat(TempFilePath, SPathDelimiter);
			strcat(TempFilePath, NewName);
			free(NewName);
		}
		else {
			abort();
		}
		printf("TempFilePath: %s\n",TempFilePath);
	}
	//should work (same size)
	strcpy(FilePath,TempFilePath);
}

MUSImp::MUSImp()
{
	Initialized = false;
	Playing = false;
	str = new FileStream();
	PLpos = 0;
	lastSound = 0xffffffff;
#ifndef WIN32
	if(core->CaseSensitive) {
		//TODO: a better, generalised function 
		char path[_MAX_PATH];
		strcpy(path, core->GamePath);
		strcat(path, musicsubfolder);
		if(!dir_exists(path) ) {
			 musicsubfolder[0]=toupper(musicsubfolder[0]);
		}
	}
#endif
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
	int len=strlen(PLName);
	if(strnicmp(name, PLName, len) == 0)
		return true;
	if(Playing)
		return false;
	core->GetSoundMgr()->ResetMusics();
	playlist.clear();
	PLpos = 0;
	if(name[0]=='*')
		return false;
	char path[_MAX_PATH];
	strcpy(path, core->GamePath);
	strcat(path, musicsubfolder);
	strcat(path, SPathDelimiter);
	strcat(path, name);
	if(!str->Open(path, true)) {
		printf("%s [NOT FOUND]\n",path);
		return false;
	}
	printf("Loading %s\n",name);
	str->ReadLine(PLName, 32);
	char counts[5];
	str->ReadLine(counts, 5);
	int count = atoi(counts);
	while(count != 0) {
		char line[64];
		str->ReadLine(line, 64);
		int len = strlen(line);
		int i = 0;
		int p = 0;
		PLString pls;
		while(i < len) {
			if((line[i]!=' ') && (line[i]!='\t'))
				pls.PLFile[p++] = line[i++];
			else {
				while(i < len) {
					if((line[i]==' ') || (line[i]=='\t'))
						i++;
					else
						break;
				}
				break;
			}		
		}
		pls.PLFile[p]=0;
		p=0;
		if(line[i]!='@' && (i < len)) {
			while(i < len) {
				if((line[i]!=' ') && (line[i]!='\t'))
					pls.PLTag[p++] = line[i++];
				else
					break;
			}
			pls.PLTag[p]=0;
			p=0;
			i++;
			if((line[i]==' ') || (line[i]=='\t'))
				strcpy(pls.PLLoop, pls.PLTag);
			else {
				while(i < len) {
					if((line[i]!=' ') && (line[i]!='\t'))
						pls.PLLoop[p++] = line[i++];
					else
						break;
				}
			pls.PLLoop[p]=0;
			}
			while(i < len) {
				if((line[i]==' ') || (line[i]=='\t'))
					i++;
				else
					break;
			}
			p=0;
		}
		else {
			pls.PLTag[0]=0;
			pls.PLLoop[0]=0;
		}
		while(i < len) {
			if((line[i]!=' ') && (line[i]!='\t'))
				i++;
			else
				break;
		}
		i++;
		while(i < len) {
			if((line[i]!=' ') && (line[i]!='\t'))
				pls.PLEnd[p++] = line[i++];
			else
				break;
		}
		pls.PLEnd[p]=0;
		bool found = false;
		for(int i = 0; i < playlist.size(); i++) {
			if(stricmp(pls.PLFile, playlist[i].PLFile) == 0) {
				pls.soundID = playlist[i].soundID;
				found = true;
				break;
			}
		}
		if(!found) {
			char FName[_MAX_PATH];
			strcpy(FName, core->GamePath);
			strcat(FName, musicsubfolder);
			strcat(FName, SPathDelimiter);
			//this is in IWD2
			if(strnicmp(pls.PLFile, "mx0000",6)==0) {
				strcat(FName, "mx0000");
				strcat(FName, SPathDelimiter);
			}
			else if(strnicmp(pls.PLFile, "SPC",3) != 0) {
				strcat(FName, PLName);
				strcat(FName, SPathDelimiter);
				strcat(FName, PLName);
			}
			strcat(FName, pls.PLFile);
			strcat(FName, ".acm");
			pls.soundID = core->GetSoundMgr()->LoadFile(FName);
#ifndef WIN32
			if((pls.soundID==-1) && core->CaseSensitive) {
					ResolveFilePath(FName);
					pls.soundID = core->GetSoundMgr()->LoadFile(FName);
			}
#endif
			printf("Loading: %s / %d\n",FName,pls.soundID);
		}
		playlist.push_back(pls);
		count--;
	}
	return true;
}
/** Start the PlayList Music Execution */
void MUSImp::Start()
{
	if(!Playing) {
		PLpos = 0;
		if(playlist.size()==0)
			return;
		if(playlist[PLpos].PLLoop[0] != 0) {
			for(int i = 0; i < playlist.size(); i++) {
				if(stricmp(playlist[i].PLFile, playlist[PLpos].PLLoop) == 0) {
					PLnext = i;
					break;
				}
			}
		}
		else {
			PLnext=PLpos+1;
		}
		core->GetSoundMgr()->Play(playlist[PLpos].soundID);
		lastSound = playlist[PLpos].soundID;
		Playing = true;
	}
}
/** Ends the Current PlayList Execution */
void MUSImp::End()
{
	if(Playing) {
		if(playlist.size()==0)
			return;
		if(playlist[PLpos].PLEnd[0] != 0) {
			//core->GetSoundMgr()->Stop(lastSound);
			for(int i = 0; i < playlist.size(); i++) {
				if(stricmp(playlist[i].PLFile, playlist[PLpos].PLEnd) == 0) {
					core->GetSoundMgr()->Play(playlist[i].soundID);
					PLnext = i;
					return;
				}
			}
			PLString pls;
			strcpy(pls.PLFile, playlist[PLpos].PLEnd);
			pls.PLEnd[0] = 1;
			pls.PLEnd[1] = 0;
			pls.PLLoop[0] = 0;
			pls.PLTag[0] = 0;
			char FName[_MAX_PATH];
			strcpy(FName, core->GamePath);
			strcat(FName, musicsubfolder);
			strcat(FName, SPathDelimiter);
			if(stricmp(pls.PLFile, "SPC1") != 0) {
				strcat(FName, PLName);
				strcat(FName, SPathDelimiter);
				strcat(FName, PLName);
			}
			strcat(FName, playlist[PLpos].PLEnd);
			strcat(FName, ".acm");
			//core->GetSoundMgr()->Stop(lastSound);
			pls.soundID = core->GetSoundMgr()->LoadFile(FName);
			//core->GetSoundMgr()->Play(lastSound);
			playlist.push_back(pls);
			PLnext = playlist.size()-1;
		}
		else
			//core->GetSoundMgr()->Stop(lastSound);
			PLnext = -1;
	}
}

void MUSImp::HardEnd()
{
	core->GetSoundMgr()->Stop(lastSound);
	Playing = false;
	PLpos = 0;
}

/** Switches the current PlayList while playing the current one */
void MUSImp::SwitchPlayList(const char * name, bool Hard) {
	if(Hard) HardEnd();
	else End();
	if(OpenPlaylist(name))
		Start();
}
/** Plays the Next Entry */
void MUSImp::PlayNext()
{
	if(!Playing)
		return;
	if(PLnext != -1) {
		lastSound = playlist[PLnext].soundID;
		PLpos = PLnext;
		core->GetSoundMgr()->Play(lastSound);
		if(playlist[PLpos].PLLoop[0] != 0) {
			for(int i = 0; i < playlist.size(); i++) {
				if(stricmp(playlist[i].PLFile, playlist[PLpos].PLLoop) == 0) {
					PLnext = i;
					break;
				}
			}
		}
		else {
			if(playlist[PLnext].PLEnd[0] == 1)
				PLnext = -1;
			else
				PLnext=PLpos+1;
		}
	}
	else
		Playing = false;
}
