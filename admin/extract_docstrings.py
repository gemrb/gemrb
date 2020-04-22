# GemRB - Infinity Engine Emulator
# Copyright (C) 2020 The GemRB Project
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

import GemRB

import inspect
import tempfile
import os

temporary_directory = tempfile.mkdtemp(prefix="GemRB_GUIScript_docstrings_")

for (name, value) in inspect.getmembers(GemRB, inspect.isbuiltin):
	with open(os.path.join(temporary_directory, name), "w") as target_file:
		target_file.write(inspect.getdoc(value))

print temporary_directory
