import GemRB

MessageWindow = 0
PortraitWindow = 0
ExpandButton = 0
ContractButton = 0
MaxExpand = 2

def OnLoad():
	global MessageWindow, PortraitWindow, ExpandButton, Expand

	GemRB.LoadWindowPack("GUIW")
	ActionsWindow = GemRB.LoadWindow(3)
	PortraitWindow = GemRB.LoadWindow(26)
	PopulatePortraitWindow()
	OptionsWindow = GemRB.LoadWindow(25)
	MessageWindow = GemRB.LoadWindow(4)
	MessageTA = GemRB.GetControl(MessageWindow, 3)
	GemRB.SetTAAutoScroll(MessageWindow, MessageTA, 1)
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
	GemRB.SetVisible(PortraitWindow, 1)
	GemRB.SetVisible(OptionsWindow, 1)
	GemRB.SetVisible(MessageWindow, 1)
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
		else:
			return
			
	GemRB.MoveTAText(MessageWindow, MessageTA, TMessageWindow, TMessageTA)
	GemRB.UnloadWindow(MessageWindow)
	
	Expand = Expand+1
	MessageWindow = TMessageWindow
	
	GemRB.SetVar("MessageWindowSize", Expand)
	UpdateResizeButtons()
	GemRB.UnhideGUI()
	GemRB.SetControlStatus(TMessageWindow,TMessageTA,IE_GUI_CONTROL_FOCUSED);
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
		else:
			return
			
	GemRB.MoveTAText(MessageWindow, MessageTA, TMessageWindow, TMessageTA)
	GemRB.UnloadWindow(MessageWindow)
	
	Expand = Expand-1
	MessageWindow = TMessageWindow
	
	GemRB.SetVar("MessageWindowSize", Expand)
	UpdateResizeButtons()
	GemRB.UnhideGUI()
	if Expand:
		GemRB.SetControlStatus(TMessageWindow,TMessageTA,IE_GUI_CONTROL_FOCUSED);       
	else:	   
		GemRB.SetControlStatus(0,0,IE_GUI_CONTROL_FOCUSED);
	return	  

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

def PopulatePortraitWindow ():
	Window = PortraitWindow

	for i in range (0,5):
		Button = GemRB.GetControl (Window, i)
		GemRB.SetVarAssoc (Window, Button, 'SelectedSingle', i)
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "PortraitButtonOnPress")

		pic = GemRB.GetPlayerPortrait (i+1,1)
		GemRB.SetButtonPicture(Window, Button, pic)
		GemRB.SetButtonFlags(Window, Button, IE_GUI_BUTTON_PICTURE, OP_SET)

