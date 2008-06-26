#character generation, sounds (GUICG19)
import GemRB

VoiceList = 0
CharSoundWindow = 0

# the available sounds
SoundSequence = [ 'a', 'n', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', \
				'm', 's', 't', 'u', 'v', '_', 'x', 'y', 'z', '0', '1', '2', \
				'3', '4', '5', '6', '7', '8', '9']
SoundIndex = 0

def OnLoad():
	global CharSoundWindow, VoiceList
	
	GemRB.LoadWindowPack("GUICG", 640, 480)
	CharSoundWindow=GemRB.LoadWindow(19)

	VoiceList = GemRB.GetControl (CharSoundWindow, 45)
	GemRB.SetTextAreaFlags (CharSoundWindow, VoiceList, IE_GUI_TEXTAREA_SELECTABLE)
	if GemRB.GetVar ("Gender")==1:
		GemRB.SetVar ("Selected", 4)
	else:
		GemRB.SetVar ("Selected", 0)

	GemRB.SetVarAssoc (CharSoundWindow, VoiceList, "Selected", 0)
	RowCount=GemRB.GetCharSounds(CharSoundWindow, VoiceList)

	PlayButton = GemRB.GetControl (CharSoundWindow, 47)
	GemRB.SetButtonState (CharSoundWindow, PlayButton, IE_GUI_BUTTON_ENABLED)
	GemRB.SetEvent (CharSoundWindow, PlayButton, IE_GUI_BUTTON_ON_PRESS, "PlayPress")
	GemRB.SetText (CharSoundWindow, PlayButton, 17318)

	TextArea = GemRB.GetControl (CharSoundWindow, 50)
	GemRB.SetText (CharSoundWindow, TextArea, 11315)

	BackButton = GemRB.GetControl(CharSoundWindow,10)
	GemRB.SetText(CharSoundWindow,BackButton,15416)
	DoneButton = GemRB.GetControl(CharSoundWindow,0)
	GemRB.SetText(CharSoundWindow,DoneButton,11973)
	GemRB.SetButtonFlags(CharSoundWindow, DoneButton, IE_GUI_BUTTON_DEFAULT,OP_OR)

	GemRB.SetEvent(CharSoundWindow, VoiceList, \
					IE_GUI_TEXTAREA_ON_CHANGE, "ChangeVoice")
	GemRB.SetEvent(CharSoundWindow,DoneButton,IE_GUI_BUTTON_ON_PRESS,"NextPress")
	GemRB.SetEvent(CharSoundWindow,BackButton,IE_GUI_BUTTON_ON_PRESS,"BackPress")
	GemRB.SetVisible(CharSoundWindow,1)
	return

def PlayPress():
	global CharSoundWindow, SoundIndex, SoundSequence

	CharSound = GemRB.QueryText(CharSoundWindow, VoiceList)
	# SClassID.h -> IE_WAV_CLASS_ID = 0x00000004
	while (not GemRB.HasResource (CharSound + SoundSequence[SoundIndex], 0x00000004)):
		SoundIndex +=1
	GemRB.PlaySound (CharSound + SoundSequence[SoundIndex])
	SoundIndex +=1
	if SoundIndex >= len(SoundSequence):
		SoundIndex = 0
	return

# When a new voice is selected, play the sounds again from the beginning of the sequence
def ChangeVoice():
	global SoundIndex
	SoundIndex = 0

def BackPress():
	global CharSoundWindow

	GemRB.UnloadWindow(CharSoundWindow)
	GemRB.SetNextScript("GUICG13") 
	return

def NextPress():
	global CharSoundWindow

	CharSound = GemRB.QueryText (CharSoundWindow, VoiceList)
	GemRB.SetToken ("CharSound", CharSound)
	GemRB.UnloadWindow(CharSoundWindow)
	GemRB.SetNextScript("CharGen8") #name
	return
