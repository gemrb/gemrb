#character generation, biography (GUICG23)
import GemRB

BioWindow = 0
EditControl = 0

def OnLoad ():
	global BioWindow, EditControl

	GemRB.LoadWindowPack ("GUICG", 640, 480)
	BioWindow = GemRB.LoadWindow (23)

	EditControl = GemRB.GetControl (BioWindow, 3)
	bio = GemRB.GetToken("BIO")
	if bio:
		GemRB.CreateTextEdit(BioWindow, 3, 29, 73, 567, 195, "NORMAL", bio)
	else:
		GemRB.CreateTextEdit(BioWindow, 3, 29, 73, 567, 195, "NORMAL", GemRB.GetString(15882))

	# done
	OkButton = GemRB.GetControl (BioWindow, 1)
	GemRB.SetText (BioWindow, OkButton, 11973)

	ClearButton = GemRB.GetControl (BioWindow, 4)
	GemRB.SetText (BioWindow, ClearButton, 34881)

	# back
	CancelButton = GemRB.GetControl (BioWindow, 2)
	GemRB.SetText (BioWindow, CancelButton, 12896)

	GemRB.SetEvent (BioWindow, OkButton, IE_GUI_BUTTON_ON_PRESS, "OkPress")
	GemRB.SetEvent (BioWindow, ClearButton, IE_GUI_BUTTON_ON_PRESS, "ClearPress")
	GemRB.SetEvent (BioWindow, CancelButton, IE_GUI_BUTTON_ON_PRESS, "CancelPress")
	GemRB.SetVisible (BioWindow,1)
	return

def OkPress ():
	global BioWindow, EditControl

	BioData = GemRB.QueryText (BioWindow, EditControl)
	GemRB.UnloadWindow (BioWindow)
	GemRB.SetNextScript ("CharGen9")
	GemRB.SetToken ("BIO", BioData)
	return
	
def CancelPress ():
	GemRB.UnloadWindow (BioWindow)
	GemRB.SetNextScript ("CharGen9")
	return

def ClearPress ():
	GemRB.SetText (BioWindow, EditControl, "")
	return
