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
 *
 */

#include "MUSImporter.h"

#include "win32def.h"

#include "Audio.h"
#include "GameData.h" // For ResourceHolder
#include "Interface.h"
#include "SoundMgr.h"

using namespace GemRB;

static char musicsubfolder[6] = "music";

MUSImporter::MUSImporter()
{
	Initialized = false;
	Playing = false;
	str = new FileStream();
	PLpos = 0;
	PLnext = -1;
	PLName[0] = '\0';
	PLNameNew[0] = '\0';
	lastSound = 0xffffffff;
	char path[_MAX_PATH];
	PathJoin(path, core->GamePath, musicsubfolder, NULL);
	manager.AddSource(path, "Music", PLUGIN_RESOURCE_DIRECTORY);
}

MUSImporter::~MUSImporter()
{
	if (str) {
		delete( str );
	}
}
/** Initializes the PlayList Manager */
bool MUSImporter::Init()
{
	Initialized = true;
	return true;
}
/** Loads a PlayList for playing */
bool MUSImporter::OpenPlaylist(const char* name)
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
	PathJoin(path, core->GamePath, musicsubfolder, name, NULL);
	Log(MESSAGE, "MUSImporter", "Loading %s...", path);
	if (!str->Open(path)) {
		Log(ERROR, "MUSImporter", "Didn't find playlist '%s'.", path);
		return false;
	}
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
			if (p < 9) {
				pls.PLTag[p] = 0;
			} else {
				pls.PLTag[9] = 0;
			}
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
void MUSImporter::Start()
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
				PLnext = 0;
		}
		PlayMusic( PLpos );
		core->GetAudioDrv()->Play();
		lastSound = playlist[PLpos].soundID;
		Playing = true;
	}
}
/** Ends the Current PlayList Execution */
void MUSImporter::End()
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

void MUSImporter::HardEnd()
{
	core->GetAudioDrv()->Stop();
	Playing = false;
	PLpos = 0;
}

/** Switches the current PlayList while playing the current one, return nonzero on error */
int MUSImporter::SwitchPlayList(const char* name, bool Hard)
{
	if (Playing) {
		//don't do anything if the requested song is already playing
		//this fixed PST's infinite song start
		int len = ( int ) strlen( PLName );
		if (strnicmp( name, PLName, len ) == 0)
			return 0;
		if (Hard) {
			HardEnd();
		} else {
			End();
		}
		//if still playing, then don't insist on trying to open it now
		//either HardEnd stopped it for us, or End marked it for early ending
		if (Playing) {
			strlcpy(PLNameNew, name, sizeof(PLNameNew) );
			return 0;
		}
	}

	if (OpenPlaylist( name )) {
		Start();
		return 0;
	}

	return -1;
}
/** Plays the Next Entry */
void MUSImporter::PlayNext()
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
		//start new music after the old faded out
		if (PLNameNew[0]) {
			if (OpenPlaylist(PLNameNew)) {
				Start();
			}
			PLNameNew[0]='\0';
		}
	}
}

void MUSImporter::PlayMusic(int pos)
{
	PlayMusic( playlist[pos].PLFile );
}

void MUSImporter::PlayMusic(char* name)
{
	char FName[_MAX_PATH];
	if (strnicmp( name, "mx9000", 6 ) == 0) { //iwd2
		PathJoin(FName, "mx9000", name, NULL);
	} else if (strnicmp( name, "mx0000", 6 ) == 0) { //iwd
		PathJoin(FName, "mx0000", name, NULL);
	} else if (strnicmp( name, "SPC", 3 ) != 0) { //bg2
		char File[_MAX_PATH];
		snprintf(File, _MAX_PATH, "%s%s", PLName, name);
		PathJoin(FName, PLName, File, NULL);
	} else {
		strlcpy(FName, name, _MAX_PATH);
	}

	ResourceHolder<SoundMgr> sound(FName, manager);
	if (sound) {
		int soundID = core->GetAudioDrv()->CreateStream( sound );
		if (soundID == -1) {
			core->GetAudioDrv()->Stop();
		}
	} else {
		core->GetAudioDrv()->Stop();
	}
	print("Playing: %s", FName);
}

bool MUSImporter::CurrentPlayList(const char* name) {
	int len = ( int ) strlen( PLName );
	if (strnicmp( name, PLName, len ) == 0) return true;
	return false;
}


#include "plugindef.h"

GEMRB_PLUGIN(0x2DCB9E8, "MUS File Importer")
PLUGIN_CLASS(IE_MUS_CLASS_ID, MUSImporter)
END_PLUGIN()
