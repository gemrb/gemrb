#character generation (GUICG 0)
import GemRB
from ie_stats import *

CharGenWindow = 0
TextAreaControl = 0
PortraitName = ""

def OnLoad():
	global CharGenWindow, TextAreaControl, PortraitName

	GemRB.LoadWindowPack("GUICG")
	CharGenWindow = GemRB.LoadWindow(0)
	PortraitButton = GemRB.GetControl(CharGenWindow, 12)
	GemRB.SetButtonFlags(CharGenWindow, PortraitButton, IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_NO_IMAGE,OP_SET)
	PortraitTable = GemRB.LoadTable("pictures")
	PortraitName = GemRB.GetTableRowName(PortraitTable,GemRB.GetVar("PortraitIndex") )
	GemRB.SetButtonPicture(CharGenWindow,PortraitButton, PortraitName+"L")
	GemRB.UnloadTable(PortraitTable)

	RaceTable = GemRB.LoadTable("races")
	ClassTable = GemRB.LoadTable("classes")
	KitTable = GemRB.LoadTable("kitlist")
	AlignmentTable = GemRB.LoadTable("aligns")
	AbilityTable = GemRB.LoadTable("ability")

	GenderButton = GemRB.GetControl(CharGenWindow,0)
	GemRB.SetText(CharGenWindow,GenderButton,11956)
	GemRB.SetButtonState(CharGenWindow,GenderButton,IE_GUI_BUTTON_DISABLED)

	RaceButton = GemRB.GetControl(CharGenWindow,1)
	GemRB.SetText(CharGenWindow,RaceButton, 11957)
	GemRB.SetButtonState(CharGenWindow,RaceButton,IE_GUI_BUTTON_DISABLED)

	ClassButton = GemRB.GetControl(CharGenWindow,2)
	GemRB.SetText(CharGenWindow,ClassButton, 11959)
	GemRB.SetButtonState(CharGenWindow,ClassButton,IE_GUI_BUTTON_DISABLED)

	AlignmentButton = GemRB.GetControl(CharGenWindow,3)
	GemRB.SetText(CharGenWindow,AlignmentButton, 11958)
	GemRB.SetButtonState(CharGenWindow,AlignmentButton,IE_GUI_BUTTON_DISABLED)

	AbilitiesButton = GemRB.GetControl(CharGenWindow,4)
	GemRB.SetText(CharGenWindow,AbilitiesButton, 11960)
	GemRB.SetButtonState(CharGenWindow,AbilitiesButton,IE_GUI_BUTTON_DISABLED)

	SkillButton = GemRB.GetControl(CharGenWindow,5)
	GemRB.SetText(CharGenWindow,SkillButton, 17372)
	GemRB.SetButtonState(CharGenWindow,SkillButton,IE_GUI_BUTTON_DISABLED)

	AppearanceButton = GemRB.GetControl(CharGenWindow,6)
	GemRB.SetText(CharGenWindow,AppearanceButton, 11961)
	GemRB.SetButtonState(CharGenWindow,AppearanceButton,IE_GUI_BUTTON_DISABLED)

	NameButton = GemRB.GetControl(CharGenWindow,7)
	GemRB.SetText(CharGenWindow,NameButton, 11963)
	GemRB.SetButtonState(CharGenWindow,NameButton,IE_GUI_BUTTON_DISABLED)

	BackButton = GemRB.GetControl(CharGenWindow, 11)
	GemRB.SetButtonState(CharGenWindow,BackButton,IE_GUI_BUTTON_ENABLED)

	AcceptButton = GemRB.GetControl(CharGenWindow, 8)
	GemRB.SetText(CharGenWindow, AcceptButton, 11962)
	GemRB.SetButtonState(CharGenWindow,AcceptButton,IE_GUI_BUTTON_ENABLED)
	GemRB.SetButtonFlags(CharGenWindow,AcceptButton, IE_GUI_BUTTON_DEFAULT,OP_OR)

	ImportButton = GemRB.GetControl(CharGenWindow, 13)
	GemRB.SetText(CharGenWindow, ImportButton, 13955)
	GemRB.SetButtonState(CharGenWindow,ImportButton,IE_GUI_BUTTON_DISABLED)

	CancelButton = GemRB.GetControl(CharGenWindow, 15)
	GemRB.SetText(CharGenWindow, CancelButton, 8159)
	GemRB.SetButtonState(CharGenWindow,CancelButton,IE_GUI_BUTTON_ENABLED)

	TextAreaControl= GemRB.GetControl(CharGenWindow,9)
	
	GemRB.SetText(CharGenWindow, TextAreaControl, 1047)
	GemRB.TextAreaAppend(CharGenWindow, TextAreaControl, ": ")
	GemRB.TextAreaAppend(CharGenWindow, TextAreaControl, GemRB.GetToken("CHARNAME") )

	GemRB.TextAreaAppend(CharGenWindow, TextAreaControl, 12135, -1)
	GemRB.TextAreaAppend(CharGenWindow, TextAreaControl,": ")
	if GemRB.GetVar("Gender") == 1:
		GemRB.TextAreaAppend(CharGenWindow, TextAreaControl, 1050)
	else:
		GemRB.TextAreaAppend(CharGenWindow, TextAreaControl, 1051)
	GemRB.TextAreaAppend(CharGenWindow, TextAreaControl,1048,-1) # new line
	GemRB.TextAreaAppend(CharGenWindow, TextAreaControl,": ")
	GemRB.TextAreaAppend(CharGenWindow, TextAreaControl,GemRB.GetTableValue(RaceTable,GemRB.GetVar("Race")-1,2))
	GemRB.TextAreaAppend(CharGenWindow, TextAreaControl,12136, -1)
	GemRB.TextAreaAppend(CharGenWindow, TextAreaControl,": ")
	KitIndex = GemRB.GetVar("Class Kit")
	if KitIndex == 0:
		GemRB.TextAreaAppend(CharGenWindow, TextAreaControl,GemRB.GetTableValue(ClassTable,GemRB.GetVar("Class")-1,2))
	else:
		GemRB.TextAreaAppend(CharGenWindow, TextAreaControl, GemRB.GetTableValue(KitTable, KitIndex,2) )
	GemRB.TextAreaAppend(CharGenWindow, TextAreaControl,1049, -1)
	GemRB.TextAreaAppend(CharGenWindow, TextAreaControl,": ")
	v = GemRB.FindTableValue(AlignmentTable,3,GemRB.GetVar("Alignment"))
	GemRB.TextAreaAppend(CharGenWindow, TextAreaControl,GemRB.GetTableValue(AlignmentTable,v,2))
	for i in range(0,6):
		v = GemRB.GetTableValue(AbilityTable, i,2)
		GemRB.TextAreaAppend(CharGenWindow, TextAreaControl, v, -1)
		GemRB.TextAreaAppend(CharGenWindow, TextAreaControl,": "+str(GemRB.GetVar("Ability "+str(i))))

	GemRB.SetEvent(CharGenWindow, CancelButton, IE_GUI_BUTTON_ON_PRESS, "CancelPress")
	GemRB.SetEvent(CharGenWindow, BackButton, IE_GUI_BUTTON_ON_PRESS, "BackPress")
	GemRB.SetEvent(CharGenWindow, AcceptButton, IE_GUI_BUTTON_ON_PRESS, "NextPress")
	GemRB.SetVisible(CharGenWindow,1)
	return
	
def NextPress():
	GemRB.UnloadWindow(CharGenWindow)
	#set my character up
	MyChar = GemRB.GetVar("Slot")
	GemRB.CreatePlayer("charbase", MyChar ) 
	GemRB.SetPlayerStat(MyChar, IE_SEX, GemRB.GetVar("Gender") )
	GemRB.SetPlayerStat(MyChar, IE_RACE, GemRB.GetVar("Race") )
	Class=GemRB.GetVar("Class")
	GemRB.SetPlayerStat(MyChar, IE_CLASS, Class)
	t=GemRB.GetVar("Alignment")
	GemRB.SetPlayerStat(MyChar, IE_ALIGNMENT, t)
	TmpTable=GemRB.LoadTable("repstart")
	t=GemRB.GetTableValue(TmpTable,t,0)
	GemRB.SetPlayerStat(MyChar, IE_REPUTATION, t)
	GemRB.UnloadTable(TmpTable)

	TmpTable=GemRB.LoadTable("strtgold")
	t=GemRB.Roll(GemRB.GetTableValue(TmpTable,Class,1),GemRB.GetTableValue(TmpTable,Class,0), GemRB.GetTableValue(TmpTable,Class,2) )
	GemRB.SetPlayerStat(MyChar, IE_GOLD, t*GemRB.GetTableValue(TmpTable,Class,3) )
	GemRB.UnloadTable(TmpTable)

	GemRB.SetPlayerStat(MyChar, IE_HATEDRACE, GemRB.GetVar("HatedRace") )
	TmpTable=GemRB.LoadTable("ability")
	AbilityCount = GemRB.GetTableRowCount(TmpTable)
	for i in range(0,AbilityCount):
		StatID=GemRB.GetTableValue(TmpTable, i,4)
		GemRB.SetPlayerStat(MyChar, StatID, GemRB.GetVar("Ability "+str(i) ) )
	GemRB.UnloadTable(TmpTable)

	TmpTable=GemRB.LoadTable("weapprof")
	ProfCount = GemRB.GetTableRowCount(TmpTable)
	for i in range(7,ProfCount):
		StatID=GemRB.GetTableValue(TmpTable, i, 0)
		GemRB.SetPlayerStat(MyChar, StatID, GemRB.GetVar("Prof "+str(i) ) )
	GemRB.UnloadTable(TmpTable)

	GemRB.SetPlayerStat(MyChar, IE_HAIR_COLOR, GemRB.GetVar("Color1") )
	GemRB.SetPlayerStat(MyChar, IE_SKIN_COLOR, GemRB.GetVar("Color2") )
	GemRB.SetPlayerStat(MyChar, IE_MAJOR_COLOR, GemRB.GetVar("Color4") )
	GemRB.SetPlayerStat(MyChar, IE_MINOR_COLOR, GemRB.GetVar("Color3") )
	GemRB.SetPlayerStat(MyChar, IE_METAL_COLOR, 0x1B )
	GemRB.SetPlayerStat(MyChar, IE_LEATHER_COLOR, 0x16 )
	GemRB.SetPlayerStat(MyChar, IE_ARMOR_COLOR, 0x17 )
	GemRB.SetPlayerStat(MyChar, IE_EA, 2 )
	Str=GemRB.GetVar("Ability 1")
	GemRB.SetPlayerStat(MyChar, IE_STR, Str)
	if Str==18:
		GemRB.SetPlayerStat(MyChar,IE_STREXTRA,GemRB.GetVar("StrExtra"))
	else:
		GemRB.SetPlayerStat(MyChar, IE_STREXTRA,0)

	GemRB.SetPlayerStat(MyChar, IE_INT, GemRB.GetVar("Ability 2"))
	GemRB.SetPlayerStat(MyChar, IE_WIS, GemRB.GetVar("Ability 3"))
	GemRB.SetPlayerStat(MyChar, IE_DEX, GemRB.GetVar("Ability 4"))
	GemRB.SetPlayerStat(MyChar, IE_CON, GemRB.GetVar("Ability 5"))
	GemRB.SetPlayerStat(MyChar, IE_CHR, GemRB.GetVar("Ability 6"))

	GemRB.SetPlayerName(MyChar, GemRB.GetToken("CHARNAME"), 0)
	#does all the rest
	GemRB.FillPlayerInfo(MyChar,PortraitName+"M", PortraitName+"S") 
	#LETS PLAY!!
	playmode = GemRB.GetVar("PlayMode")
	if playmode >=0:
		print "PlayMode: ",playmode
		GemRB.EnterGame()
	else:
		#leaving multi player pregen
		GemRB.UnloadWindow(CharGenWindow)
		GemRB.SetNextScript("GUIMP")
	return

def CancelPress():
	GemRB.UnloadWindow(CharGenWindow)
	GemRB.SetNextScript("CharGen")
	return

def BackPress():
	GemRB.UnloadWindow(CharGenWindow)
	GemRB.SetNextScript("CharGen8") #name
	return

