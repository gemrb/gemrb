import GemRB

MessageWindow = 0
ExpandButton = 0
Expand = 0

def OnLoad():
	global MessageWindow, ExpandButton, Expand
	GemRB.LoadWindowPack("GUIW")
	ActionsWindow = GemRB.LoadWindow(3)
	PortraitWindow = GemRB.LoadWindow(1)
	OptionsWindow = GemRB.LoadWindow(0)
	MessageWindow = GemRB.LoadWindow(4)
	MessageTA = GemRB.GetControl(MessageWindow, 3)
	GemRB.SetVar("PortraitWindow", PortraitWindow)
	GemRB.SetVar("ActionsWindow", ActionsWindow)
	GemRB.SetVar("OptionsWindow", OptionsWindow)
	GemRB.SetVar("MessageWindow", MessageWindow)
	GemRB.SetVar("PortraitPosition", 2) #Right
	GemRB.SetVar("ActionsPosition", 4) #BottomAdded
	GemRB.SetVar("OptionsPosition", 0) #Left
	GemRB.SetVar("MessagePosition", 4) #BottomAdded
	
	GemRB.SetVar("MessageTextArea", MessageTA)
	
	if Expand == 0:
		ExpandButton = GemRB.GetControl(MessageWindow, 2)
		GemRB.SetEvent(MessageWindow, ExpandButton, IE_GUI_BUTTON_ON_PRESS, "OnIncreaseSize")
	
	GemRB.SetVisible(ActionsWindow, 1)
	GemRB.SetVisible(PortraitWindow, 1)
	GemRB.SetVisible(OptionsWindow, 1)
	GemRB.SetVisible(MessageWindow, 1)
	return
	
def OnIncreaseSize():
	global MessageWindow, ExpandButton, Expand
	if Expand == 1:
		return
	GemRB.ExecuteString('HideGUI()')
	GemRB.UnloadWindow(MessageWindow)
	GemRB.LoadWindowPack("GUIW")
	MessageWindow = GemRB.LoadWindow(12)
	MessageTA = GemRB.GetControl(MessageWindow, 1)
	GemRB.SetVar("MessageWindow", MessageWindow)
	GemRB.SetVar("MessageTextArea", MessageTA)
	GemRB.ExecuteString('UnhideGUI()')
	return