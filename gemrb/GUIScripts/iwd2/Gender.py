#character generation, gender (GUICG1)
import GemRB

GenderWindow = 0
TextAreaControl = 0
DoneButton = 0

def OnLoad():
	global GenderWindow, TextAreaControl, DoneButton
	
	GemRB.LoadWindowPack("GUICG")
	#this hack will redraw the base CG window
	GenderWindow = GemRB.LoadWindow(0)
	PortraitButton = GemRB.GetControl(GenderWindow, 12)
	GemRB.SetButtonFlags(GenderWindow, PortraitButton, IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_NO_IMAGE,OP_SET)
	ImportButton = GemRB.GetControl(GenderWindow, 13)
	GemRB.SetText(GenderWindow, ImportButton, 13955)
	GemRB.SetButtonState(GenderWindow,ImportButton,IE_GUI_BUTTON_DISABLED)

	CancelButton = GemRB.GetControl(GenderWindow, 15)
	GemRB.SetText(GenderWindow, CancelButton, 13727)
	GemRB.SetButtonState(GenderWindow,CancelButton,IE_GUI_BUTTON_DISABLED)

	BiographyButton = GemRB.GetControl(GenderWindow, 16)
	GemRB.SetText(GenderWindow, BiographyButton, 18003)
	GemRB.SetButtonState(GenderWindow,BiographyButton,IE_GUI_BUTTON_DISABLED)

	GemRB.SetVisible(GenderWindow,1)
	GemRB.DrawWindows()
	GemRB.UnloadWindow(GenderWindow)
	GenderWindow = GemRB.LoadWindow(1)

	BackButton = GemRB.GetControl(GenderWindow,6)
	GemRB.SetText(GenderWindow,BackButton,15416)
	DoneButton = GemRB.GetControl(GenderWindow,0)
	GemRB.SetText(GenderWindow,DoneButton,11973)

	TextAreaControl = GemRB.GetControl(GenderWindow, 5)
	GemRB.SetText(GenderWindow,TextAreaControl,17236)

	MaleButton = GemRB.GetControl(GenderWindow,2)
	GemRB.SetButtonFlags(GenderWindow,MaleButton,IE_GUI_BUTTON_RADIOBUTTON,OP_OR)
	GemRB.SetText(GenderWindow,MaleButton,1050)

	FemaleButton = GemRB.GetControl(GenderWindow,3)
	GemRB.SetButtonFlags(GenderWindow,FemaleButton,IE_GUI_BUTTON_RADIOBUTTON,OP_OR)
	GemRB.SetText(GenderWindow,FemaleButton,1051)

	GemRB.SetVarAssoc(GenderWindow,MaleButton,"Gender",1)
	GemRB.SetVarAssoc(GenderWindow,FemaleButton,"Gender",2)
	GemRB.SetEvent(GenderWindow,MaleButton,IE_GUI_BUTTON_ON_PRESS,"ClickedMale")
	GemRB.SetEvent(GenderWindow,FemaleButton,IE_GUI_BUTTON_ON_PRESS,"ClickedFemale")
	GemRB.SetEvent(GenderWindow,DoneButton,IE_GUI_BUTTON_ON_PRESS,"NextPress")
	GemRB.SetEvent(GenderWindow,BackButton,IE_GUI_BUTTON_ON_PRESS,"BackPress")
	GemRB.SetButtonState(GenderWindow,DoneButton,IE_GUI_BUTTON_DISABLED)
	GemRB.SetVisible(GenderWindow,1)
	return

def ClickedMale():
	GemRB.SetText(GenderWindow,TextAreaControl,13083)
	GemRB.SetButtonState(GenderWindow,DoneButton,IE_GUI_BUTTON_ENABLED)
	return

def ClickedFemale():
	GemRB.SetText(GenderWindow,TextAreaControl,13084)
	GemRB.SetButtonState(GenderWindow,DoneButton,IE_GUI_BUTTON_ENABLED)
	return

def BackPress():
	GemRB.UnloadWindow(GenderWindow)
	GemRB.SetNextScript("CharGen")
	GemRB.SetVar("Gender",0)  #scrapping the gender value
	return

def NextPress():
	GemRB.UnloadWindow(GenderWindow)
	GemRB.SetNextScript("Portrait") #appearance
	return
