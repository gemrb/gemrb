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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/SaveGameIterator.cpp,v 1.14 2004/02/11 20:39:36 balrog994 Exp $
 *
 */

#include "SaveGameIterator.h"
#ifdef WIN32
#include <io.h>
#include <windows.h>
#include <direct.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fnmatch.h>
#include <dlfcn.h>
#endif
#include "Interface.h"

extern Interface * core;

SaveGameIterator::SaveGameIterator(void)
{
}

SaveGameIterator::~SaveGameIterator(void)
{
}

static void DelTree(char *Pt)
{
	char Path[_MAX_PATH];
	strcpy(Path, Pt);
#ifdef WIN32
	struct _finddata_t c_file;
	strcat(Path, SPathDelimiter);
	strcat(Path, "*.*");
	long hFile=(long)_findfirst(Path, &c_file);
	if(!hFile)
		return;
#else
	DIR * dir = opendir(Path);
	if(dir==NULL)
		return;
	struct dirent * de = readdir(dir);  //Lookup the first entry in the Directory
	if(de == NULL) {
		closedir(dir);
		return;
	}
#endif
	do {
		char dtmp[_MAX_PATH];
#ifdef WIN32
		if(c_file.attrib & _A_SUBDIR)
			if(c_file.name[0] == '.')
				continue;
		sprintf(dtmp, "%s%s%s", Pt, SPathDelimiter, c_file.name);
#else
		struct stat fst;
		sprintf(dtmp, "%s%s%s", Path, SPathDelimiter, de->d_name);
		stat(dtmp, &fst);
		if(S_ISDIR(fst.st_mode))
			continue;
		if(de->d_name[0] == '.')
			continue;
#endif
		unlink(dtmp);
#ifdef WIN32
	} while(_findnext(hFile, &c_file) == 0);
	_findclose(hFile);
#else
	} while((de = readdir(dir)) != NULL);
	closedir(dir);
#endif
}

static const char *PlayMode()
{
	unsigned long playmode=1;

	core->GetDictionary()->Lookup("PlayMode",playmode);
	if(playmode==2) return "mpsave";
	return "save";
}

int SaveGameIterator::GetSaveGameCount()
{
/*
	char Prefix[10] = {0};
	for(int i=0;i<8 && core->INIConfig[i] && core->INIConfig[i]!='.';i++) Prefix[i]=core->INIConfig[i];
*/
#ifdef WIN32
	//The windows _findfirst/_findnext functions allow the use of wildcards so we'll use them :)
	struct _finddata_t c_file;
	long hFile;
#endif
	int count = 0;
	char Path[_MAX_PATH];
	const char *SaveFolder=PlayMode();
#ifdef WIN32
	sprintf(Path, "%s%s%s*.*", core->GamePath, SaveFolder,SPathDelimiter);
	if((hFile = (long)_findfirst(Path, &c_file)) == -1L) //If there is no file matching our search
		return -1;
#else
	sprintf(Path, "%s%s", core->GamePath,SaveFolder);
	DIR * dir = opendir(Path);
	if(dir == NULL) //If we cannot open the Directory
		return -1;
	struct dirent * de = readdir(dir);  //Lookup the first entry in the Directory
	if(de == NULL) {
		closedir(dir);
		return -1;
	}
#endif
	do { //Iterate through all the available modules to load
		char dtmp[_MAX_PATH];
#ifdef WIN32
		if(c_file.attrib & _A_SUBDIR) {
			if(c_file.name[0] == '.')
				continue;
			sprintf(dtmp, "%s%s%s%s", core->GamePath, SaveFolder, SPathDelimiter, c_file.name);
#else
		struct stat fst;
		sprintf(dtmp, "%s%s%s", Path, SPathDelimiter, de->d_name);
		stat(dtmp, &fst);
		if(S_ISDIR(fst.st_mode)) {
			if(de->d_name[0] == '.')
				continue;
#endif
			char ftmp[_MAX_PATH];
			sprintf(ftmp, "%s%s%s.bmp", dtmp, SPathDelimiter, core->GameNameResRef);
#ifndef WIN32
			ResolveFilePath (ftmp);
#endif
			FILE * exist = fopen(ftmp, "rb");
			if(!exist)
				continue;
			fclose(exist);
#ifndef WIN32
			char Name[_MAX_PATH], Text[_MAX_PATH];
			int cnt = sscanf(de->d_name, "%*d-%s - %s", Name, Text);
#else
			char Name[_MAX_PATH], Text[_MAX_PATH];
			int cnt = sscanf(c_file.name, "%*d-%s - %s", Name, Text);
#endif
			if(cnt == 2) {
				printf("[Name = %s, Text = %s]\n", Name, Text);
				count++;
			}
			if(cnt == 1) {
				printf("[Name = %s, No Description]\n", Name);
				count++;
			}
		}
#ifdef WIN32
	} while(_findnext(hFile, &c_file) == 0);
	_findclose(hFile);
#else
	} while((de = readdir(dir)) != NULL);
	closedir(dir);  //No other files in the directory, close it
#endif
	return count;
}

SaveGame * SaveGameIterator::GetSaveGame(int index, bool Remove)
{
/*
	char Prefix[10];
	int i;
	for(i=0;i<8 && core->INIConfig[i] && core->INIConfig[i]!='.';i++) Prefix[i]=core->INIConfig[i];
	Prefix[i]=0;
*/
#ifdef WIN32
	//The windows _findfirst/_findnext functions allow the use of wildcards so we'll use them :)
	struct _finddata_t c_file;
	long hFile;
	int flg;
#endif
	int count = -1, prtrt = 0;
	char Path[_MAX_PATH];
	char dtmp[_MAX_PATH];
	const char *SaveFolder=PlayMode();
#ifdef WIN32
	sprintf(Path, "%s%s%s*.*", core->GamePath, SaveFolder, SPathDelimiter);
	if((hFile = (long)_findfirst(Path, &c_file)) == -1L) //If there is no file matching our search
		return NULL;
	sprintf(Path, "%s%s%s", core->GamePath, SaveFolder);
#else
	sprintf(Path, "%s%s", core->GamePath, SaveFolder);
	DIR * dir = opendir(Path);
	if(dir == NULL) //If we cannot open the Directory
		return NULL;
#endif
#ifndef WIN32  //Linux Statement
	struct dirent * de = readdir(dir);  //Lookup the first entry in the Directory
	if(de == NULL) //If no entry exists just return
		return NULL;
#endif
	char Name[_MAX_PATH], Text[_MAX_PATH];
	do { //Iterate through all the available modules to load
#ifdef WIN32
		sprintf(dtmp, "%s%s%s", Path, SPathDelimiter, c_file.name);
		if(c_file.attrib & _A_SUBDIR) {
			if(c_file.name[0] == '.')
				continue;
			sprintf(dtmp, "%s%s%s%s", core->GamePath, SaveFolder, SPathDelimiter, c_file.name);
#else
		struct stat fst;
		sprintf(dtmp, "%s%s%s", Path, SPathDelimiter, de->d_name);
		stat(dtmp, &fst);
		if(S_ISDIR(fst.st_mode)) {
			if(de->d_name[0] == '.')
				continue;
#endif
			char ftmp[_MAX_PATH];
			sprintf(ftmp, "%s%s%s.bmp", dtmp, SPathDelimiter, core->GameNameResRef);
#ifndef WIN32
			ResolveFilePath (ftmp);
#endif
			FILE * exist = fopen(ftmp, "rb");
			if(!exist)
				continue;
			fclose(exist);
#ifndef WIN32
			int cnt = sscanf(de->d_name, "%*d-%s - %s", Name, Text);
#else
			int cnt = sscanf(c_file.name, "%*d-%s - %s", Name, Text);
#endif
			if(cnt == 2) {
				printf("[Name = %s, Text = %s]\n", Name, Text);
				count++;
			}
			if(cnt == 1) {
				Text[0]=0;
				printf("[Name = %s, No Description]\n", Name);
				count++;
			}
			if(count == index) {
				if(Remove) {
#ifdef WIN32
					sprintf(Path, "%s%s%s%s", core->GamePath, SaveFolder, SPathDelimiter, c_file.name);
#else
					sprintf(Path, "%s%s%s%s", core->GamePath, SaveFolder, SPathDelimiter, de->d_name);
#endif
					DelTree(Path);
					rmdir(Path);
					break;
				}
#ifdef WIN32
				long file;
				struct _finddata_t bmpf;
				sprintf(Path, "%s%s%s%s%s*.bmp", core->GamePath, SaveFolder, SPathDelimiter, c_file.name, SPathDelimiter);
				file = (long)_findfirst(Path, &bmpf);
				if(file == NULL) {
					_findclose(hFile);
					return NULL;
				}
#else
				sprintf(Path, "%s%s%s%s", core->GamePath, SaveFolder, SPathDelimiter, de->d_name);
				DIR * ndir = opendir(Path);
				//If we cannot open the Directory
				if(ndir == NULL) {
					closedir(dir);
					return NULL;
				}
				struct dirent * de2 = readdir(ndir);  //Lookup the first entry in the Directory
				if(de2 == NULL) {  // No first entry!!!
					closedir(dir);
					closedir(ndir);
					return NULL;
				}
#endif
				do {
#ifdef WIN32
					if(strnicmp(bmpf.name, "PORTRT", 6) == 0)
#else
					if(strnicmp(de2->d_name, "PORTRT", 6) == 0)
#endif
						prtrt++;
#ifdef WIN32
				} while(_findnext(file, &bmpf) == 0);
				_findclose(file);
#else
				} while((de2 = readdir(ndir)) != NULL);
				closedir(ndir);  //No other files in the directory, close it
#endif
				break;
			}
		}
#ifdef WIN32
	} while((flg=_findnext(hFile, &c_file)) == 0);
	_findclose(hFile);
	if(Remove || (flg != 0))
		return NULL;
#else
	} while((de = readdir(dir)) != NULL);
	closedir(dir);  //No other files in the directory, close it
	if(Remove || (de == NULL) )
		return NULL;
#endif
	SaveGame * sg = new SaveGame(dtmp, Name, core->GameNameResRef, prtrt);
	return sg;
}
