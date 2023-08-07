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
