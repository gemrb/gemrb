import GemRB

OptionsWindow = 0

def OnLoad():
	global OptionsWindow
	GemRB.LoadWindowPack("GUIOPT", 640, 480)
	OptionsWindow = GemRB.LoadWindowObject(13)
	OptionsWindow.SetFrame ()
	SoundButton = OptionsWindow.GetControl(8)
	GameButton = OptionsWindow.GetControl(9)
	GraphicButton = OptionsWindow.GetControl(7)
	BackButton = OptionsWindow.GetControl(11)
	SoundButton.SetStatus(IE_GUI_BUTTON_ENABLED)
	GameButton.SetStatus(IE_GUI_BUTTON_ENABLED)
	GraphicButton.SetStatus(IE_GUI_BUTTON_ENABLED)
	BackButton.SetStatus(IE_GUI_BUTTON_ENABLED)
	SoundButton.SetText(17164)
	GameButton.SetText(17165)
	GraphicButton.SetText(17162)
	BackButton.SetText(10308)
	SoundButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "SoundPress")
	GameButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "GamePress")
	GraphicButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "GraphicPress")
	BackButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "BackPress")
	BackButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)
	OptionsWindow.SetVisible(1)
	return
	
def SoundPress():
	if OptionsWindow:
		OptionsWindow.Unload()
	GemRB.SetNextScript("GUIOPT7")
	return
	
def GamePress():
	if OptionsWindow:
		OptionsWindow.Unload()
	GemRB.SetNextScript("GUIOPT8")
	return
	
def GraphicPress():
	if OptionsWindow:
		OptionsWindow.Unload()
	GemRB.SetNextScript("GUIOPT6")
	return
	
def BackPress():
	if OptionsWindow:
		OptionsWindow.Unload()
	GemRB.SetNextScript("Start")
	return
