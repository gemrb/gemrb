import GemRB

def OnLoad():
	GemRB.LoadWindowPack("GUIMG")
	ActionsWindow = GemRB.LoadWindow(0)
	PortraitWindow = GemRB.LoadWindow(1)
	OptionsWindow = GemRB.LoadWindow(2)
	GemRB.SetVar("PortraitWindow", PortraitWindow)
	GemRB.SetVar("ActionsWindow", ActionsWindow)
	GemRB.SetVar("OptionsWindow", OptionsWindow)
	GemRB.SetVar("PortraitPosition", 1) #Bottom
	GemRB.SetVar("ActionsPosition", 1) #Bottom
	GemRB.SetVar("OptionsPosition", 1) #Bottom
	
	GemRB.SetVisible(ActionsWindow, 1)
	GemRB.SetVisible(PortraitWindow, 1)
	GemRB.SetVisible(OptionsWindow, 1)
	return