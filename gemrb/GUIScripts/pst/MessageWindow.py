import GemRB

MessageWindow = 0
ActionsWindow = 0
PortraitWindow = 0
OptionsWindow = 0
CloseButton = 0
MaxExpand = 1

def OnLoad():
	global MessageWindow, ActionsWindow, PortraitWindow, OptionsWindow, CloseButton
	GemRB.LoadWindowPack("GUIWORLD")
	ActionsWindow = GemRB.LoadWindow(0)
	PortraitWindow = GemRB.LoadWindow(1)
	OptionsWindow = GemRB.LoadWindow(2)
	MessageWindow = GemRB.LoadWindow(7)
	MessageTA = GemRB.GetControl(MessageWindow, 1)
	GemRB.SetTAAutoScroll(MessageWindow, MessageTA, 1)
	GemRB.SetVar("PortraitWindow", PortraitWindow)
	GemRB.SetVar("ActionsWindow", ActionsWindow)
	GemRB.SetVar("OptionsWindow", OptionsWindow)
	GemRB.SetVar("MessageWindow", -1)
	GemRB.SetVar("PortraitPosition", 1) #Bottom
	GemRB.SetVar("ActionsPosition", 1) #Bottom
	GemRB.SetVar("OptionsPosition", 1) #Bottom
	GemRB.SetVar("MessagePosition", 1) #Bottom
	
	GemRB.SetVar("MessageTextArea", MessageTA)
	GemRB.SetVar("MessageWindowSize", 0)
	
	CloseButton=GemRB.GetControl(MessageWindow, 0)

	UpdateResizeButtons()
	
	GemRB.SetVisible(ActionsWindow, 1)
	GemRB.SetVisible(PortraitWindow, 1)
	GemRB.SetVisible(OptionsWindow, 1)
	GemRB.SetVisible(MessageWindow, 0)
	return
	
def OnIncreaseSize():
	global MessageWindow
	
	MessageTA = GemRB.GetVar("MessageTextArea")
	Expand = GemRB.GetVar("MessageWindowSize")
	
	if Expand == 1:
		return
	
	GemRB.HideGUI()
	
	if Expand == 0:
		GemRB.SetVar("MessageWindow", MessageWindow);
		GemRB.SetVar("PortraitWindow", -1)
		GemRB.SetVar("ActionsWindow", -1)
		GemRB.SetVar("OptionsWindow", -1)
		GemRB.SetVisible(MessageWindow, 1)
		GemRB.SetVisible(PortraitWindow, 0)
		GemRB.SetVisible(ActionsWindow, 0)
		GemRB.SetVisible(OptionsWindow, 0)
			
	Expand = 1
	
	GemRB.SetVar("MessageWindowSize", Expand)
	UpdateResizeButtons()
	GemRB.UnhideGUI()
	return
	
def OnDecreaseSize():
	global MessageWindow
	
	MessageTA = GemRB.GetVar("MessageTextArea")
	Expand = GemRB.GetVar("MessageWindowSize")
	
	if Expand == 0:
		return
	
	GemRB.HideGUI()
	
	if Expand == 0:
		GemRB.SetVar("MessageWindow", -1);
		GemRB.SetVar("PortraitWindow", PortraitWindow)
		GemRB.SetVar("ActionsWindow", ActionsWindow)
		GemRB.SetVar("OptionsWindow", OptionsWindow)
		GemRB.SetVisible(MessageWindow, 0)
		GemRB.SetVisible(PortraitWindow, 1)
		GemRB.SetVisible(ActionsWindow, 1)
		GemRB.SetVisible(OptionsWindow, 1)
			
	Expand = 0
	
	GemRB.SetVar("MessageWindowSize", Expand)
	UpdateResizeButtons()
	GemRB.UnhideGUI()
	return
	
def UpdateResizeButtons():
	global MessageWindow, CloseButton
	
	GemRB.SetEvent(MessageWindow, CloseButton, IE_GUI_BUTTON_ON_PRESS, "OnDecreaseSize")
	return
