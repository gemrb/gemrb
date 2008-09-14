#character generation, biography (GUICG23)
import GemRB

BioWindow = 0
EditControl = 0

def OnLoad ():
	global BioWindow, EditControl

	GemRB.LoadWindowPack ("GUICG", 640, 480)
	BioWindow = GemRB.LoadWindow (23)

	EditControl = GemRB.GetControl (BioWindow, 3)
	BIO = GemRB.GetToken("BIO")
	EditControl = GemRB.ConvertEdit (BioWindow, EditControl, 5)
	GemRB.SetVarAssoc (BioWindow, EditControl, "row", 0)
	if BIO:
		GemRB.SetText (BioWindow, EditControl, BIO)
	else:
		GemRB.SetText (BioWindow, EditControl, 15882)

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

	row = 0
	line = None
	BioData = ""

	#there is no way to get the entire TextArea content
	#this hack retrieves the TextArea content row by row
	#there is no way to know how much data is in the TextArea
	while 1:
		GemRB.SetVar ("row", row)
		GemRB.SetVarAssoc (BioWindow, EditControl, "row", row)
		line = GemRB.QueryText (BioWindow, EditControl)
		if len(line)<=0:
			break
		BioData += line+"\n"
		row += 1
	
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
