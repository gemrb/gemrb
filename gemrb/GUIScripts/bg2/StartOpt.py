import GemRB

OptionsWindow = 0

def OnLoad():
	global OptionsWindow
	GemRB.LoadWindowPack("GUIOPT")
	OptionsWindow = GemRB.LoadWindow(13)
	GemRB.SetWindowPos (OptionsWindow, 640, 480, WINDOW_SCALE)
	GemRB.SetWindowFrame (OptionsWindow)
	SoundButton = GemRB.GetControl(OptionsWindow, 8)
	GameButton = GemRB.GetControl(OptionsWindow, 9)
	GraphicButton = GemRB.GetControl(OptionsWindow, 7)
	BackButton = GemRB.GetControl(OptionsWindow, 11)
	GemRB.SetControlStatus(OptionsWindow, SoundButton, IE_GUI_BUTTON_ENABLED)
	GemRB.SetControlStatus(OptionsWindow, GameButton, IE_GUI_BUTTON_ENABLED)
	GemRB.SetControlStatus(OptionsWindow, GraphicButton, IE_GUI_BUTTON_ENABLED)
	GemRB.SetControlStatus(OptionsWindow, BackButton, IE_GUI_BUTTON_ENABLED)
	GemRB.SetText(OptionsWindow, SoundButton, 17164)
	GemRB.SetText(OptionsWindow, GameButton, 17165)
	GemRB.SetText(OptionsWindow, GraphicButton, 17162)
	GemRB.SetText(OptionsWindow, BackButton, 10308)
	GemRB.SetEvent(OptionsWindow, SoundButton, IE_GUI_BUTTON_ON_PRESS, "SoundPress")
	GemRB.SetEvent(OptionsWindow, GameButton, IE_GUI_BUTTON_ON_PRESS, "GamePress")
	GemRB.SetEvent(OptionsWindow, GraphicButton, IE_GUI_BUTTON_ON_PRESS, "GraphicPress")
	GemRB.SetEvent(OptionsWindow, BackButton, IE_GUI_BUTTON_ON_PRESS, "BackPress")
	GemRB.SetVisible(OptionsWindow, 1)
	return
	
def SoundPress():
	GemRB.UnloadWindow(OptionsWindow)
	GemRB.SetNextScript("GUIOPT7")
	return
	
def GamePress():
	GemRB.UnloadWindow(OptionsWindow)
	GemRB.SetNextScript("GUIOPT8")
	return
	
def GraphicPress():
	GemRB.UnloadWindow(OptionsWindow)
	GemRB.SetNextScript("GUIOPT6")
	return
	
def BackPress():
	GemRB.UnloadWindow(OptionsWindow)
	GemRB.SetNextScript("Start")
	return
