#include "Python.h"
#include "GUIScript.h"

static void run_script (const char * script_file_path) {
	GemRB::GUIScript * interpreter = new GemRB::GUIScript;
	interpreter -> Init_non_specific ();
	interpreter -> ExecFile (script_file_path);
	delete interpreter;
}

int main (int argc, char ** argv) {
	if (argc != 2) {
		exit (1);
	} else {
		run_script (argv[1]);
		/* Unfortunately, ExecFile returns silently when the file cannot be read
		into a memory buffer, so this branch may exit successfully even when the
		code was not run. Also, it will segfault if there is an error in the script. */
		exit (0);
	}
}
