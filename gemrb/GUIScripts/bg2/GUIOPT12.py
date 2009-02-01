#character sounds
import GemRB

SoundWindow = 0
TextAreaControl = 0

def OnLoad():
	global SoundWindow, TextAreaControl
	GemRB.LoadWindowPack("GUIOPT", 640, 480)
	SoundWindow = GemRB.LoadWindowObject(12)
	TextAreaControl = SoundWindow.GetControl(16)
	SubtitleButton = SoundWindow.GetControl(20)
	SubtitleButtonB = SoundWindow.GetControl(5)
	WarCryButton = SoundWindow.GetControl(18)
	WarCryButtonB = SoundWindow.GetControl(6)
	StepsButton = SoundWindow.GetControl(19)
	StepsButtonB = SoundWindow.GetControl(7)
	ActionButton = SoundWindow.GetControl(21)
	ActionButtonB1 = SoundWindow.GetControl(8)
	ActionButtonB2 = SoundWindow.GetControl(9)
	ActionButtonB3 = SoundWindow.GetControl(10)
	SelectionButton = SoundWindow.GetControl(57)
	SelectionButtonB1 = SoundWindow.GetControl(58)
	SelectionButtonB2 = SoundWindow.GetControl(59)
	SelectionButtonB3 = SoundWindow.GetControl(60)
	OkButton = SoundWindow.GetControl(24)
	CancelButton = SoundWindow.GetControl(25)
	TextAreaControl.SetText(18041)
	OkButton.SetText(11973)
	CancelButton.SetText(13727)
	SubtitleButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "SubtitlePress")
	SubtitleButtonB.SetEvent(IE_GUI_BUTTON_ON_PRESS, "SubtitlePress")
	SubtitleButtonB.SetFlags(IE_GUI_BUTTON_CHECKBOX,OP_OR)
	SubtitleButtonB.SetVarAssoc("Subtitles",1)

	WarCryButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "WarCryPress")
	WarCryButtonB.SetEvent(IE_GUI_BUTTON_ON_PRESS, "WarCryPress")
	WarCryButtonB.SetFlags(IE_GUI_BUTTON_CHECKBOX,OP_OR)
	WarCryButtonB.SetVarAssoc("Attack Sounds",1)

	StepsButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "StepsPress")
	StepsButtonB.SetEvent(IE_GUI_BUTTON_ON_PRESS, "StepsPress")
	StepsButtonB.SetFlags(IE_GUI_BUTTON_CHECKBOX,OP_OR)
	StepsButtonB.SetVarAssoc("Footsteps",1)

	ActionButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "ActionPress")
	ActionButtonB1.SetEvent(IE_GUI_BUTTON_ON_PRESS, "ActionPress")
	ActionButtonB1.SetFlags(IE_GUI_BUTTON_RADIOBUTTON,OP_OR)
	ActionButtonB1.SetVarAssoc("Command Sounds Frequency",1)

	ActionButtonB2.SetEvent(IE_GUI_BUTTON_ON_PRESS, "ActionPress")
	ActionButtonB2.SetFlags(IE_GUI_BUTTON_RADIOBUTTON,OP_OR)
	ActionButtonB2.SetVarAssoc("Command Sounds Frequency",2)

	ActionButtonB3.SetEvent(IE_GUI_BUTTON_ON_PRESS, "ActionPress")
	ActionButtonB3.SetFlags(IE_GUI_BUTTON_RADIOBUTTON,OP_OR)
	ActionButtonB3.SetVarAssoc("Command Sounds Frequency",3)

	SelectionButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "SelectionPress")
	SelectionButtonB1.SetEvent(IE_GUI_BUTTON_ON_PRESS, "SelectionPress")
	SelectionButtonB1.SetFlags(IE_GUI_BUTTON_RADIOBUTTON,OP_OR)
	SelectionButtonB1.SetVarAssoc("Selection Sounds Frequency",1)

	SelectionButtonB2.SetEvent(IE_GUI_BUTTON_ON_PRESS, "SelectionPress")
	SelectionButtonB2.SetFlags(IE_GUI_BUTTON_RADIOBUTTON,OP_OR)
	SelectionButtonB2.SetVarAssoc("Selection Sounds Frequency",2)

	SelectionButtonB3.SetEvent(IE_GUI_BUTTON_ON_PRESS, "SelectionPress")
	SelectionButtonB3.SetFlags(IE_GUI_BUTTON_RADIOBUTTON,OP_OR)
	SelectionButtonB3.SetVarAssoc("Selection Sounds Frequency",3)

	OkButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "OkPress")
	CancelButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "CancelPress")
	SoundWindow.SetVisible(1)
	return
	
def SubtitlePress():
	TextAreaControl.SetText(18015)
	return
	
def WarCryPress():
	TextAreaControl.SetText(18013)
	return
	
def StepsPress():
	TextAreaControl.SetText(18014)
	return
	
def ActionPress():
	TextAreaControl.SetText(18016)
	return
	
def SelectionPress():
	TextAreaControl.SetText(11352)
	return
	
def OkPress():
	if SoundWindow:
		SoundWindow.Unload()
	GemRB.SetNextScript("GUIOPT7")
	return
	
def CancelPress():
	if SoundWindow:
		SoundWindow.Unload()
	GemRB.SetNextScript("GUIOPT7")
	return
