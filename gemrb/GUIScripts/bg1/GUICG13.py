#character generation, color (GUICG13)
import GemRB

global IE_ANIM_ID
ColorTable = 0
ColorWindow = 0
ColorPicker = 0
DoneButton = 0
ColorIndex = 0
PickedColor = 0
HairButton = 0
SkinButton = 0
MajorButton = 0
MinorButton = 0
HairColor = 0
SkinColor = 0
MinorColor = 0
MajorColor = 0
PDollButton = 0
IE_ANIM_ID = 206

def RefreshPDoll():
	PDollTable = GemRB.LoadTable("pdolls")
	AnimID = 0x6000
	table = GemRB.LoadTable("avprefr")
	AnimID = AnimID+GemRB.GetTableValue(table, GemRB.GetVar("Race"),0)
	table = GemRB.LoadTable("avprefc")
	AnimID = AnimID+GemRB.GetTableValue(table, GemRB.GetVar("Class"),0)
	table = GemRB.LoadTable("avprefg")
	AnimID = AnimID+GemRB.GetTableValue(table, GemRB.GetVar("Gender"),0)
	ResRef = GemRB.GetTableValue(PDollTable,hex(AnimID), "LEVEL1")
	GemRB.SetButtonPLT(ColorWindow, PDollButton, ResRef,
		0, MinorColor, MajorColor, SkinColor, 0, 0, HairColor, 0)

	return

def OnLoad():
	global ColorWindow, DoneButton, PDollButton, ColorTable
	global HairButton, SkinButton, MajorButton, MinorButton
	global HairColor, SkinColor, MinorColor, MajorColor
	
	GemRB.LoadWindowPack("GUICG")
	ColorWindow=GemRB.LoadWindow(13)

	ColorTable = GemRB.LoadTable("clowncol")
	#set these colors to some default
	PortraitTable = GemRB.LoadTable("pictures")
	PortraitName = GemRB.GetToken("LargePortrait")
	PortraitName = PortraitName[0:len(PortraitName)-1]
	PortraitIndex = GemRB.GetTableRowIndex(PortraitTable, PortraitName)
	if PortraitIndex<0:
		HairColor=GemRB.GetTableValue(PortraitTable,0,1)
		SkinColor=GemRB.GetTableValue(PortraitTable,0,2)
		MinorColor=GemRB.GetTableValue(PortraitTable,0,3)
		MajorColor=GemRB.GetTableValue(PortraitTable,0,4)
	else:
		HairColor=GemRB.GetTableValue(PortraitTable,PortraitIndex,1)
		SkinColor=GemRB.GetTableValue(PortraitTable,PortraitIndex,2)
		MinorColor=GemRB.GetTableValue(PortraitTable,PortraitIndex,3)
		MajorColor=GemRB.GetTableValue(PortraitTable,PortraitIndex,4)

	PDollButton = GemRB.GetControl(ColorWindow, 1)
	GemRB.SetButtonFlags(ColorWindow, PDollButton, IE_GUI_BUTTON_PICTURE,OP_OR)

	HairButton = GemRB.GetControl(ColorWindow, 2)
	GemRB.SetButtonFlags(ColorWindow, HairButton, IE_GUI_BUTTON_PICTURE,OP_OR)
	GemRB.SetEvent(ColorWindow, HairButton, IE_GUI_BUTTON_ON_PRESS,"HairPress")
	GemRB.SetButtonBAM(ColorWindow, HairButton, "COLGRAD", 0, 0, HairColor)

	SkinButton = GemRB.GetControl(ColorWindow, 3)
	GemRB.SetButtonFlags(ColorWindow, SkinButton, IE_GUI_BUTTON_PICTURE,OP_OR)
	GemRB.SetEvent(ColorWindow, SkinButton, IE_GUI_BUTTON_ON_PRESS,"SkinPress")
	GemRB.SetButtonBAM(ColorWindow, SkinButton, "COLGRAD", 0, 0, SkinColor)

	MajorButton = GemRB.GetControl(ColorWindow, 5)
	GemRB.SetButtonFlags(ColorWindow, MajorButton, IE_GUI_BUTTON_PICTURE,OP_OR)
	GemRB.SetEvent(ColorWindow, MajorButton, IE_GUI_BUTTON_ON_PRESS,"MajorPress")
	GemRB.SetButtonBAM(ColorWindow, MajorButton, "COLGRAD", 0, 0, MinorColor)

	MinorButton = GemRB.GetControl(ColorWindow, 4)
	GemRB.SetButtonFlags(ColorWindow, MinorButton, IE_GUI_BUTTON_PICTURE,OP_OR)
	GemRB.SetEvent(ColorWindow, MinorButton, IE_GUI_BUTTON_ON_PRESS,"MinorPress")
	GemRB.SetButtonBAM(ColorWindow, MinorButton, "COLGRAD", 0, 0, MajorColor)

	BackButton = GemRB.GetControl(ColorWindow,13)
	GemRB.SetText(ColorWindow,BackButton,15416)
	DoneButton = GemRB.GetControl(ColorWindow,0)
	GemRB.SetText(ColorWindow,DoneButton,11973)
	GemRB.SetButtonFlags(ColorWindow, DoneButton, IE_GUI_BUTTON_DEFAULT,OP_OR)

	GemRB.SetEvent(ColorWindow,DoneButton,IE_GUI_BUTTON_ON_PRESS,"NextPress")
	GemRB.SetEvent(ColorWindow,BackButton,IE_GUI_BUTTON_ON_PRESS,"BackPress")
	RefreshPDoll()
	GemRB.SetVisible(ColorWindow,1)
	return

def DonePress():
	global HairColor, SkinColor, MinorColor, MajorColor

	GemRB.UnloadWindow(ColorPicker)
	GemRB.SetVisible(ColorWindow,1)
	PickedColor=GemRB.GetTableValue(ColorTable, ColorIndex, GemRB.GetVar("Selected"))
	if ColorIndex==0:
		HairColor=PickedColor
		GemRB.SetButtonBAM(ColorWindow, HairButton, "COLGRAD", 0, 0, HairColor)
		RefreshPDoll()
		return
	if ColorIndex==1:
		SkinColor=PickedColor
		GemRB.SetButtonBAM(ColorWindow, SkinButton, "COLGRAD", 0, 0, SkinColor)
		RefreshPDoll()
		return
	if ColorIndex==2:
		MinorColor=PickedColor
		GemRB.SetButtonBAM(ColorWindow, MajorButton, "COLGRAD", 0, 0, MinorColor)
		RefreshPDoll()
		return

	MajorColor=PickedColor
	GemRB.SetButtonBAM(ColorWindow, MinorButton, "COLGRAD", 0, 0, MajorColor)
	RefreshPDoll()
	return

def GetColor():
	global ColorPicker

	ColorPicker=GemRB.LoadWindow(14)
	GemRB.SetVar("Selected",-1)
	for i in range(34):
		Button = GemRB.GetControl(ColorPicker, i)
		GemRB.SetButtonState(ColorPicker, Button, IE_GUI_BUTTON_LOCKED)
		GemRB.SetButtonFlags(ColorPicker, Button, IE_GUI_BUTTON_PICTURE,OP_OR)
		#GemRB.SetButtonFlags(ColorPicker, Button, IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_RADIOBUTTON,OP_OR)

	Selected = -1
	for i in range(34):
		MyColor = GemRB.GetTableValue(ColorTable, ColorIndex, i)
		if MyColor == "*":
			break
		Button = GemRB.GetControl(ColorPicker, i)
		GemRB.SetButtonBAM(ColorPicker, Button, "COLGRAD", 0, 0, MyColor)
		if PickedColor == MyColor:
			GemRB.SetVar("Selected",i)
			Selected = i
		GemRB.SetButtonState(ColorPicker, Button, IE_GUI_BUTTON_ENABLED)
		GemRB.SetVarAssoc(ColorPicker, Button, "Selected",i)
		GemRB.SetEvent(ColorPicker, Button, IE_GUI_BUTTON_ON_PRESS, "DonePress")
	
	GemRB.SetVisible(ColorPicker,1)
	return

def HairPress():
	global ColorIndex, PickedColor

	GemRB.SetVisible(ColorWindow,0)
	ColorIndex = 0
	PickedColor = HairColor
	GetColor()
	return

def SkinPress():
	global ColorIndex, PickedColor

	GemRB.SetVisible(ColorWindow,0)
	ColorIndex = 1
	PickedColor = SkinColor
	GetColor()
	return

def MajorPress():
	global ColorIndex, PickedColor

	GemRB.SetVisible(ColorWindow,0)
	ColorIndex = 2
	PickedColor = MinorColor
	GetColor()
	return

def MinorPress():
	global ColorIndex, PickedColor

	GemRB.SetVisible(ColorWindow,0)
	ColorIndex = 3
	PickedColor = MajorColor
	GetColor()
	return

def BackPress():
	GemRB.UnloadWindow(ColorWindow)
	GemRB.SetNextScript("CharGen7")
	return

def NextPress():
	GemRB.UnloadWindow(ColorWindow)
	GemRB.SetVar("HairColor",HairColor)
	GemRB.SetVar("SkinColor",SkinColor)
	GemRB.SetVar("MinorColor",MinorColor)
	GemRB.SetVar("MajorColor",MajorColor)
	GemRB.SetNextScript("GUICG19") #sounds
	return
