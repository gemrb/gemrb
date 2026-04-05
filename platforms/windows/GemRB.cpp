// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

// GemRB.cpp : Defines the entry point for the application.

#include "win32def.h"

#include "Interface.h"
#include "PluginMgr.h"

#include "Logging/Loggers/Stdio.h"
#include "Logging/Logging.h"

#include <clocale> //language encoding

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
