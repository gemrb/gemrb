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

#include "Audio.h"
#include "GameData.h" // For ResourceHolder
#include "Interface.h"
#include "SoundMgr.h"

using namespace GemRB;

static char musicsubfolder[6] = "music";

MUSImporter::MUSImporter()
{
	str = new FileStream();
	char path[_MAX_PATH];
	PathJoin(path, core->config.GamePath, musicsubfolder, nullptr);
	manager.AddSource(path, "Music", PLUGIN_RESOURCE_DIRECTORY);
}

MUSImporter::~MUSImporter()
{
	delete str;
}

/** Initializes the PlayList Manager */
bool MUSImporter::Init()
{
	Initialized = true;
	return true;
}

#define SKIP_BLANKS while (i < len) {\
	if (isblank(line[i])) {\
		i++;\
	} else {\
		break;\
	}\
}

#define FILL_VAR(VAR) while (i < len) {\
	if (!isblank(line[i])) {\
		pls.VAR[p++] = line[i++];\
	} else {\
		break;\
	}\
}

/** Loads a PlayList for playing */
bool MUSImporter::OpenPlaylist(const ieVariable& name)
{
	if (Playing || IsCurrentPlayList(name)) {
		return true;
	}
	core->GetAudioDrv()->ResetMusics();
	playlist.clear();
	PLpos = 0;
	PLName.Reset();
	if (IsStar(name)) {
		return false;
	}
	char path[_MAX_PATH];
	PathJoin(path, core->config.GamePath, musicsubfolder, name, nullptr);
	Log(MESSAGE, "MUSImporter", "Loading {}...", path);
	if (!str->Open(path)) {
		Log(ERROR, "MUSImporter", "Didn't find playlist '{}'.", path);
		return false;
	}
	strret_t c = str->ReadVariable(PLName);
	while (c > 0) {
		if (isblank(PLName[c - 1]))
			PLName[c - 1] = 0;
		else
			break;
		c--;
	}
	std::string line;
	str->ReadLine(line, 5);
	int count = atoi(line.c_str());
	while (count != 0) {
		strret_t len = str->ReadLine(line);
		int i = 0;
		int p = 0;
		PLString pls;
		while (i < len) {
			if (!isblank(line[i]))
				pls.PLFile[p++] = line[i++];
			else {
				SKIP_BLANKS
				break;
			}
		}
		pls.PLFile[p] = 0;
		p = 0;
		if (i < len && line[i] != '@') {
			FILL_VAR(PLTag)
			if (p < 9) {
				pls.PLTag[p] = 0;
			} else {
				pls.PLTag[9] = 0;
			}
			p = 0;
			SKIP_BLANKS
			if (line[i] == '@')
				strcpy( pls.PLLoop, pls.PLTag );
			else {
				FILL_VAR(PLLoop)
				pls.PLLoop[p] = 0;
			}
			SKIP_BLANKS
			p = 0;
		} else {
			pls.PLTag[0] = 0;
			pls.PLLoop[0] = 0;
		}
		while (i < len) {
			if (!isblank(line[i]))
				i++;
			else {
				SKIP_BLANKS
				break;
			}
		}
		FILL_VAR(PLEnd)
		pls.PLEnd[p] = 0;
		playlist.push_back( pls );
		count--;
	}
	return true;
}
/** Start the PlayList Music Execution */
void MUSImporter::Start()
{
	if (Playing) return;
	if (playlist.empty()) return;

	PLpos = 0;
	if (playlist[PLpos].PLLoop[0] != 0) {
		for (unsigned int i = 0; i < playlist.size(); i++) {
			if (stricmp(playlist[i].PLFile, playlist[PLpos].PLLoop) == 0) {
				PLnext = i;
				break;
			}
		}
	} else {
		PLnext = PLpos + 1;
		if ((unsigned int) PLnext >= playlist.size()) {
			PLnext = 0;
		}
	}

	PlayMusic(PLpos);
	core->GetAudioDrv()->Play();
	lastSound = playlist[PLpos].soundID;
	Playing = true;
}
/** Ends the Current PlayList Execution */
void MUSImporter::End()
{
	if (!Playing) return;
	if (playlist.empty()) return;

	if (playlist[PLpos].PLEnd[0] != 0) {
		if (stricmp(playlist[PLpos].PLEnd, "end") != 0) {
			PlayMusic(playlist[PLpos].PLEnd);
		}
	}
	PLnext = -1;
}

void MUSImporter::HardEnd()
{
	core->GetAudioDrv()->Stop();
	Playing = false;
	PLpos = 0;
}

/** Switches the current PlayList while playing the current one, return nonzero on error */
int MUSImporter::SwitchPlayList(const ieVariable& name, bool Hard)
{
	if (Playing) {
		//don't do anything if the requested song is already playing
		//this fixed PST's infinite song start
		if (IsCurrentPlayList(name))
			return 0;
		if (Hard) {
			HardEnd();
		} else {
			End();
		}
		//if still playing, then don't insist on trying to open it now
		//either HardEnd stopped it for us, or End marked it for early ending
		if (Playing) {
			PLNameNew = name;
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

void MUSImporter::PlayMusic(const ieVariable& name)
{
	char FName[_MAX_PATH];
	if (name.BeginsWith("mx9000")) { //iwd2
		PathJoin(FName, "mx9000", name, nullptr);
	} else if (name.BeginsWith("mx0000")) { //iwd
		PathJoin(FName, "mx0000", name, nullptr);
	} else if (!name.BeginsWith("SPC")) { //bg2
		char File[_MAX_PATH];
		fmt::format_to(File, "{}{}", PLName, name);
		PathJoin(FName, PLName.c_str(), File, nullptr);
	} else {
		strlcpy(FName, name.c_str(), _MAX_PATH);
	}

	ResourceHolder<SoundMgr> sound = GetResourceHolder<SoundMgr>(FName, manager, true);
	if (sound) {
		int soundID = core->GetAudioDrv()->CreateStream( sound );
		if (soundID == -1) {
			core->GetAudioDrv()->Stop();
		}
	} else {
		core->GetAudioDrv()->Stop();
	}
	Log(MESSAGE, "MUSImporter", "Playing {}...", FName);
}

bool MUSImporter::IsCurrentPlayList(const ieVariable& name) {
	return name == PLName;
}

#include "plugindef.h"

GEMRB_PLUGIN(0x2DCB9E8, "MUS File Importer")
PLUGIN_CLASS(IE_MUS_CLASS_ID, MUSImporter)
END_PLUGIN()
