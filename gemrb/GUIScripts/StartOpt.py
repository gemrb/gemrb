import GemRB

OptionsWindow = 0

def OnLoad():
	global OptionsWindow
	GemRB.LoadWindowPack("START")
	OptionsWindow = GemRB.LoadWindow(0)
	SoundButton = GemRB.GetControl(OptionsWindow, 0)
	Disabled1Button = GemRB.GetControl(OptionsWindow, 3)
	GameButton = GemRB.GetControl(OptionsWindow, 4)
	GraphicButton = GemRB.GetControl(OptionsWindow, 1)
	Disabled2Button = GemRB.GetControl(OptionsWindow, 2)
	BackButton = GemRB.GetControl(OptionsWindow, 5)
	GemRB.SetControlStatus(OptionsWindow, Disabled1Button, 0x00000003);
	GemRB.SetControlStatus(OptionsWindow, Disabled2Button, 0x00000003);
	GemRB.SetControlStatus(OptionsWindow, SoundButton, 0x00000000);
	GemRB.SetControlStatus(OptionsWindow, GameButton, 0x00000000);
	GemRB.SetControlStatus(OptionsWindow, GraphicButton, 0x00000000);
	GemRB.SetControlStatus(OptionsWindow, BackButton, 0x00000000);
	GemRB.SetText(OptionsWindow, Disabled1Button, "")
	GemRB.SetText(OptionsWindow, Disabled2Button, "")
	GemRB.SetText(OptionsWindow, SoundButton, 17164)
	GemRB.SetText(OptionsWindow, GameButton, 17165)
	GemRB.SetText(OptionsWindow, GraphicButton, 17162)
	GemRB.SetText(OptionsWindow, BackButton, 10308)
	GemRB.SetEvent(OptionsWindow, SoundButton, 0x00000000, "SoundPress")
	GemRB.SetEvent(OptionsWindow, GameButton, 0x00000000, "GamePress")
	GemRB.SetEvent(OptionsWindow, GraphicButton, 0x00000000, "GraphicPress")
	GemRB.SetEvent(OptionsWindow, BackButton, 0x00000000, "BackPress")
	GemRB.SetEvent(OptionsWindow, Disabled1Button, 0x00000000, "")
	GemRB.SetEvent(OptionsWindow, Disabled2Button, 0x00000000, "")
	GemRB.SetVisible(OptionsWindow, 1)
	return
	
def SoundPress():
	global OptionsWindow
	GemRB.SetNextScript("GUIOPT7")
	return
	
def GamePress():
	global OptionsWindow
	GemRB.SetNextScript("GUIOPT8")
	return
	
def GraphicPress():
	global OptionsWindow
	GemRB.SetNextScript("GUIOPT6")
	return
	
def BackPress():
	global OptionsWindow
	GemRB.SetNextScript("Start")
	return