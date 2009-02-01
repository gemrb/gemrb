#character generation, gender (GUICG1)
import GemRB

GenderWindow = 0
TextAreaControl = 0
DoneButton = 0

def OnLoad():
	global GenderWindow, TextAreaControl, DoneButton
	
	GemRB.LoadWindowPack("GUICG", 800, 600)
	#this hack will redraw the base CG window
	GenderWindow = GemRB.LoadWindowObject(0)
	GenderWindow.SetFrame( )
	PortraitButton = GenderWindow.GetControl(12)
	PortraitButton.SetFlags(IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_NO_IMAGE,OP_SET)
	ImportButton = GenderWindow.GetControl(13)
	ImportButton.SetText(13955)
	ImportButton.SetState(IE_GUI_BUTTON_DISABLED)

	CancelButton = GenderWindow.GetControl(15)
	CancelButton.SetText(13727)
	CancelButton.SetState(IE_GUI_BUTTON_DISABLED)

	BiographyButton = GenderWindow.GetControl(16)
	BiographyButton.SetText(18003)
	BiographyButton.SetState(IE_GUI_BUTTON_DISABLED)

	GenderWindow.SetVisible(1)
	GemRB.DrawWindows()
	if GenderWindow:
		GenderWindow.Unload()
	GenderWindow = GemRB.LoadWindowObject(1)

	BackButton = GenderWindow.GetControl(6)
	BackButton.SetText(15416)
	DoneButton = GenderWindow.GetControl(0)
	DoneButton.SetText(36789)
	DoneButton.SetFlags(IE_GUI_BUTTON_DEFAULT,OP_OR)

	TextAreaControl = GenderWindow.GetControl(5)
	TextAreaControl.SetText(17236)

	MaleButton = GenderWindow.GetControl(7)
	MaleButton.SetEvent(IE_GUI_BUTTON_ON_PRESS,"ClickedMale")
	MaleButton.SetFlags(IE_GUI_BUTTON_RADIOBUTTON,OP_OR)
	FemaleButton = GenderWindow.GetControl(8)
	FemaleButton.SetEvent(IE_GUI_BUTTON_ON_PRESS,"ClickedFemale")
	FemaleButton.SetFlags(IE_GUI_BUTTON_RADIOBUTTON,OP_OR)
	MaleButton.SetVarAssoc("Gender",1)
	FemaleButton.SetVarAssoc("Gender",2)
	
	
	MaleButton = GenderWindow.GetControl(2)
	MaleButton.SetFlags(IE_GUI_BUTTON_RADIOBUTTON,OP_OR)
	MaleButton.SetText(1050)

	FemaleButton = GenderWindow.GetControl(3)
	FemaleButton.SetFlags(IE_GUI_BUTTON_RADIOBUTTON,OP_OR)
	FemaleButton.SetText(1051)

	MaleButton.SetVarAssoc("Gender",1)
	FemaleButton.SetVarAssoc("Gender",2)
	MaleButton.SetEvent(IE_GUI_BUTTON_ON_PRESS,"ClickedMale")
	FemaleButton.SetEvent(IE_GUI_BUTTON_ON_PRESS,"ClickedFemale")
	DoneButton.SetEvent(IE_GUI_BUTTON_ON_PRESS,"NextPress")
	BackButton.SetEvent(IE_GUI_BUTTON_ON_PRESS,"BackPress")
	DoneButton.SetState(IE_GUI_BUTTON_DISABLED)
	GenderWindow.SetVisible(1)
	return

def ClickedMale():
	TextAreaControl.SetText(13083)
	DoneButton.SetState(IE_GUI_BUTTON_ENABLED)
	return

def ClickedFemale():
	TextAreaControl.SetText(13084)
	DoneButton.SetState(IE_GUI_BUTTON_ENABLED)
	return

def BackPress():
	if GenderWindow:
		GenderWindow.Unload()
	GemRB.SetNextScript("CharGen")
	GemRB.SetVar("Gender",0)  #scrapping the gender value
	return

def NextPress():
	if GenderWindow:
		GenderWindow.Unload()
	GemRB.SetNextScript("Portrait") #appearance
	return
