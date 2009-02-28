#character generation, sound (GUICG19)
import GemRB

SoundWindow = 0
TextAreaControl = 0
DoneButton = 0
TopIndex = 0

def OnLoad():
	global SoundWindow, TextAreaControl, DoneButton, TopIndex

	GemRB.LoadWindowPack("GUICG", 800,  600)
	#this hack will redraw the base CG window
	SoundWindow = GemRB.LoadWindowObject(19)
	GemRB.SetVar("Sound",0)  #scrapping the sound value

	BackButton = SoundWindow.GetControl(10)
	BackButton.SetText(15416)
	BackButton.SetFlags(IE_GUI_BUTTON_CANCEL,OP_OR)

	DoneButton = SoundWindow.GetControl(0)
	DoneButton.SetText(36789)
	DoneButton.SetFlags(IE_GUI_BUTTON_DEFAULT,OP_OR)

	TextAreaControl = SoundWindow.GetControl(50)
	TextAreaControl.SetText(17236)

	TextAreaControl = SoundWindow.GetControl(45)
	TextAreaControl.SetFlags(IE_GUI_TEXTAREA_SELECTABLE)
	TextAreaControl.SetVarAssoc("Sound", 0)
	RowCount=TextAreaControl.GetCharSounds()

	DefaultButton = SoundWindow.GetControl(47)
	DefaultButton.SetText(33479)

	DoneButton.SetEvent(IE_GUI_BUTTON_ON_PRESS,"NextPress")
	BackButton.SetEvent(IE_GUI_BUTTON_ON_PRESS,"BackPress")
	DefaultButton.SetEvent(IE_GUI_BUTTON_ON_PRESS,"DefaultPress")
	SoundWindow.SetVisible(1)
	return

def DefaultPress():
	GemRB.SetVar("Sound",0)  #scrapping the sound value
	TextAreaControl.SetVarAssoc("Sound", 0)
	return

def BackPress():
	if SoundWindow:
		SoundWindow.Unload()
	GemRB.SetNextScript("Appearance")
	GemRB.SetVar("Sound",0)  #scrapping the sound value
	return

def NextPress():
	GemRB.SetToken("VoiceSet", TextAreaControl.QueryText())
	if SoundWindow:
		SoundWindow.Unload()
	GemRB.SetNextScript("CharGen8") #name
	return
