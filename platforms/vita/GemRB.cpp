// SPDX-FileCopyrightText: 2020 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

// GemRB.cpp : Defines the entry point for the application.

#include "Interface.h"
#include "VitaLogger.h"

#include <Python.h>
#include <SDL.h>
#include <clocale> //language encoding
#include <psp2/apputil.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/power.h>

// allocating memory for application on Vita
int _newlib_heap_size_user = 344 * 1024 * 1024;
char* vitaArgv[3];
char configPath[25];

void VitaSetArguments(int* argc, char** argv[])
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
	vitaArgv[0] = (char*) "";
	// 0x05 probably corresponds to psla event sent from launcher screen of the app in LiveArea
	if (eventParam.type == 0x05) {
		char buffer[2048];
		memset(buffer, 0, 2048);
		sceAppUtilAppEventParseLiveArea(&eventParam, buffer);
		vitaArgv[1] = (char*) "-c";
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

	SanityCheck();

	//Py_Initialize crashes on Vita otherwise
	Py_NoSiteFlag = 1;
	Py_IgnoreEnvironmentFlag = 1;
	Py_NoUserSiteDirectory = 1;

	try {
		Interface gemrb(LoadFromArgs(argc, argv));
		gemrb.Main();
	} catch (CoreInitializationException& cie) {
		Log(FATAL, "Main", "Aborting due to fatal error... {}", cie);
		ToggleLogging(false);
		return sceKernelExitProcess(GEM_ERROR);
	}

#if SDL_COMPILEDVERSION < SDL_VERSIONNUM(1, 3, 0)
	constexpr int32_t VITA_FULLSCREEN_WIDTH = 960;
	constexpr int32_t VITA_FULLSCREEN_HEIGHT = 544;

	if (width != VITA_FULLSCREEN_WIDTH || height != VITA_FULLSCREEN_HEIGHT) {
		SDL_Rect vitaDestRect;
		vitaDestRect.x = 0;
		vitaDestRect.y = 0;
		vitaDestRect.w = width;
		vitaDestRect.h = height;

		if (VideoDriver->GetFullscreenMode()) {
			const char* optstr = config->GetValueForKey("VitaKeepAspectRatio");
			if (!optstr || atoi(optstr) > 0) {
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
	VideoDriver.reset();

	ToggleLogging(false);

	return sceKernelExitProcess(0);
}
