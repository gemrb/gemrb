import GemRB

def OnLoad():
	GemRB.LoadWindowPack("GUIW")
	ActionsWindow = GemRB.LoadWindow(3)
	PortraitWindow = GemRB.LoadWindow(1)
	OptionsWindow = GemRB.LoadWindow(0)
	TopWindow = GemRB.LoadWindow(2)
	GemRB.SetVar("PortraitWindow", PortraitWindow)
	GemRB.SetVar("ActionsWindow", ActionsWindow)
	GemRB.SetVar("OptionsWindow", OptionsWindow)
	GemRB.SetVar("TopWindow", TopWindow)
	GemRB.SetVar("PortraitPosition", 2) #Right
	GemRB.SetVar("ActionsPosition", 1) #Left
	GemRB.SetVar("OptionsPosition", 0) #Bottom
	GemRB.SetVar("TopWindow", 3) #Top
	
	GemRB.SetVisible(ActionsWindow, 1)
	GemRB.SetVisible(PortraitWindow, 1)
	GemRB.SetVisible(OptionsWindow, 1)
	GemRB.SetVisible(TopWindow, 1)
	return