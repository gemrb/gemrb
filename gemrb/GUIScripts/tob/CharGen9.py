#character generation (GUICG 0)
import GemRB
from ie_stats import *
from GUICommon import GetLearnableMageSpells, GetLearnablePriestSpells

CharGenWindow = 0
TextAreaControl = 0
PortraitName = ""

def OnLoad():
	global CharGenWindow, TextAreaControl, PortraitName

	GemRB.LoadWindowPack("GUICG", 640, 480)
	CharGenWindow = GemRB.LoadWindow(0)
	PortraitButton = GemRB.GetControl(CharGenWindow, 12)
	GemRB.SetButtonFlags(CharGenWindow, PortraitButton, IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_NO_IMAGE,OP_SET)
	PortraitTable = GemRB.LoadTable("pictures")
	PortraitName = GemRB.GetTableRowName(PortraitTable,GemRB.GetVar ("PortraitIndex") )
	GemRB.SetButtonPicture(CharGenWindow,PortraitButton, PortraitName+"M")
	GemRB.UnloadTable (PortraitTable)

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
	GemRB.SetText(CharGenWindow, BackButton, 15416)
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

	BiographyButton = GemRB.GetControl(CharGenWindow, 16)
	GemRB.SetText(CharGenWindow, BiographyButton, 18003)
	GemRB.SetButtonState(CharGenWindow,BiographyButton,IE_GUI_BUTTON_DISABLED)
	TextAreaControl= GemRB.GetControl(CharGenWindow,9)
	
	GemRB.SetText(CharGenWindow, TextAreaControl, 1047)
	GemRB.TextAreaAppend(CharGenWindow, TextAreaControl, ": ")
	GemRB.TextAreaAppend(CharGenWindow, TextAreaControl, GemRB.GetToken("CHARNAME") )

	GemRB.TextAreaAppend(CharGenWindow, TextAreaControl, 12135, -1)
	GemRB.TextAreaAppend(CharGenWindow, TextAreaControl,": ")
	if GemRB.GetVar ("Gender") == 1:
		GemRB.TextAreaAppend(CharGenWindow, TextAreaControl, 1050)
	else:
		GemRB.TextAreaAppend(CharGenWindow, TextAreaControl, 1051)
	GemRB.TextAreaAppend(CharGenWindow, TextAreaControl,1048,-1) # new line
	GemRB.TextAreaAppend(CharGenWindow, TextAreaControl,": ")
	GemRB.TextAreaAppend(CharGenWindow, TextAreaControl,GemRB.GetTableValue(RaceTable,GemRB.GetVar ("Race")-1,2))
	GemRB.TextAreaAppend(CharGenWindow, TextAreaControl,12136, -1)
	GemRB.TextAreaAppend(CharGenWindow, TextAreaControl,": ")
	KitIndex = GemRB.GetVar ("Class Kit")
	if KitIndex == 0:
		Class = GemRB.GetVar ("Class")-1
		ClassTitle=GemRB.GetTableValue(ClassTable, Class, 2)
	else:
		ClassTitle=GemRB.GetTableValue(KitTable, KitIndex, 2)
	GemRB.TextAreaAppend(CharGenWindow, TextAreaControl, ClassTitle)
	GemRB.TextAreaAppend(CharGenWindow, TextAreaControl,1049, -1)
	GemRB.TextAreaAppend(CharGenWindow, TextAreaControl,": ")
	v = GemRB.FindTableValue(AlignmentTable,3,GemRB.GetVar ("Alignment"))
	GemRB.TextAreaAppend(CharGenWindow, TextAreaControl,GemRB.GetTableValue(AlignmentTable,v,2))
	for i in range(6):
		v = GemRB.GetTableValue(AbilityTable, i,2)
		GemRB.TextAreaAppend(CharGenWindow, TextAreaControl, v, -1)
		GemRB.TextAreaAppend(CharGenWindow, TextAreaControl,": "+str(GemRB.GetVar ("Ability "+str(i))))

	GemRB.SetEvent(CharGenWindow, CancelButton, IE_GUI_BUTTON_ON_PRESS, "CancelPress")
	GemRB.SetEvent(CharGenWindow, BackButton, IE_GUI_BUTTON_ON_PRESS, "BackPress")
	GemRB.SetEvent(CharGenWindow, AcceptButton, IE_GUI_BUTTON_ON_PRESS, "NextPress")
	GemRB.SetVisible(CharGenWindow,1)
	return
	
def NextPress():

	GemRB.UnloadWindow(CharGenWindow)
	#set my character up
	MyChar = GemRB.GetVar ("Slot")
	GemRB.CreatePlayer("charbase", MyChar ) 
	GemRB.SetPlayerStat (MyChar, IE_SEX, GemRB.GetVar ("Gender") )
	RaceTable = GemRB.LoadTable("races")
	Race = GemRB.GetVar ("Race")-1
	GemRB.SetPlayerStat (MyChar, IE_RACE, GemRB.GetTableValue(RaceTable, Race, 3) )
	ClassTable = GemRB.LoadTable("classes")
	ClassIndex = GemRB.GetVar ("Class")-1
	Class = GemRB.GetTableValue(ClassTable, ClassIndex, 5)
	GemRB.SetPlayerStat (MyChar, IE_CLASS, Class)
	KitIndex = GemRB.GetVar ("Class Kit")
	GemRB.SetPlayerStat (MyChar, IE_KIT, KitIndex)
	t = GemRB.GetVar ("Alignment")
	GemRB.SetPlayerStat (MyChar, IE_ALIGNMENT, t)

	#mage spells
	Learnable = GetLearnableMageSpells( KitIndex, t, 1)
	SpellBook = GemRB.GetVar ("MageSpellBook")
	j=1
	for i in range(len(Learnable) ):
		if SpellBook & j:
			GemRB.LearnSpell(MyChar, Learnable[i], 0)
		j=j<<1

	#priest spells
	TmpTable = GemRB.LoadTable("clskills")
	TableName = GemRB.GetTableValue(TmpTable, Class, 1)
	if TableName != "*":
		ClassFlag = 0 #set this according to class
		Learnable = GetLearnablePriestSpells( ClassFlag, t, 1)
		for i in range(len(Learnable) ):
			GemRB.LearnSpell(MyChar, Learnable[i], 0)

	GemRB.UnloadTable (TmpTable)
	TmpTable=GemRB.LoadTable("repstart")
	AlignmentTable = GemRB.LoadTable("aligns")
	t=GemRB.FindTableValue(AlignmentTable, 3, t)
	t=GemRB.GetTableValue(TmpTable,t,0) * 10
	GemRB.SetPlayerStat (MyChar, IE_REPUTATION, t)

	#slot 1 is the protagonist
	if MyChar == 1:
		GemRB.GameSetReputation( t )

	GemRB.UnloadTable (TmpTable)
	TmpTable=GemRB.LoadTable("strtgold")
	t=GemRB.Roll(GemRB.GetTableValue(TmpTable,Class,1),GemRB.GetTableValue(TmpTable,Class,0), GemRB.GetTableValue(TmpTable,Class,2) )
	GemRB.SetPlayerStat (MyChar, IE_GOLD, t*GemRB.GetTableValue(TmpTable,Class,3) )
	GemRB.UnloadTable (AlignmentTable)
	GemRB.UnloadTable (ClassTable)
	GemRB.UnloadTable (RaceTable)
	GemRB.UnloadTable (TmpTable)

	GemRB.SetPlayerStat (MyChar, IE_HATEDRACE, GemRB.GetVar ("HatedRace") )
	TmpTable=GemRB.LoadTable("ability")
	AbilityCount = GemRB.GetTableRowCount(TmpTable)
	for i in range(AbilityCount):
		StatID=GemRB.GetTableValue(TmpTable, i,4)
		GemRB.SetPlayerStat (MyChar, StatID, GemRB.GetVar ("Ability "+str(i) ) )
	GemRB.UnloadTable (TmpTable)

	TmpTable=GemRB.LoadTable("weapprof")
	ProfCount = GemRB.GetTableRowCount(TmpTable)
	#bg2 weapprof.2da contains the bg1 proficiencies too, skipping those
	for i in range(8,ProfCount):
		StatID=GemRB.GetTableValue(TmpTable, i, 0)
		GemRB.SetPlayerStat (MyChar, StatID, GemRB.GetVar ("Prof "+str(i) ) )
	GemRB.UnloadTable (TmpTable)

	GemRB.SetPlayerStat (MyChar, IE_HAIR_COLOR, GemRB.GetVar ("HairColor") )
	GemRB.SetPlayerStat (MyChar, IE_SKIN_COLOR, GemRB.GetVar ("SkinColor") )
	GemRB.SetPlayerStat (MyChar, IE_MAJOR_COLOR, GemRB.GetVar ("MajorColor") )
	GemRB.SetPlayerStat (MyChar, IE_MINOR_COLOR, GemRB.GetVar ("MinorColor") )
	#GemRB.SetPlayerStat (MyChar, IE_METAL_COLOR, 0x1B )
	#GemRB.SetPlayerStat (MyChar, IE_LEATHER_COLOR, 0x16 )
	#GemRB.SetPlayerStat (MyChar, IE_ARMOR_COLOR, 0x17 )
	GemRB.SetPlayerStat (MyChar, IE_EA, 2 )
	Str=GemRB.GetVar ("Ability 1")
	GemRB.SetPlayerStat (MyChar, IE_STR, Str)
	if Str==18:
		GemRB.SetPlayerStat (MyChar,IE_STREXTRA,GemRB.GetVar ("StrExtra"))
	else:
		GemRB.SetPlayerStat (MyChar, IE_STREXTRA,0)

	GemRB.SetPlayerStat (MyChar, IE_INT, GemRB.GetVar ("Ability 2"))
	GemRB.SetPlayerStat (MyChar, IE_WIS, GemRB.GetVar ("Ability 3"))
	GemRB.SetPlayerStat (MyChar, IE_DEX, GemRB.GetVar ("Ability 4"))
	GemRB.SetPlayerStat (MyChar, IE_CON, GemRB.GetVar ("Ability 5"))
	GemRB.SetPlayerStat (MyChar, IE_CHR, GemRB.GetVar ("Ability 6"))

	#setting skills (thieving/ranger)
	TmpTable = GemRB.LoadTable ("skills")
	RowCount = GemRB.GetTableRowCount (TmpTable)-2

        for i in range(RowCount):
		stat = GemRB.GetTableValue (TmpTable, i+2, 2)
		value = GemRB.GetVar ("Skill "+str(i) )
		GemRB.SetPlayerStat (MyChar, stat, value )
 	GemRB.UnloadTable (TmpTable)
	
	GemRB.SetPlayerName (MyChar, GemRB.GetToken("CHARNAME"), 0)
	TmpTable = GemRB.LoadTable ("clskills")
	GemRB.SetPlayerStat (MyChar, IE_XP, GemRB.GetTableValue (TmpTable, Class, 3) )  #this will also set the level (automatically)
	GemRB.UnloadTable (TmpTable)

	#does all the rest
	GemRB.FillPlayerInfo(MyChar,PortraitName+"M", PortraitName+"S") 
	#LETS PLAY!!
	playmode = GemRB.GetVar ("PlayMode")
	if playmode >=0:
		GemRB.EnterGame()
	else:
		#leaving multi player pregen
		GemRB.UnloadWindow(CharGenWindow)
		GemRB.SetNextScript("Start")
	return

def CancelPress():
	GemRB.UnloadWindow(CharGenWindow)
	GemRB.SetNextScript("CharGen")
	return

def BackPress():
	GemRB.UnloadWindow(CharGenWindow)
	GemRB.SetNextScript("CharGen8") #name
	return

