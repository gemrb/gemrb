import GemRB

MessageWindow = 0
ExpandButton = 0
ContractButton = 0
MaxExpand = 2

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
	
	UpdateResizeButtons()
	
	GemRB.SetVisible(ActionsWindow, 1)
	GemRB.SetVisible(PortraitWindow, 1)
	GemRB.SetVisible(OptionsWindow, 1)
	GemRB.SetVisible(MessageWindow, 1)
	return
	
def OnIncreaseSize():
	global MessageWindow, ExpandButton
	GemRB.ExecuteString('HideGUI()')
	GemRB.UnloadWindow(MessageWindow)
	Expand = GemRB.GetVar("MessageWindowSize")
	if Expand == 0:
		GemRB.LoadWindowPack("GUIW")
		MessageWindow = GemRB.LoadWindow(12)
		MessageTA = GemRB.GetControl(MessageWindow, 1)
		GemRB.SetVar("MessageWindow", MessageWindow)
		GemRB.SetVar("MessageTextArea", MessageTA)
	else :
		if Expand == 1:
			GemRB.LoadWindowPack("GUIW")
			MessageWindow = GemRB.LoadWindow(7)
			MessageTA = GemRB.GetControl(MessageWindow, 1)
			GemRB.SetVar("MessageWindow", MessageWindow)
			GemRB.SetVar("MessageTextArea", MessageTA)
	Expand = Expand+1
	GemRB.SetVar("MessageWindowSize", Expand)
	UpdateResizeButtons()
	GemRB.ExecuteString('UnhideGUI()')
	return
	
def OnDecreaseSize():
	global MessageWindow, ContractButton
	GemRB.ExecuteString('HideGUI()')
	GemRB.UnloadWindow(MessageWindow)
	Expand = GemRB.GetVar("MessageWindowSize")
	if Expand == 2:
		GemRB.LoadWindowPack("GUIW")
		MessageWindow = GemRB.LoadWindow(12)
		MessageTA = GemRB.GetControl(MessageWindow, 1)
		GemRB.SetVar("MessageWindow", MessageWindow)
		GemRB.SetVar("MessageTextArea", MessageTA)
	else:
		if Expand == 1:
			GemRB.LoadWindowPack("GUIW")
			MessageWindow = GemRB.LoadWindow(4)
			MessageTA = GemRB.GetControl(MessageWindow, 3)
			GemRB.SetVar("MessageWindow", MessageWindow)
			GemRB.SetVar("MessageTextArea", MessageTA)
	Expand = Expand-1
	GemRB.SetVar("MessageWindowSize", Expand)
	UpdateResizeButtons()
	GemRB.ExecuteString('UnhideGUI()')
	return
	
def UpdateResizeButtons():
	global MessageWindow, ExpandButton, ContractButton
	Expand = GemRB.GetVar("MessageWindowSize")
	
	if Expand == 0:
		ExpandButton = GemRB.GetControl(MessageWindow, 2)
		GemRB.SetEvent(MessageWindow, ExpandButton, IE_GUI_BUTTON_ON_PRESS, "OnIncreaseSize")
	else:
		if Expand == 1:
			ExpandButton = GemRB.GetControl(MessageWindow, 0)
			GemRB.SetEvent(MessageWindow, ExpandButton, IE_GUI_BUTTON_ON_PRESS, "OnIncreaseSize")
			ContractButton = GemRB.GetControl(MessageWindow, 3)
			GemRB.SetEvent(MessageWindow, ContractButton, IE_GUI_BUTTON_ON_PRESS, "OnDecreaseSize")
		else:
			ContractButton = GemRB.GetControl(MessageWindow, 0)
			GemRB.SetEvent(MessageWindow, ContractButton, IE_GUI_BUTTON_ON_PRESS, "OnDecreaseSize")
	return