import GemRB

MessageWindow = 0
PortraitWindow = 0
OptionsWindow = 0
ExpandButton = 0
ContractButton = 0
MaxExpand = 2

def OnLoad():
	global MessageWindow, ExpandButton, Expand
	global PortraitWindow, OptionsWindow

	GemRB.LoadWindowPack("GUIW")
	ActionsWindow = GemRB.LoadWindow(3)
	PortraitWindow = GemRB.LoadWindow(1)
	OptionsWindow = GemRB.LoadWindow(0)
	MessageWindow = GemRB.LoadWindow(4)
	MessageTA = GemRB.GetControl(MessageWindow, 3)
	GemRB.SetTAAutoScroll(MessageWindow, MessageTA, 1)
	Button=GemRB.GetControl(OptionsWindow, 10)
	GemRB.SetEvent(OptionsWindow, Button, IE_GUI_BUTTON_ON_PRESS, "MinimizeOptions")
	Button=GemRB.GetControl(PortraitWindow, 8)
	GemRB.SetEvent(PortraitWindow, Button, IE_GUI_BUTTON_ON_PRESS, "MinimizePortraits")
	Button=GemRB.GetControl(ActionsWindow, 60)
	GemRB.SetEvent(ActionsWindow, Button, IE_GUI_BUTTON_ON_PRESS, "MaximizeOptions")
	Button=GemRB.GetControl(ActionsWindow, 61)
	GemRB.SetEvent(ActionsWindow, Button, IE_GUI_BUTTON_ON_PRESS, "MaximizePortraits")

	GemRB.SetVar("PortraitWindow", PortraitWindow)
	GemRB.SetVar("ActionsWindow", ActionsWindow)
	GemRB.SetVar("OptionsWindow", OptionsWindow)
	GemRB.SetVar("MessageWindow", MessageWindow)
	GemRB.SetVar("PortraitPosition", 2) #Right
	GemRB.SetVar("ActionsPosition", 4) #BottomAdded
	GemRB.SetVar("OptionsPosition", 0) #Left
	GemRB.SetVar("MessagePosition", 4) #BottomAdded
	
	GemRB.SetVar("MessageTextArea", MessageTA)
	GemRB.SetVar("MessageWindowSize", 0)

	UpdateResizeButtons()
	
	GemRB.SetVisible(ActionsWindow, 1)
	GemRB.SetVisible(PortraitWindow, 0)
	GemRB.SetVisible(OptionsWindow, 0)
	GemRB.SetVisible(MessageWindow, 1)
	return

def MinimizeOptions():
	GemRB.HideGUI()
	GemRB.SetVisible(OptionsWindow, 0)
	GemRB.SetVar("OptionsWindow", -1)
	GemRB.UnhideGUI()
	return

def MaximizeOptions():
	GemRB.HideGUI()
	GemRB.SetVar("OptionsWindow", OptionsWindow)
	GemRB.UnhideGUI()
	return

def MinimizePortraits():
	GemRB.HideGUI()
	GemRB.SetVisible(PortraitWindow, 0)
	GemRB.SetVar("PortraitWindow", -1)
	GemRB.UnhideGUI()
	return

def MaximizePortraits():
	GemRB.HideGUI()
	GemRB.SetVar("PortraitWindow", PortraitWindow)
	GemRB.UnhideGUI()
	return

def OnIncreaseSize():
	global MessageWindow, ExpandButton
	
	TMessageWindow = 0
	TMessageTA = 0
	
	GemRB.HideGUI()
	MessageTA = GemRB.GetVar("MessageTextArea")
	Expand = GemRB.GetVar("MessageWindowSize")
	
	if Expand == 0:
		GemRB.LoadWindowPack("GUIW")
		TMessageWindow = GemRB.LoadWindow(12)
		TMessageTA = GemRB.GetControl(TMessageWindow, 1)
		GemRB.SetVar("MessageWindow", TMessageWindow)
		GemRB.SetVar("MessageTextArea", TMessageTA)
		GemRB.SetTAAutoScroll(TMessageWindow, TMessageTA, 1)
	else :
		if Expand == 1:
			GemRB.LoadWindowPack("GUIW")
			TMessageWindow = GemRB.LoadWindow(7)
			TMessageTA = GemRB.GetControl(TMessageWindow, 1)
			GemRB.SetVar("MessageWindow", TMessageWindow)
			GemRB.SetVar("MessageTextArea", TMessageTA)
			GemRB.SetTAAutoScroll(TMessageWindow, TMessageTA, 1)
			
	GemRB.MoveTAText(MessageWindow, MessageTA, TMessageWindow, TMessageTA)
	GemRB.UnloadWindow(MessageWindow)
	
	Expand = Expand+1
	MessageWindow = TMessageWindow
	
	GemRB.SetVar("MessageWindowSize", Expand)
	UpdateResizeButtons()
	GemRB.UnhideGUI()
	return
	
def OnDecreaseSize():
	global MessageWindow, ContractButton
	
	TMessageWindow = 0
	TMessageTA = 0
	
	GemRB.HideGUI()
	MessageTA = GemRB.GetVar("MessageTextArea")
	Expand = GemRB.GetVar("MessageWindowSize")
	
	if Expand == 2:
		GemRB.LoadWindowPack("GUIW")
		TMessageWindow = GemRB.LoadWindow(12)
		TMessageTA = GemRB.GetControl(TMessageWindow, 1)
		GemRB.SetVar("MessageWindow", TMessageWindow)
		GemRB.SetVar("MessageTextArea", TMessageTA)
		GemRB.SetTAAutoScroll(TMessageWindow, TMessageTA, 1)
	else:
		if Expand == 1:
			GemRB.LoadWindowPack("GUIW")
			TMessageWindow = GemRB.LoadWindow(4)
			TMessageTA = GemRB.GetControl(TMessageWindow, 3)
			GemRB.SetVar("MessageWindow", TMessageWindow)
			GemRB.SetVar("MessageTextArea", TMessageTA)
			GemRB.SetTAAutoScroll(TMessageWindow, TMessageTA, 1)
			
	GemRB.MoveTAText(MessageWindow, MessageTA, TMessageWindow, TMessageTA)
	GemRB.UnloadWindow(MessageWindow)
	
	Expand = Expand-1
	MessageWindow = TMessageWindow
	
	GemRB.SetVar("MessageWindowSize", Expand)
	UpdateResizeButtons()
	GemRB.UnhideGUI()
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
