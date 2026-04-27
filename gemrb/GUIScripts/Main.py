# SPDX-FileCopyrightText: 2016 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

# This script sets up the imports for the __main__ module
# we could use this file for other GUIScript initializations as well

import sys
# 2.6+ only, so we ignore failures
sys.dont_write_bytecode = True

# these imports may not be used here
# we include them to ensure they exist when the GUIScript engine initializes
# we also want anything executed using ExecString to have access to the symbols
from GUIDefines import *
from GUIClasses import *
from ie_restype import RES_BAM

import GemRB

def Init():
	# this function is run after the game type is set
	# this is where we would run initializations (even on a per-game type basis)
	
	class stdioWrapper(object):
		def __init__(self, log_level):
			self.log_level = log_level
			self.buffer = ""
		def write(self, message):
			self.buffer += str(message)
			if self.buffer.endswith("\n"):
				self.flush()
		def flush(self):
			out = self.buffer.rstrip("\n")
			if out:
				GemRB.Log(self.log_level, "Python", out)
			self.buffer = ""

	sys.stdout = stdioWrapper(LOG_MESSAGE)
	sys.stderr = stdioWrapper(LOG_ERROR)
	
	print("Python version: " + sys.version)
	
	# create a global scrollbar for the ScrollView to clone from
	# but only if we can (would fail in tests)
	SBArgs = CreateScrollbarARGs ()
	if GemRB.HasResource (SBArgs[0], RES_BAM):
		frame = {'x' : 0, 'y' : 0, 'w' : 0, 'h' : 0}
		sb = GemRB.CreateView (-1, IE_GUI_SCROLLBAR, frame, SBArgs)
		sb.AddAlias ("SBGLOB")

	# tobex invented racetext, but then the ees used it too and changed the layout ...
	# we assume that anyone with tobex installed wants to play with GemRB, so move
	# it away for compatibility
	# Fishing for trouble and Item revisions also install it, though it could be from
	# biffing the tobex file
	if GameCheck.IsBG2 ():
		FixRaceTable ()

# we assume that if a bad racetext.2da file is in a biff, it is also in override
def FixRaceTable ():
	import os
	gamePath = GemRB.GetSystemVariable (SV_GAMEPATH)

	# is there a bad copy in the override?
	path = os.path.join (gamePath, "override")
	path = os.path.join (path, "racetext.2da")
	bad = False
	found = False
	try:
		with open (path, encoding="latin-1") as table:
			found = True
			table.readline()
			table.readline()
			if "STRREF" in table.readline():
				bad = True
	except FileNotFoundError:
		pass
	if bad:
		os.rename (path, path + ".tobex")
	elif not found:
		# the file is ours, compatible or not present
		return

	# however the file might be in a biff as well, so we
	# need to put our own in place
	import shutil
	unhPath = GemRB.GetSystemVariable (SV_UNHARDCODEDPATH)
	ourPath = os.path.join (unhPath, "unhardcoded", "bg2", "racetext.2da")
	try:
		shutil.copy (ourPath, path)
		shutil.copy (ourPath, path + ".gemrb") # so users can easily switch back and forth
	except:
		print("Manually place the GemRB version of racetext.2da into override/")
