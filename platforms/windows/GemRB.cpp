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

#include "win32def.h" // logging
#include <clocale> //language encoding

#include "Interface.h"
#include "Win32Console.h"

using namespace GemRB;

int main(int argc, char* argv[])
{
	AddLogWriter(createWin32ConsoleLogger());
	ToggleLogging(true);

	setlocale(LC_ALL, "");

	Interface::SanityCheck(VERSION_GEMRB);

	core = new Interface();
	CFGConfig* config = new CFGConfig(argc, argv);

	if (core->Init( config ) == GEM_ERROR) {
		delete config;
		delete( core );
		Log(MESSAGE, "Main", "Aborting due to fatal error...");

		ToggleLogging(false); // Windows build will hang if we leave the logging thread running
		return -1;
	}
	delete config;

	core->Main();
	delete( core );

	ToggleLogging(false); // Windows build will hang if we leave the logging thread running
	return 0;
}
