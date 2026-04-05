// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

// GemRB.cpp : Defines the entry point for the application.

#include "Interface.h"
#include "PluginMgr.h"

#include "Logging/Loggers/Stdio.h"
#include "Logging/Logging.h"

#include <clocale> //language encoding

using namespace GemRB;

int main(int argc, char* argv[])
{
	setlocale(LC_ALL, "");
#ifdef HAVE_SETENV
	setenv("SDL_VIDEO_X11_WMCLASS", argv[0], 0);

	// Prevent fragmentation of the heap by malloc (glibc).
	// The default threshold is 128*1024, which can result in a large memory usage
	// due to fragmentation since we use a lot of small objects. On the other hand
	// if the threshold is too low, free() starts to permanently ask the kernel
	// about shrinking the heap.
	#if defined(HAVE_UNISTD_H)
	int pagesize = sysconf(_SC_PAGESIZE);
	#else
	int pagesize = 4 * 1024;
	#endif
	setenv("MALLOC_TRIM_THRESHOLD_", fmt::format("{}", 5 * pagesize), 1);
#endif

	try {
		auto cfg = LoadFromArgs(argc, argv);

		if (cfg.Logging) {
			ToggleLogging(cfg.Logging);
		}

		if (cfg.LogColor >= 0 && cfg.LogColor < int(ANSIColor::count)) {
			AddLogWriter(createStdioLogWriter(ANSIColor(cfg.LogColor)));
		} else {
			AddLogWriter(createStdioLogWriter());
		}

		SanityCheck();

		Interface gemrb(std::move(cfg));
		gemrb.Main();
	} catch (CoreInitializationException& cie) {
		Log(FATAL, "Main", "Aborting due to fatal error... {}", cie);
		return GEM_ERROR;
	}

	VideoDriver.reset();
	PluginMgr::Get()->RunCleanup();

	return GEM_OK;
}
