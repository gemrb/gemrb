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

#include <psp2/kernel/processmgr.h>
#include <psp2/power.h>
#include <psp2/apputil.h> 

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

void main(int argc, char* argv[])
{
	scePowerSetArmClockFrequency(444);
	scePowerSetBusClockFrequency(222);
	scePowerSetGpuClockFrequency(222);
	scePowerSetGpuXbarClockFrequency(166);

	// Selecting game config from init params
	VitaSetArguments(&argc, &argv);

	setlocale(LC_ALL, "");

	Interface::SanityCheck(VERSION_GEMRB);

	core = new Interface();
	CFGConfig* config = new CFGConfig(argc, argv);
	if (core->Init( config ) == GEM_ERROR) {
		delete config;
		delete( core );
		InitializeLogging();
		Log(MESSAGE, "Main", "Aborting due to fatal error...");
		ShutdownLogging();
		sceKernelExitProcess(0);
		return -1;
	}
	InitializeLogging();
	delete config;

	core->Main();
	delete( core );
	ShutdownLogging();

	sceKernelExitProcess(0);
}
