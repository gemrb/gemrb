import GemRB

SoundWindow = 0
TextAreaControl = 0

def OnLoad():
	global SoundWindow, TextAreaControl
	GemRB.LoadWindowPack("GUIOPT")
	SoundWindow = GemRB.LoadWindow(11)
	TextAreaControl = GemRB.GetControl(SoundWindow, 16)
	SubtitleButton = GemRB.GetControl(SoundWindow, 20)
	WarCryButton = GemRB.GetControl(SoundWindow, 18)
	StepsButton = GemRB.GetControl(SoundWindow, 19)
	ActionButton = GemRB.GetControl(SoundWindow, 21)
	SelectionButton = GemRB.GetControl(SoundWindow, 57)
	OkButton = GemRB.GetControl(SoundWindow, 24)
	CancelButton = GemRB.GetControl(SoundWindow, 25)
	GemRB.SetText(SoundWindow, TextAreaControl, 18041)
	GemRB.SetText(SoundWindow, OkButton, 11973)
	GemRB.SetText(SoundWindow, CancelButton, 13727)
	GemRB.SetEvent(SoundWindow, SubtitleButton, IE_GUI_BUTTON_ON_PRESS, "SubtitlePress")
	GemRB.SetEvent(SoundWindow, WarCryButton, IE_GUI_BUTTON_ON_PRESS, "WarCryPress")
	GemRB.SetEvent(SoundWindow, StepsButton, IE_GUI_BUTTON_ON_PRESS, "StepsPress")
	GemRB.SetEvent(SoundWindow, ActionButton, IE_GUI_BUTTON_ON_PRESS, "ActionPress")
	GemRB.SetEvent(SoundWindow, SelectionButton, IE_GUI_BUTTON_ON_PRESS, "SelectionPress")
	GemRB.SetEvent(SoundWindow, OkButton, IE_GUI_BUTTON_ON_PRESS, "OkPress")
	GemRB.SetEvent(SoundWindow, CancelButton, IE_GUI_BUTTON_ON_PRESS, "CancelPress")
	GemRB.ShowModal(SoundWindow)
	return
	
def SubtitlePress():
	global SoundWindow, TextAreaControl
	GemRB.SetText(SoundWindow, TextAreaControl, 18015)
	return
	
def WarCryPress():
	global SoundWindow, TextAreaControl
	GemRB.SetText(SoundWindow, TextAreaControl, 18013)
	return
	
def StepsPress():
	global SoundWindow, TextAreaControl
	GemRB.SetText(SoundWindow, TextAreaControl, 18014)
	return
	
def ActionPress():
	global SoundWindow, TextAreaControl
	GemRB.SetText(SoundWindow, TextAreaControl, 18016)
	return
	
def SelectionPress():
	global SoundWindow, TextAreaControl
	GemRB.SetText(SoundWindow, TextAreaControl, 11352)
	return
	
def OkPress():
	global SoundWindow, TextAreaControl
	GemRB.UnloadWindow(SoundWindow)
	GemRB.SetNextScript("GUIOPT7")
	return
	
def CancelPress():
	global SoundWindow, TextAreaControl
	GemRB.UnloadWindow(SoundWindow)
	GemRB.SetNextScript("GUIOPT7")
	return
