#character generation, sound (GUICG19)
import GemRB

SoundWindow = 0
TextAreaControl = 0
DoneButton = 0
TopIndex = 0

def OnLoad():
	global SoundWindow, TextAreaControl, DoneButton, TopIndex
	
	GemRB.LoadWindowPack("GUICG")
	#this hack will redraw the base CG window
	SoundWindow = GemRB.LoadWindow(19)
	GemRB.SetVar("Sound",0)  #scrapping the sound value

	BackButton = GemRB.GetControl(SoundWindow,10)
	GemRB.SetText(SoundWindow,BackButton,15416)
	DoneButton = GemRB.GetControl(SoundWindow,0)
	GemRB.SetText(SoundWindow,DoneButton,36789)
	GemRB.SetButtonFlags(SoundWindow, DoneButton, IE_GUI_BUTTON_DEFAULT,OP_OR)

	TextAreaControl = GemRB.GetControl(SoundWindow, 50)
	GemRB.SetText(SoundWindow,TextAreaControl,17236)

	TextAreaControl = GemRB.GetControl(SoundWindow, 45)
	GemRB.SetTextAreaSelectable(SoundWindow, TextAreaControl,1)
	GemRB.SetVarAssoc(SoundWindow, TextAreaControl, "Sound", 0)
	RowCount=GemRB.GetCharSounds(SoundWindow, TextAreaControl)
	
	DefaultButton = GemRB.GetControl(SoundWindow,47)
	GemRB.SetText(SoundWindow, DefaultButton, 33479)

	GemRB.SetEvent(SoundWindow,DoneButton,IE_GUI_BUTTON_ON_PRESS,"NextPress")
	GemRB.SetEvent(SoundWindow,BackButton,IE_GUI_BUTTON_ON_PRESS,"BackPress")
	GemRB.SetEvent(SoundWindow,DefaultButton,IE_GUI_BUTTON_ON_PRESS,"DefaultPress")
	GemRB.SetVisible(SoundWindow,1)
	return

def DefaultPress():
	GemRB.SetVar("Sound",0)  #scrapping the sound value
	return

def BackPress():
	GemRB.UnloadWindow(SoundWindow)
	GemRB.SetNextScript("Appearance")
	GemRB.SetVar("Sound",0)  #scrapping the sound value
	return

def NextPress():
	GemRB.UnloadWindow(SoundWindow)
	GemRB.SetNextScript("CharGen8") #name
	return
