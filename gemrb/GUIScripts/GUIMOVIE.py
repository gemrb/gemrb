import GemRB

MovieWindow = 0
TextAreaControl = 0

def OnLoad():
	global MovieWindow, TextAreaControl
	GemRB.LoadWindowPack("GUIMOVIE")
	MovieWindow = GemRB.LoadWindow(0)
	TextAreaControl = GemRB.GetControl(MovieWindow, 0)
	PlayButton = GemRB.GetControl(MovieWindow, 2)
	CreditsButton = GemRB.GetControl(MovieWindow, 3)
	DoneButton = GemRB.GetControl(MovieWindow, 4)
#GemRB.Read2DAToControl(MovieWindow, TextAreaControl, "MOVIES")
	GemRB.SetText(MovieWindow, PlayButton, 17318)
	GemRB.SetText(MovieWindow, CreditsButton, 15591)
	GemRB.SetText(MovieWindow, DoneButton, 11973)
	GemRB.SetEvent(MovieWindow, PlayButton, 0x00000000, "PlayPress")
	GemRB.SetEvent(MovieWindow, CreditsButton, 0x00000000, "CreditsPress")
	GemRB.SetEvent(MovieWindow, DoneButton, 0x00000000, "DonePress")
	GemRB.ShowModal(MovieWindow)
	return
	
def PlayPress():
	return

def CreditsPress():
	return

def DonePress():
	GemRB.UnloadWindow(MovieWindow)
	GemRB.SetNextScript("Start")
	return
