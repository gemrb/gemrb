#character generation, name (GUICG5)
import GemRB

NameWindow = 0

def OnLoad():
	global NameWindow
	
	GemRB.LoadWindowPack("GUICG")
	NameWindow = GemRB.LoadWindow(5)

	NameField = GemRB.GetControl(NameWindow, 2)
	return

def BackPress():
	GemRB.UnloadWindow(NameWindow)
	GemRB.SetNextScript("CharGen8")
	return

def NextPress():
        GemRB.UnloadWindow(NameWindow)
	GemRB.SetNextScript("CharGen2") #Before race
	return

