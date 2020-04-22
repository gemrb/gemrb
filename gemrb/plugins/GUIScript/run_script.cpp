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
