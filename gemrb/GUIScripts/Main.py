# This script sets up the imports for the __main__ module
# we could use this file for other GUIScript initializations as well

import sys
# 2.6+ only, so we ignore failures
sys.dont_write_bytecode = True

import GemRB

from GUIDefines import *
from GUIClasses import *

def Init():
	# this function is run after the game type is set
	# this is where we would run initializations (even on a per-game type basis)
	pass
