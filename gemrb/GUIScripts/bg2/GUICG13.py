#character generation, color (GUICG13)
import GemRB

ColorWindow = 0
DoneButton = 0

def OnLoad():
	global ColorWindow, DoneButton
	global Color1, Color2, Color3, Color4
	
	GemRB.LoadWindowPack("GUICG")
	ColorWindow=GemRB.LoadWindow(13)

	#set these colors to some default
	Color1=0
	Color2=1
	Color3=2
	Color4=3
	HairButton = GemRB.GetControl(ColorWindow, 2)
	GemRB.SetButtonFlags(ColorWindow, HairButton, IE_GUI_BUTTON_PICTURE,OP_OR)
	GemRB.SetButtonBAM(ColorWindow, HairButton, "COLGRAD",Color1)
	SkinButton = GemRB.GetControl(ColorWindow, 3)
	GemRB.SetButtonFlags(ColorWindow, SkinButton, IE_GUI_BUTTON_PICTURE,OP_OR)
	GemRB.SetButtonBAM(ColorWindow, SkinButton, "COLGRAD",Color2)
	MajorButton = GemRB.GetControl(ColorWindow, 4)
	GemRB.SetButtonFlags(ColorWindow, MajorButton, IE_GUI_BUTTON_PICTURE,OP_OR)
	GemRB.SetButtonBAM(ColorWindow, MajorButton, "COLGRAD",Color3)
	MinorButton = GemRB.GetControl(ColorWindow, 5)
	GemRB.SetButtonFlags(ColorWindow, MinorButton, IE_GUI_BUTTON_PICTURE,OP_OR)
	GemRB.SetButtonBAM(ColorWindow, MinorButton, "COLGRAD",Color4)

	BackButton = GemRB.GetControl(ColorWindow,13)
	GemRB.SetText(ColorWindow,BackButton,15416)
	DoneButton = GemRB.GetControl(ColorWindow,0)
	GemRB.SetText(ColorWindow,DoneButton,11973)

	GemRB.SetEvent(ColorWindow,DoneButton,IE_GUI_BUTTON_ON_PRESS,"NextPress")
	GemRB.SetEvent(ColorWindow,BackButton,IE_GUI_BUTTON_ON_PRESS,"BackPress")
	GemRB.SetVisible(ColorWindow,1)
	return

def BackPress():
	GemRB.UnloadWindow(ColorWindow)
	GemRB.SetNextScript("CharGen7")
	return

def NextPress():
        GemRB.UnloadWindow(ColorWindow)
	GemRB.SetNextScript("CharGen8") #name
	return
