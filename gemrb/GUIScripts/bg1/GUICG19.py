#character generation, sounds (GUICG19)
import GemRB

VoiceList = 0
CharSoundWindow = 0

def OnLoad():
	global CharSoundWindow, VoiceList
	
	GemRB.LoadWindowPack("GUICG", 640, 480)
	CharSoundWindow=GemRB.LoadWindowObject(19)

	VoiceList = CharSoundWindow.GetControl (45)
	VoiceList.SetFlags (IE_GUI_TEXTAREA_SELECTABLE)
	if GemRB.GetVar ("Gender")==1:
		GemRB.SetVar ("Selected", 3) #first male sound
	else:
		GemRB.SetVar ("Selected", 0)

	VoiceList.SetVarAssoc ("Selected", 0)
	RowCount=VoiceList.GetCharSounds()

	PlayButton = CharSoundWindow.GetControl (47)
	PlayButton.SetState (IE_GUI_BUTTON_ENABLED)
	PlayButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "PlayPress")
	PlayButton.SetText (17318)

	TextArea = CharSoundWindow.GetControl (50)
	TextArea.SetText (11315)

	BackButton = CharSoundWindow.GetControl(10)
	BackButton.SetText(15416)
	DoneButton = CharSoundWindow.GetControl(0)
	DoneButton.SetText(11973)
	DoneButton.SetFlags(IE_GUI_BUTTON_DEFAULT,OP_OR)

	DoneButton.SetEvent(IE_GUI_BUTTON_ON_PRESS,"NextPress")
	BackButton.SetEvent(IE_GUI_BUTTON_ON_PRESS,"BackPress")
	CharSoundWindow.SetVisible(1)
	return

def PlayPress():
	global CharSoundWindow

	CharSound = VoiceList.QueryText()
	GemRB.PlaySound (CharSound+"a")
	return

def BackPress():
	global CharSoundWindow

	if CharSoundWindow:
		CharSoundWindow.Unload()
	GemRB.SetNextScript("GUICG13") 
	return

def NextPress():
	global CharSoundWindow

	CharSound = VoiceList.QueryText ()
	GemRB.SetToken ("CharSound", CharSound)
	if CharSoundWindow:
		CharSoundWindow.Unload()
	GemRB.SetNextScript("CharGen8") #name
	return
