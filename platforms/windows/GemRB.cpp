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

#include "win32def.h"
#include <clocale> //language encoding

#include "Interface.h"
#include "Logging/Logging.h"
#include "Logging/Loggers/Stdio.h"
#include "PluginMgr.h"

using namespace GemRB;

void SetupLogging(const CoreSettings& cfg)
{
	if (cfg.Logging) {
		ToggleLogging(cfg.Logging);
	}

	if (cfg.LogColor >= 0 && cfg.LogColor < int(ANSIColor::count)) {
		AddLogWriter(createStdioLogWriter(ANSIColor(cfg.LogColor)));
	} else {
		AddLogWriter(createStdioLogWriter());
	}
}

int main(int argc, char* argv[])
{
	setlocale(LC_ALL, "");

	auto hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD dwMode = 0;
	GetConsoleMode(hConsole, &dwMode);
	SetConsoleMode(hConsole, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

	int ret = GEM_OK;
	try {
		auto cfg = LoadFromArgs(argc, argv);
		SetupLogging(cfg);

		SanityCheck();

		Interface gemrb(std::move(cfg));
		gemrb.Main();
	} catch (CoreInitializationException& cie) {
		Log(FATAL, "Main", "Aborting due to fatal error... {}", cie);
		ret = GEM_ERROR;
	}

	VideoDriver.reset();
	PluginMgr::Get()->RunCleanup();

	ToggleLogging(false); // Windows build will hang if we leave the logging thread running
	SetConsoleMode(hConsole, dwMode);
	return ret;
}
