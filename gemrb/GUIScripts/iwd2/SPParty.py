#Single Player Party Select
import GemRB

OptionsWindow = 0

def OnLoad():
	global OptionsWindow
	GemRB.LoadWindowPack("GUISP")
	
	PartySelectWindow = GemRB.LoadWindow(1)
	TextArea = GemRB.GetControl(PartySelectWindow, 6)
	PartyScrollBar = GemRB.GetControl(PartySelectWindow, 8)
	
	NewPartyButton = GemRB.GetControl(PartySelectWindow, 0)
	Party1Button = GemRB.GetControl(PartySelectWindow, 1)
	Party2Button = GemRB.GetControl(PartySelectWindow, 2)
	Party3Button = GemRB.GetControl(PartySelectWindow, 3)
	Party4Button = GemRB.GetControl(PartySelectWindow, 4)
	Party5Button = GemRB.GetControl(PartySelectWindow, 5)
	
	ModifyButton = GemRB.GetControl(PartySelectWindow, 12)
	GemRB.SetText(PartySelectWindow, ModifyButton, 10316)
	CancelButton = GemRB.GetControl(PartySelectWindow, 11)
	GemRB.SetText(PartySelectWindow, CancelButton, 13727)
	DoneButton = GemRB.GetControl(PartySelectWindow, 10)
	GemRB.SetText(PartySelectWindow, DoneButton, 11973)
	
	
	
	GemRB.SetVisible(OptionsWindow, 1)
	
	return
	
def ReturnPress():
	global OptionsWindow
	GemRB.UnloadWindow(OptionsWindow)
	GemRB.SetNextScript("Start")
	return
	
def GraphicsPress():
	global OptionsWindow
	GemRB.UnloadWindow(OptionsWindow)
	GemRB.SetNextScript("Graphics")
	return
	
def SoundPress():
	global OptionsWindow
	GemRB.UnloadWindow(OptionsWindow)
	GemRB.SetNextScript("Sound")
	return
	
def GamePlayPress():
	global OptionsWindow
	GemRB.UnloadWindow(OptionsWindow)
	GemRB.SetNextScript("GamePlay")
	return

def MoviePress():
	global OptionsWindow
	GemRB.UnloadWindow(OptionsWindow)
	GemRB.SetNextScript("Movies")
	return
