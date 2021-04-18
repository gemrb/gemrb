
from __future__ import print_function # so that print(blah) will work in console

from ie_stats import *
from GUIDefines import *
import GemRB
from GemRB import * # we dont want to have to type 'GemRB.Command' instead of simply 'Command'

# /handy/ shorthand forms
def gps(stat, base=0):
	print (GemRB.GetPlayerStat(GemRB.GameGetFirstSelectedPC(), stat, base))

def sps(stat, value, pcf=1):
	GemRB.SetPlayerStat(GemRB.GameGetFirstSelectedPC(), stat, value, pcf)

def mta(area):
	GemRB.MoveToArea(area)

def debug(level):
	GemRB.ConsoleWindowLog (level)

def cc(cre, px=-1, py=-1):
	GemRB.CreateCreature(GemRB.GameGetFirstSelectedPC(), cre, px, py)

def ci(item, slot=-1, c0=1, c1=0, c2=0):
	GemRB.CreateItem(GemRB.GameGetFirstSelectedPC(), item, slot, c0, c1, c2)

def cv(var, context="GLOBAL"):
	GemRB.CheckVar(var, context)

def ex(cmd):
	GemRB.ExecuteString(cmd)

# the actual function that the GemRB::Console calls
def Exec(cmd):
	import sys
	
	con = GemRB.GetView("CONSOLE", 1)
	
	class OutputCapture(object):
		def __init__(self, out):
			self.out = out
			self.buffer = ""
		
		def write(self, message):
			self.out.write(message)
			self.buffer += str(message)
			if self.buffer.endswith("\n"):
				out = self.buffer
				if out:
					con.Append(cmd + ": [color=00ff00]" + out + "[/color]\n")
				self.buffer = ""
	
	try:
		stdout = sys.stdout

		if con:
			sys.stdout = OutputCapture(stdout)

		return eval(cmd)
	except (SyntaxError, NameError, TypeError, ZeroDivisionError) as error:
		if con:
			con.Append(cmd + ": [color=ff0000]" + str(error) + "[/color]\n")
			
		sys.stderr.write(error)
	finally:
		sys.stdout = stdout
