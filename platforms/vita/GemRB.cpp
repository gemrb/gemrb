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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

// GemRB.cpp : Defines the entry point for the application.

#include <clocale> //language encoding

#include "Interface.h"
#include "VitaLogger.h"

#include <psp2/kernel/processmgr.h>
#include <psp2/power.h>
#include <psp2/apputil.h>

#include <Python.h>
#include <SDL.h>

// allocating memory for application on Vita
int _newlib_heap_size_user = 344 * 1024 * 1024;
char *vitaArgv[3];
char configPath[25];

void VitaSetArguments(int *argc, char **argv[])
{
	SceAppUtilInitParam appUtilParam;
	SceAppUtilBootParam appUtilBootParam;
	memset(&appUtilParam, 0, sizeof(SceAppUtilInitParam));
	memset(&appUtilBootParam, 0, sizeof(SceAppUtilBootParam));
	sceAppUtilInit(&appUtilParam, &appUtilBootParam);
	SceAppUtilAppEventParam eventParam;
	memset(&eventParam, 0, sizeof(SceAppUtilAppEventParam));
	sceAppUtilReceiveAppEvent(&eventParam);
	int vitaArgc = 1;
	vitaArgv[0] = (char*)"";
	// 0x05 probably corresponds to psla event sent from launcher screen of the app in LiveArea
	if (eventParam.type == 0x05) {
		char buffer[2048];
		memset(buffer, 0, 2048);
		sceAppUtilAppEventParseLiveArea(&eventParam, buffer);
		vitaArgv[1] = (char*)"-c";
		sprintf(configPath, "ux0:data/GemRB/%s.cfg", buffer);
		vitaArgv[2] = configPath;
		vitaArgc = 3;
	}
	*argc = vitaArgc;
	*argv = vitaArgv;
}

using namespace GemRB;

int main(int argc, char* argv[])
{
	scePowerSetArmClockFrequency(444);
	scePowerSetBusClockFrequency(222);
	scePowerSetGpuClockFrequency(222);
	scePowerSetGpuXbarClockFrequency(166);
	
	mallopt(M_TRIM_THRESHOLD, 20 * 1024);

	// Selecting game config from init params
	VitaSetArguments(&argc, &argv);

	setlocale(LC_ALL, "");
	
	AddLogWriter(createVitaLogger());
	ToggleLogging(true);

	Interface::SanityCheck(VERSION_GEMRB);
	
	//Py_Initialize crashes on Vita otherwise
	Py_NoSiteFlag = 1;
	Py_IgnoreEnvironmentFlag = 1;
	Py_NoUserSiteDirectory = 1;

	core = new Interface();
	CFGConfig* config = new CFGConfig(argc, argv);

	if (core->Init( config ) == GEM_ERROR) {
		delete config;
		delete( core );
		Log(MESSAGE, "Main", "Aborting due to fatal error...");
		ToggleLogging(false);
		return sceKernelExitProcess(-1);
	}
	
#if SDL_COMPILEDVERSION < SDL_VERSIONNUM(1,3,0)
	constexpr int32_t VITA_FULLSCREEN_WIDTH = 960;
	constexpr int32_t VITA_FULLSCREEN_HEIGHT = 544;
	
	if (width != VITA_FULLSCREEN_WIDTH || height != VITA_FULLSCREEN_HEIGHT)	{
		SDL_Rect vitaDestRect;
		vitaDestRect.x = 0;
		vitaDestRect.y = 0;
		vitaDestRect.w = width;
		vitaDestRect.h = height;

		if (core->GetVideoDriver()->GetFullscreenMode()) {
			const char* optstr = config->GetValueForKey("VitaKeepAspectRatio");
			if (optstr && atoi(optstr) > 0) {
				if ((static_cast<float>(VITA_FULLSCREEN_WIDTH) / VITA_FULLSCREEN_HEIGHT) >= (static_cast<float>(width) / height)) {
					float scale = static_cast<float>(VITA_FULLSCREEN_HEIGHT) / height;
					vitaDestRect.w = width * scale;
					vitaDestRect.h = VITA_FULLSCREEN_HEIGHT;
					vitaDestRect.x = (VITA_FULLSCREEN_WIDTH - vitaDestRect.w) / 2;
				} else {
					float scale = static_cast<float>(VITA_FULLSCREEN_WIDTH) / width;
					vitaDestRect.w = VITA_FULLSCREEN_WIDTH;
					vitaDestRect.h = height * scale;
					vitaDestRect.y = (VITA_FULLSCREEN_HEIGHT - vitaDestRect.h) / 2;
				}
			} else {
				vitaDestRect.w = VITA_FULLSCREEN_WIDTH;
				vitaDestRect.h = VITA_FULLSCREEN_HEIGHT;
			}
			
			SDL_SetVideoModeBilinear(true);
		} else {
			//center game area
			vitaDestRect.x = (VITA_FULLSCREEN_WIDTH - width) / 2;
			vitaDestRect.y = (VITA_FULLSCREEN_HEIGHT - height) / 2;
		}

		SDL_SetVideoModeScaling(vitaDestRect.x, vitaDestRect.y, vitaDestRect.w, vitaDestRect.h);
	}
#endif
	delete config;

	core->Main();
	delete( core );
	ToggleLogging(false);

	return sceKernelExitProcess(0);
}
