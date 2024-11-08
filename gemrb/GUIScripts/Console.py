
from __future__ import print_function # so that print(blah) will work in console

from ie_stats import *
from GUIDefines import *
import GemRB
from GemRB import * # we dont want to have to type 'GemRB.Command' instead of simply 'Command'

def OnLoad():
	consoleWin = GemRB.LoadWindow(0, "console", WINDOW_TOP|WINDOW_HCENTER)
	consoleWin.SetFlags(IE_GUI_VIEW_INVISIBLE | WF_BORDERLESS | WF_ALPHA_CHANNEL, OP_OR)
	consoleWin.SetFlags(WF_DESTROY_ON_CLOSE, OP_NAND);
	consoleWin.AddAlias("WIN_CON")
	consoleWin.SetBackground({'r' : 0, 'g' : 0, 'b' : 0, 'a' : 200})
	
	histLabel = consoleWin.GetControl(2)
	histLabel.SetText ("History:")
	hist = consoleWin.GetControl(3)

	console = consoleWin.ReplaceSubview (0, IE_GUI_CONSOLE, hist)
	console.AddAlias ("CONSOLE_CTL", 1);
	
	consoleWin.SetAction(console.Focus, ACTION_WINDOW_FOCUS_GAINED)
	
	consoleOut = consoleWin.GetControl(1)
	consoleOut.SetFlags (IE_GUI_TEXTAREA_AUTOSCROLL)
	consoleOut.AddAlias("CONSOLE", 1);
	consoleOut.SetBackground({'r' : 0, 'g' : 0, 'b' : 0, 'a' : 200})
	
def ToggleConsole():
	consoleWin = GemRB.GetView("WIN_CON")
	if consoleWin is None: # if outside of a game
		return

	if consoleWin.IsVisible():
		consoleWin.Close()
	else:
		consoleWin.Focus()

# /handy/ shorthand forms
def gps(stat, base=0):
	print (GemRB.GetPlayerStat(GemRB.GameGetFirstSelectedPC(), stat, base))

def sps(stat, value, pcf=1):
	GemRB.SetPlayerStat(GemRB.GameGetFirstSelectedPC(), stat, value, pcf)

def mta(area):
	GemRB.MoveToArea(area)

def debug(level):
	GemRB.ConsoleWindowLog (level)
	
def cast(spellRes):
	GemRB.SpellCast (GemRB.GameGetFirstSelectedPC (), -3, 0, spellRes)

def cc(cre, px=-1, py=-1):
	GemRB.CreateCreature(GemRB.GameGetFirstSelectedPC(), cre, px, py)

def ci(item, c0=1, c1=0, c2=0, slot=-1):
	GemRB.CreateItem(GemRB.GameGetFirstSelectedPC(), item, slot, c0, c1, c2)

def cv(var, context="GLOBAL"):
	GemRB.CheckVar(var, context)

def ex(cmd, runner = 0):
	GemRB.ExecuteString(cmd, runner)

def ev(trigger):
	GemRB.EvaluateString(trigger)

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
					con.Append("[color=ffffff]" + cmd + ": [/color][color=00ff00]" + out + "[/color]\n")
				self.buffer = ""
	
	try:
		stdout = sys.stdout

		if con:
			sys.stdout = OutputCapture(stdout)

		locals = {} # we dont want to expose our locals
		modend = cmd.find('.')
		if modend > -1 and modend < len("ex('ActionOverride(Pla"): # Point arguments use a dot
			import importlib
			importlib.invalidate_caches()
			modname = cmd[0:modend]
			locals[modname] = importlib.import_module(modname)
		return eval(cmd, globals(), locals)
	except (SyntaxError, NameError, TypeError, ZeroDivisionError) as error:
		if con:
			con.Append("[color=ffffff]" + cmd + ": [/color][color=ff0000]" + str(error) + "[/color]\n")
			
		sys.stderr.write(error)
	finally:
		sys.stdout = stdout
