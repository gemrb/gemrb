import GemRB

OptionsWindow = 0

def OnLoad():
	global OptionsWindow
	GemRB.LoadWindowPack("GUIOPT")
	OptionsWindow = GemRB.LoadWindow(12)
	SoundButton = GemRB.GetControl(OptionsWindow, 8)
	GameButton = GemRB.GetControl(OptionsWindow, 9)
	GraphicButton = GemRB.GetControl(OptionsWindow, 7)
	BackButton = GemRB.GetControl(OptionsWindow, 11)
	GemRB.SetControlStatus(OptionsWindow, SoundButton, 0x00000000);
	GemRB.SetControlStatus(OptionsWindow, GameButton, 0x00000000);
	GemRB.SetControlStatus(OptionsWindow, GraphicButton, 0x00000000);
	GemRB.SetControlStatus(OptionsWindow, BackButton, 0x00000000);
	GemRB.SetText(OptionsWindow, SoundButton, 17164)
	GemRB.SetText(OptionsWindow, GameButton, 17165)
	GemRB.SetText(OptionsWindow, GraphicButton, 17162)
	GemRB.SetText(OptionsWindow, BackButton, 10308)
	GemRB.SetEvent(OptionsWindow, SoundButton, 0x00000000, "SoundPress")
	GemRB.SetEvent(OptionsWindow, GameButton, 0x00000000, "GamePress")
	GemRB.SetEvent(OptionsWindow, GraphicButton, 0x00000000, "GraphicPress")
	GemRB.SetEvent(OptionsWindow, BackButton, 0x00000000, "BackPress")
	GemRB.SetVisible(OptionsWindow, 1)
	return
	
def SoundPress():
	global OptionsWindow
	GemRB.UnloadWindow(OptionsWindow)
	GemRB.SetNextScript("GUIOPT7")
	return
	
def GamePress():
	global OptionsWindow
	GemRB.UnloadWindow(OptionsWindow)
	GemRB.SetNextScript("GUIOPT8")
	return
	
def GraphicPress():
	global OptionsWindow
	GemRB.UnloadWindow(OptionsWindow)
	GemRB.SetNextScript("GUIOPT6")
	return
	
def BackPress():
	global OptionsWindow
	GemRB.UnloadWindow(OptionsWindow)
	GemRB.SetNextScript("Start")
	return