#character generation, sounds (GUICG19)
import GemRB

VoiceList = 0
CharSoundWindow = 0

# the available sounds
SoundSequence = [ 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', \
				'm', 's', 't', 'u', 'v', '_', 'x', 'y', 'z', '0', '1', '2', \
				'3', '4', '5', '6', '7', '8', '9']
SoundIndex = 0

def OnLoad():
	global CharSoundWindow, VoiceList
	
	GemRB.LoadWindowPack("GUICG", 640, 480)
	CharSoundWindow=GemRB.LoadWindowObject(19)

	VoiceList = CharSoundWindow.GetControl (45)
	VoiceList.SetFlags (IE_GUI_TEXTAREA_SELECTABLE)
	if GemRB.GetVar ("Gender")==1:
		GemRB.SetVar ("Selected", 4)
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

	GemRB.SetEvent(CharSoundWindow, VoiceList, \
					IE_GUI_TEXTAREA_ON_CHANGE, "ChangeVoice")
	DoneButton.SetEvent(IE_GUI_BUTTON_ON_PRESS,"NextPress")
	BackButton.SetEvent(IE_GUI_BUTTON_ON_PRESS,"BackPress")
	CharSoundWindow.SetVisible(1)
	return

def PlayPress():
	global CharSoundWindow, SoundIndex, SoundSequence

	CharSound = VoiceList.QueryText()
	# SClassID.h -> IE_WAV_CLASS_ID = 0x00000004
	while (not GemRB.HasResource (CharSound + SoundSequence[SoundIndex], 0x00000004)):
		NextSound()
	# play the sound like it was a speech, so any previous yells are quieted
	GemRB.PlaySound (CharSound + SoundSequence[SoundIndex], 0, 0, 4)
	NextSound()
	return

def NextSound():
	global SoundIndex, SoundSequence
	SoundIndex += 1
	if SoundIndex >= len(SoundSequence):
		SoundIndex = 0

# When a new voice is selected, play the sounds again from the beginning of the sequence
def ChangeVoice():
	global SoundIndex
	SoundIndex = 0

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
