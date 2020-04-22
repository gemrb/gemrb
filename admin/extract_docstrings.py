import GemRB

import inspect
import tempfile
import os

temporary_directory = tempfile.mkdtemp(prefix="GemRB_GUIScript_docstrings_")

for (name, value) in inspect.getmembers(GemRB, inspect.isbuiltin):
	with open(os.path.join(temporary_directory, name), "w") as target_file:
		target_file.write(inspect.getdoc(value))

print temporary_directory
