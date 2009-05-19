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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $Id$
 *
 */

#include "../../includes/win32def.h"
#include "MUSImp.h"
#include "../Core/Interface.h"
#include "../Core/Audio.h"

static char musicsubfolder[6] = "music";

MUSImp::MUSImp()
{
	Initialized = false;
	Playing = false;
	str = new FileStream();
	PLpos = 0;
	PLName[0] = '\0';
	lastSound = 0xffffffff;
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
	core->GetAudioDrv()->ResetMusics();
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
#ifndef WIN32
	ResolveFilePath( path );
#endif
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
			if ((unsigned int) PLnext >= playlist.size())
				PLnext = -1;
		}
		PlayMusic( PLpos );
		core->GetAudioDrv()->Play();
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
	core->GetAudioDrv()->Stop();
	Playing = false;
	PLpos = 0;
}

/** Switches the current PlayList while playing the current one */
void MUSImp::SwitchPlayList(const char* name, bool Hard)
{
	//don't do anything if the requested song is already playing
	//this fixed PST's infinite song start
	if (Playing) {
		int len = ( int ) strlen( PLName );
		if (strnicmp( name, PLName, len ) == 0)
			return;
		if (Hard) {
			HardEnd();
		} else {
			End();
		}
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
				if ((unsigned int) PLnext >= playlist.size() ) {
					PLnext = 0;
				}
		}
	} else {
		Playing = false;
		core->GetAudioDrv()->Stop();
	}
}

void MUSImp::PlayMusic(int pos)
{
	PlayMusic( playlist[pos].PLFile );
}

void MUSImp::PlayMusic(char* name)
{
	char FName[_MAX_PATH];
	if (strnicmp( name, "mx9000", 6 ) == 0) { //iwd2
		snprintf( FName, _MAX_PATH, "%s%s%smx9000%s%s.acm",
			core->GamePath, musicsubfolder, SPathDelimiter,
			SPathDelimiter, name);
	} else if (strnicmp( name, "mx0000", 6 ) == 0) { //iwd
		snprintf( FName, _MAX_PATH, "%s%s%smx0000%s%s.acm",
			core->GamePath, musicsubfolder, SPathDelimiter,
			SPathDelimiter, name);
	} else if (strnicmp( name, "SPC", 3 ) != 0) { //bg2
		snprintf( FName, _MAX_PATH, "%s%s%s%s%s%s%s.acm",
			core->GamePath, musicsubfolder, SPathDelimiter,
			PLName, SPathDelimiter, PLName, name);
	}
	else {
		snprintf(FName, _MAX_PATH, "%s%s%s%s.acm",
			core->GamePath, musicsubfolder, SPathDelimiter,
			name);
	}
#ifndef WIN32
		ResolveFilePath( FName );
#endif
	int soundID = core->GetAudioDrv()->StreamFile( FName );
	if (soundID == -1) {
		core->GetAudioDrv()->Stop();
	}
	printf( "Playing: %s\n", FName );
}
