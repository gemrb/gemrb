#character sounds
import GemRB

SoundWindow = 0
TextAreaControl = 0

def OnLoad():
	global SoundWindow, TextAreaControl
	GemRB.LoadWindowPack("GUIOPT")
	SoundWindow = GemRB.LoadWindow(12)
	GemRB.SetWindowPos (SoundWindow, 640, 480, WINDOW_SCALE)
	TextAreaControl = GemRB.GetControl(SoundWindow, 16)
	SubtitleButton = GemRB.GetControl(SoundWindow, 20)
	SubtitleButtonB = GemRB.GetControl(SoundWindow, 5)
	WarCryButton = GemRB.GetControl(SoundWindow, 18)
	WarCryButtonB = GemRB.GetControl(SoundWindow, 6)
	StepsButton = GemRB.GetControl(SoundWindow, 19)
	StepsButtonB = GemRB.GetControl(SoundWindow, 7)
	ActionButton = GemRB.GetControl(SoundWindow, 21)
	ActionButtonB1 = GemRB.GetControl(SoundWindow, 8)
	ActionButtonB2 = GemRB.GetControl(SoundWindow, 9)
	ActionButtonB3 = GemRB.GetControl(SoundWindow, 10)
	SelectionButton = GemRB.GetControl(SoundWindow, 57)
	SelectionButtonB1 = GemRB.GetControl(SoundWindow, 58)
	SelectionButtonB2 = GemRB.GetControl(SoundWindow, 59)
	SelectionButtonB3 = GemRB.GetControl(SoundWindow, 60)
	OkButton = GemRB.GetControl(SoundWindow, 24)
	CancelButton = GemRB.GetControl(SoundWindow, 25)
	GemRB.SetText(SoundWindow, TextAreaControl, 18041)
	GemRB.SetText(SoundWindow, OkButton, 11973)
	GemRB.SetText(SoundWindow, CancelButton, 13727)
	GemRB.SetEvent(SoundWindow, SubtitleButton, IE_GUI_BUTTON_ON_PRESS, "SubtitlePress")
	GemRB.SetEvent(SoundWindow, SubtitleButtonB, IE_GUI_BUTTON_ON_PRESS, "SubtitlePress")
	GemRB.SetButtonFlags(SoundWindow, SubtitleButtonB, IE_GUI_BUTTON_CHECKBOX,OP_OR)
	GemRB.SetVarAssoc(SoundWindow, SubtitleButtonB, "Subtitles",1)

	GemRB.SetEvent(SoundWindow, WarCryButton, IE_GUI_BUTTON_ON_PRESS, "WarCryPress")
	GemRB.SetEvent(SoundWindow, WarCryButtonB, IE_GUI_BUTTON_ON_PRESS, "WarCryPress")
	GemRB.SetButtonFlags(SoundWindow, WarCryButtonB, IE_GUI_BUTTON_CHECKBOX,OP_OR)
	GemRB.SetVarAssoc(SoundWindow, WarCryButtonB, "Attack Sounds",1)

	GemRB.SetEvent(SoundWindow, StepsButton, IE_GUI_BUTTON_ON_PRESS, "StepsPress")
	GemRB.SetEvent(SoundWindow, StepsButtonB, IE_GUI_BUTTON_ON_PRESS, "StepsPress")
	GemRB.SetButtonFlags(SoundWindow, StepsButtonB, IE_GUI_BUTTON_CHECKBOX,OP_OR)
	GemRB.SetVarAssoc(SoundWindow, StepsButtonB, "Footsteps",1)

	GemRB.SetEvent(SoundWindow, ActionButton, IE_GUI_BUTTON_ON_PRESS, "ActionPress")
	GemRB.SetEvent(SoundWindow, ActionButtonB1, IE_GUI_BUTTON_ON_PRESS, "ActionPress")
	GemRB.SetButtonFlags(SoundWindow, ActionButtonB1, IE_GUI_BUTTON_RADIOBUTTON,OP_OR)
	GemRB.SetVarAssoc(SoundWindow, ActionButtonB1, "Command Sounds Frequency",1)

	GemRB.SetEvent(SoundWindow, ActionButtonB2, IE_GUI_BUTTON_ON_PRESS, "ActionPress")
	GemRB.SetButtonFlags(SoundWindow, ActionButtonB2, IE_GUI_BUTTON_RADIOBUTTON,OP_OR)
	GemRB.SetVarAssoc(SoundWindow, ActionButtonB2, "Command Sounds Frequency",2)

	GemRB.SetEvent(SoundWindow, ActionButtonB3, IE_GUI_BUTTON_ON_PRESS, "ActionPress")
	GemRB.SetButtonFlags(SoundWindow, ActionButtonB3, IE_GUI_BUTTON_RADIOBUTTON,OP_OR)
	GemRB.SetVarAssoc(SoundWindow, ActionButtonB3, "Command Sounds Frequency",3)

	GemRB.SetEvent(SoundWindow, SelectionButton, IE_GUI_BUTTON_ON_PRESS, "SelectionPress")
	GemRB.SetEvent(SoundWindow, SelectionButtonB1, IE_GUI_BUTTON_ON_PRESS, "SelectionPress")
	GemRB.SetButtonFlags(SoundWindow, SelectionButtonB1, IE_GUI_BUTTON_RADIOBUTTON,OP_OR)
	GemRB.SetVarAssoc(SoundWindow, SelectionButtonB1, "Selection Sounds Frequency",1)

	GemRB.SetEvent(SoundWindow, SelectionButtonB2, IE_GUI_BUTTON_ON_PRESS, "SelectionPress")
	GemRB.SetButtonFlags(SoundWindow, SelectionButtonB2, IE_GUI_BUTTON_RADIOBUTTON,OP_OR)
	GemRB.SetVarAssoc(SoundWindow, SelectionButtonB2, "Selection Sounds Frequency",2)

	GemRB.SetEvent(SoundWindow, SelectionButtonB3, IE_GUI_BUTTON_ON_PRESS, "SelectionPress")
	GemRB.SetButtonFlags(SoundWindow, SelectionButtonB3, IE_GUI_BUTTON_RADIOBUTTON,OP_OR)
	GemRB.SetVarAssoc(SoundWindow, SelectionButtonB3, "Selection Sounds Frequency",3)

	GemRB.SetEvent(SoundWindow, OkButton, IE_GUI_BUTTON_ON_PRESS, "OkPress")
	GemRB.SetEvent(SoundWindow, CancelButton, IE_GUI_BUTTON_ON_PRESS, "CancelPress")
	GemRB.SetVisible(SoundWindow,1)
	return
	
def SubtitlePress():
	GemRB.SetText(SoundWindow, TextAreaControl, 18015)
	return
	
def WarCryPress():
	GemRB.SetText(SoundWindow, TextAreaControl, 18013)
	return
	
def StepsPress():
	GemRB.SetText(SoundWindow, TextAreaControl, 18014)
	return
	
def ActionPress():
	GemRB.SetText(SoundWindow, TextAreaControl, 18016)
	return
	
def SelectionPress():
	GemRB.SetText(SoundWindow, TextAreaControl, 11352)
	return
	
def OkPress():
	GemRB.UnloadWindow(SoundWindow)
	GemRB.SetNextScript("GUIOPT7")
	return
	
def CancelPress():
	GemRB.UnloadWindow(SoundWindow)
	GemRB.SetNextScript("GUIOPT7")
	return
