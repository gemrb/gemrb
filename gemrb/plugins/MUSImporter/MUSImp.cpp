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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/MUSImporter/MUSImp.cpp,v 1.32 2004/04/13 23:13:12 doc_wagon Exp $
 *
 */

#include "../../includes/win32def.h"
#include "MUSImp.h"
#include "../Core/Interface.h"

static char musicsubfolder[6] = "music";

MUSImp::MUSImp()
{
	Initialized = false;
	Playing = false;
	str = new FileStream();
	PLpos = 0;
	PLName[0] = '\0';
	lastSound = 0xffffffff;
#ifndef WIN32
	if (core->CaseSensitive) {
		//TODO: a better, generalised function 
		char path[_MAX_PATH];
		strcpy( path, core->GamePath );
		strcat( path, musicsubfolder );
		if (!dir_exists( path )) {
			musicsubfolder[0] = toupper( musicsubfolder[0] );
		}
	}
#endif
}
MUSImp::~MUSImp()
{
	if (str) {
		delete( str );
	}
}
/** Initializes the PlayList Manager */
bool MUSImp::Init()
{
	Initialized = true;
	return true;
}
/** Loads a PlayList for playing */
bool MUSImp::OpenPlaylist(const char* name)
{
	if (PLName[0] != '\0') {
		int len = ( int ) strlen( PLName );
		if (strnicmp( name, PLName, len ) == 0)
			return true;
	}
	if (Playing) {
		return false;
	}
	core->GetSoundMgr()->ResetMusics();
	playlist.clear();
	PLpos = 0;
	if (name[0] == '*') {
		return false;
	}
	char path[_MAX_PATH];
	strcpy( path, core->GamePath );
	strcat( path, musicsubfolder );
	strcat( path, SPathDelimiter );
	strcat( path, name );
	if (!str->Open( path, true )) {
		printf( "%s [NOT FOUND]\n", path );
		return false;
	}
	printf( "Loading %s\n", name );
	int c = str->ReadLine( PLName, 32 );
	while (c > 0) {
		if (( PLName[c - 1] == ' ' ) || ( PLName[c - 1] == '\t' ))
			PLName[c - 1] = 0;
		else
			break;
		c--;
	}
	char counts[5];
	str->ReadLine( counts, 5 );
	int count = atoi( counts );
	while (count != 0) {
		char line[64];
		int len = str->ReadLine( line, 64 );
		int i = 0;
		int p = 0;
		PLString pls;
		while (i < len) {
			if (( line[i] != ' ' ) && ( line[i] != '\t' ))
				pls.PLFile[p++] = line[i++];
			else {
				while (i < len) {
					if (( line[i] == ' ' ) || ( line[i] == '\t' ))
						i++;
					else
						break;
				}
				break;
			}
		}
		pls.PLFile[p] = 0;
		p = 0;
		if (line[i] != '@' && ( i < len )) {
			while (i < len) {
				if (( line[i] != ' ' ) && ( line[i] != '\t' ))
					pls.PLTag[p++] = line[i++];
				else
					break;
			}
			pls.PLTag[p] = 0;
			p = 0;
			while (i < len) {
				if (( line[i] == ' ' ) || ( line[i] == '\t' ))
					i++;
				else {
					break;
				}
			}
			if (line[i] == '@')
				strcpy( pls.PLLoop, pls.PLTag );
			else {
				while (i < len) {
					if (( line[i] != ' ' ) && ( line[i] != '\t' ))
						pls.PLLoop[p++] = line[i++];
					else
						break;
				}
				pls.PLLoop[p] = 0;
			}
			while (i < len) {
				if (( line[i] == ' ' ) || ( line[i] == '\t' ))
					i++;
				else
					break;
			}
			p = 0;
		} else {
			pls.PLTag[0] = 0;
			pls.PLLoop[0] = 0;
		}
		while (i < len) {
			if (( line[i] != ' ' ) && ( line[i] != '\t' ))
				i++;
			else {
				while (i < len) {
					if (( line[i] == ' ' ) || ( line[i] == '\t' ))
						i++;
					else
						break;
				}
				break;
			}
		}
		while (i < len) {
			if (( line[i] != ' ' ) && ( line[i] != '\t' ))
				pls.PLEnd[p++] = line[i++];
			else
				break;
		}
		pls.PLEnd[p] = 0;
		playlist.push_back( pls );
		count--;
	}
	return true;
}
/** Start the PlayList Music Execution */
void MUSImp::Start()
{
	if (!Playing) {
		PLpos = 0;
		if (playlist.size() == 0)
			return;
		if (playlist[PLpos].PLLoop[0] != 0) {
			for (unsigned int i = 0; i < playlist.size(); i++) {
				if (stricmp( playlist[i].PLFile, playlist[PLpos].PLLoop ) == 0) {
					PLnext = i;
					break;
				}
			}
		} else {
			PLnext = PLpos + 1;
			if (PLnext >= playlist.size())
				PLnext = -1;
		}
		PlayMusic( PLpos );
		core->GetSoundMgr()->Play();
		lastSound = playlist[PLpos].soundID;
		Playing = true;
	}
}
/** Ends the Current PlayList Execution */
void MUSImp::End()
{
	if (Playing) {
		if (playlist.size() == 0)
			return;
		if (playlist[PLpos].PLEnd[0] != 0) {
			if (stricmp( playlist[PLpos].PLEnd, "end" ) != 0)
				PlayMusic( playlist[PLpos].PLEnd );
		}
		PLnext = -1;
	}
}

void MUSImp::HardEnd()
{
	core->GetSoundMgr()->Stop();
	Playing = false;
	PLpos = 0;
}

/** Switches the current PlayList while playing the current one */
void MUSImp::SwitchPlayList(const char* name, bool Hard)
{
	if (Hard) {
		HardEnd();
	} else {
		End();
	}
	if (OpenPlaylist( name )) {
		Start();
	}
}
/** Plays the Next Entry */
void MUSImp::PlayNext()
{
	if (!Playing) {
		return;
	}
	if (PLnext != -1) {
		PlayMusic( PLnext );
		PLpos = PLnext;
		if (playlist[PLpos].PLLoop[0] != 0) {
			for (unsigned int i = 0; i < playlist.size(); i++) {
				if (stricmp( playlist[i].PLFile, playlist[PLpos].PLLoop ) == 0) {
					PLnext = i;
					break;
				}
			}
		} else {
			if (stricmp( playlist[PLnext].PLEnd, "end" ) == 0)
				PLnext = -1;
			else
				PLnext = PLpos + 1;
		}
	} else {
		Playing = false;
		core->GetSoundMgr()->Stop();
	}
}

void MUSImp::PlayMusic(int pos)
{
	PlayMusic( playlist[pos].PLFile );
}

void MUSImp::PlayMusic(char* name)
{
	char FName[_MAX_PATH];
	strcpy( FName, core->GamePath );
	strcat( FName, musicsubfolder );
	strcat( FName, SPathDelimiter );
	//this is in IWD2
	if (strnicmp( name, "mx0000", 6 ) == 0) {
		strcat( FName, "mx0000" );
		strcat( FName, SPathDelimiter );
	} else if (strnicmp( name, "SPC", 3 ) != 0) {
		strcat( FName, PLName );
		strcat( FName, SPathDelimiter );
		strcat( FName, PLName );
	}
	strcat( FName, name );
	strcat( FName, ".acm" );
	int soundID = core->GetSoundMgr()->StreamFile( FName );
#ifndef WIN32
	if (( soundID == -1 ) && core->CaseSensitive) {
		ResolveFilePath( FName );
		soundID = core->GetSoundMgr()->StreamFile( FName );
	}
#endif
	if (soundID == -1) {
		core->GetSoundMgr()->Stop();
	}
	printf( "Playing: %s\n", FName );
}
