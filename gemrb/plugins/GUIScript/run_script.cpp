/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2020 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "GUIScript.h"
#include "Interface.h"
#include "InterfaceConfig.h"

#include "Python.h"

static void run_script(const char *script_file_path) {

	/* There are global variables `core` and `gs` declared in headers. `gs`
	   is automagically assigned when the object is constructed. */
	GemRB::InterfaceConfig *config = new GemRB::InterfaceConfig(0, NULL);
	GemRB::Interface *interface = new GemRB::Interface();
	GemRB::core = interface;
	interface->Init(config);
	GemRB::GUIScript *interpreter = new GemRB::GUIScript();
	/* From now on, `gs` = `interpreter` is well-defined. */

	interpreter->Init();
	interpreter->ExecFile(script_file_path);

	delete interpreter;
	delete interface;
	delete config;
}

int main(int argc, char **argv) {
	if (argc != 2) {
		exit(1);
	} else {
		run_script(argv[1]);
		/* Unfortunately, ExecFile returns silently when the file cannot
		be read into a memory buffer, so this branch may exit
		successfully even when the code was not run. Also, it will
		segfault if there is an error in the script. */
		exit(0);
	}
}
