# -*-python-*-
# GemRB - Infinity Engine Emulator
# Copyright (C) 2003-2005 The GemRB Project
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#


#Character Generation

###################################################

import GemRB
from GUIDefines import *
from ie_stats import *
from ie_spells import LS_MEMO
import GUICommon
import Spellbook
import CommonTables
import LUSkillsSelection

CharGenWindow = 0
CharGenState = 0
TextArea = 0
PortraitButton = 0
AcceptButton = 0

GenderButton = 0
GenderWindow = 0
GenderTextArea = 0
GenderDoneButton = 0

Portrait = 0
PortraitsTable = 0
PortraitPortraitButton = 0

RaceButton = 0
RaceWindow = 0
RaceTextArea = 0
RaceDoneButton = 0

ClassButton = 0
ClassWindow = 0
ClassTextArea = 0
ClassDoneButton = 0

ClassMultiWindow = 0
ClassMultiTextArea = 0
ClassMultiDoneButton = 0

KitTable = 0
KitWindow = 0
KitTextArea = 0
KitDoneButton = 0

AlignmentButton = 0
AlignmentWindow = 0
AlignmentTable = 0
AlignmentTextArea = 0
AlignmentDoneButton = 0

AbilitiesButton = 0
AbilitiesWindow = 0
AbilitiesTable = 0
AbilitiesRaceAddTable = 0
AbilitiesRaceReqTable = 0
AbilitiesClassReqTable = 0
AbilitiesMinimum = 0
AbilitiesMaximum = 0
AbilitiesModifier = 0
AbilitiesTextArea = 0
AbilitiesRecallButton = 0
AbilitiesDoneButton = 0

SkillsButton = 0
SkillsWindow = 0
SkillsTable = 0
SkillsTextArea = 0
SkillsDoneButton = 0
SkillsPointsLeft = 0
SkillsState = 0

RacialEnemyButton = 0
RacialEnemyWindow = 0
RacialEnemyTable = 0
RacialEnemyTextArea = 0
RacialEnemyDoneButton = 0

ProficienciesWindow = 0
ProficienciesTable = 0
ProfsMaxTable = 0
ProficienciesTextArea = 0
ProficienciesDoneButton = 0
ProficienciesPointsLeft = 0

MageSpellsWindow = 0
MageSpellsTextArea = 0
MageSpellsDoneButton = 0
MageSpellsSelectPointsLeft = 0

MageMemorizeWindow = 0
MageMemorizeTextArea = 0
MageMemorizeDoneButton = 0
MageMemorizePointsLeft = 0

PriestMemorizeWindow = 0
PriestMemorizeTextArea = 0
PriestMemorizeDoneButton = 0
PriestMemorizePointsLeft = 0

AppearanceButton = 0
AppearanceWindow = 0
AppearanceTable = 0
AppearanceAvatarButton = 0
AppearanceHairButton = 0
AppearanceSkinButton = 0
AppearanceMajorButton = 0
AppearanceMinorButton = 0
HairColor = 0
SkinColor = 0
MajorColor = 0
MinorColor = 0

CharSoundWindow = 0
CharSoundTable = 0
CharSoundStrings = 0

BiographyButton = 0
BiographyWindow = 0
BiographyField = 0

NameButton = 0
NameWindow = 0
NameField = 0
NameDoneButton = 0

SoundIndex = 0
VerbalConstants = None
HasStrExtra = 0
MyChar = 0
ImportedChar = 0

def OnLoad():
	global CharGenWindow, CharGenState, TextArea, PortraitButton, AcceptButton
	global GenderButton, RaceButton, ClassButton, AlignmentButton
	global AbilitiesButton, SkillsButton, AppearanceButton, BiographyButton, NameButton
	global KitTable, ProficienciesTable, AlignmentTable, RacialEnemyTable
	global AbilitiesTable, SkillsTable, PortraitsTable
	global MyChar, ImportedChar

	KitTable = GemRB.LoadTable ("magesch")
	ProficienciesTable = GemRB.LoadTable ("weapprof")
	AlignmentTable = GemRB.LoadTable ("aligns")
	RacialEnemyTable = GemRB.LoadTable ("haterace")
	AbilitiesTable = GemRB.LoadTable ("ability")
	SkillsTable = GemRB.LoadTable ("skills")
	PortraitsTable = GemRB.LoadTable ("pictures")
	GemRB.LoadWindowPack ("GUICG", 640, 480)
	CharGenWindow = GemRB.LoadWindow (0)
	CharGenWindow.SetFrame ()
	CharGenState = 0
	MyChar = GemRB.GetVar ("Slot")
	ImportedChar = 0

	GenderButton = CharGenWindow.GetControl (0)
	GenderButton.SetState (IE_GUI_BUTTON_ENABLED)
	GenderButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)
	GenderButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, GenderPress)
	GenderButton.SetText (11956)

	RaceButton = CharGenWindow.GetControl (1)
	RaceButton.SetState (IE_GUI_BUTTON_DISABLED)
	RaceButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, RacePress)
	RaceButton.SetText (11957)

	ClassButton = CharGenWindow.GetControl (2)
	ClassButton.SetState (IE_GUI_BUTTON_DISABLED)
	ClassButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, ClassPress)
	ClassButton.SetText (11959)

	AlignmentButton = CharGenWindow.GetControl (3)
	AlignmentButton.SetState (IE_GUI_BUTTON_DISABLED)
	AlignmentButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, AlignmentPress)
	AlignmentButton.SetText (11958)

	AbilitiesButton = CharGenWindow.GetControl (4)
	AbilitiesButton.SetState (IE_GUI_BUTTON_DISABLED)
	AbilitiesButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, AbilitiesPress)
	AbilitiesButton.SetText (11960)

	SkillsButton = CharGenWindow.GetControl (5)
	SkillsButton.SetState (IE_GUI_BUTTON_DISABLED)
	SkillsButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, SkillsPress)
	SkillsButton.SetText (11983)

	AppearanceButton = CharGenWindow.GetControl (6)
	AppearanceButton.SetState (IE_GUI_BUTTON_DISABLED)
	AppearanceButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, AppearancePress)
	AppearanceButton.SetText (11961)

	BiographyButton = CharGenWindow.GetControl (16)
	BiographyButton.SetState (IE_GUI_BUTTON_DISABLED)
	BiographyButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, BiographyPress)
	BiographyButton.SetText (18003)

	NameButton = CharGenWindow.GetControl (7)
	NameButton.SetState (IE_GUI_BUTTON_DISABLED)
	NameButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, NamePress)
	NameButton.SetText (11963)

	BackButton = CharGenWindow.GetControl (11)
	BackButton.SetState (IE_GUI_BUTTON_ENABLED)
	BackButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, BackPress)
	BackButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	PortraitButton = CharGenWindow.GetControl (12)
	PortraitButton.SetFlags (IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_NO_IMAGE, OP_SET)
	PortraitButton.SetState (IE_GUI_BUTTON_LOCKED)

	ImportButton = CharGenWindow.GetControl (13)
	ImportButton.SetState (IE_GUI_BUTTON_ENABLED)
	ImportButton.SetText (13955)
	ImportButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, ImportPress)

	CancelButton = CharGenWindow.GetControl (15)
	CancelButton.SetState (IE_GUI_BUTTON_ENABLED)
	CancelButton.SetText (13727)
	CancelButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, CancelPress)

	AcceptButton = CharGenWindow.GetControl (8)
	AcceptButton.SetState (IE_GUI_BUTTON_DISABLED)
	AcceptButton.SetText (11962)
	AcceptButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, AcceptPress)

	TextArea = CharGenWindow.GetControl (9)
	TextArea.SetText (16575)

	CharGenWindow.SetVisible (WINDOW_VISIBLE)
	return

def BackPress():
	global CharGenWindow, CharGenState, SkillsState
	global GenderButton, RaceButton, ClassButton, AlignmentButton, AbilitiesButton, SkillsButton, AppearanceButton, BiographyButton, NameButton

	if CharGenState > 0:
		CharGenState = CharGenState - 1
	else:
		CancelPress()
		return

	if CharGenState > 6:
		CharGenState = 6
		GemRB.SetToken ("CHARNAME","")

	if CharGenState == 0:
		RaceButton.SetState (IE_GUI_BUTTON_DISABLED)
		RaceButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_NAND)
		GenderButton.SetState (IE_GUI_BUTTON_ENABLED)
		GenderButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)
	elif CharGenState == 1:
		ClassButton.SetState (IE_GUI_BUTTON_DISABLED)
		ClassButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_NAND)
		RaceButton.SetState (IE_GUI_BUTTON_ENABLED)
		RaceButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)
	elif CharGenState == 2:
		AlignmentButton.SetState (IE_GUI_BUTTON_DISABLED)
		AlignmentButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_NAND)
		ClassButton.SetState (IE_GUI_BUTTON_ENABLED)
		ClassButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)
	elif CharGenState == 3:
		AbilitiesButton.SetState (IE_GUI_BUTTON_DISABLED)
		AbilitiesButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_NAND)
		AlignmentButton.SetState (IE_GUI_BUTTON_ENABLED)
		AlignmentButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)
	elif CharGenState == 4:
		SkillsButton.SetState (IE_GUI_BUTTON_DISABLED)
		SkillsButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_NAND)
		AbilitiesButton.SetState (IE_GUI_BUTTON_ENABLED)
		AbilitiesButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)
	elif CharGenState == 5:
		AppearanceButton.SetState (IE_GUI_BUTTON_DISABLED)
		SkillsButton.SetState (IE_GUI_BUTTON_ENABLED)
		SkillsButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)
		SkillsState = 0
	elif CharGenState == 6:
		NameButton.SetState (IE_GUI_BUTTON_DISABLED)
		BiographyButton.SetState (IE_GUI_BUTTON_DISABLED)
		AppearanceButton.SetState (IE_GUI_BUTTON_ENABLED)

	AcceptButton.SetState (IE_GUI_BUTTON_DISABLED)
	SetCharacterDescription()
	return

def CancelPress():
	global CharGenWindow

	if CharGenWindow:
		CharGenWindow.Unload ()
	GemRB.CreatePlayer ("", MyChar | 0x8000 )
	GemRB.SetNextScript ("PartyFormation")
	return

def AcceptPress():
	#mage spells
	Kit = GemRB.GetPlayerStat (MyChar, IE_KIT)
	KitIndex = KitTable.FindValue (3, Kit)
	Class = GemRB.GetPlayerStat (MyChar, IE_CLASS)
	t = GemRB.GetPlayerStat (MyChar, IE_ALIGNMENT)
	TableName = CommonTables.ClassSkills.GetValue (Class, 2, 0)
	if TableName != "*":
		#todo: set up ALL spell levels not just level 1
		Spellbook.SetupSpellLevels (MyChar, TableName, IE_SPELL_TYPE_WIZARD, 1)
		Learnable = Spellbook.GetLearnableMageSpells (KitIndex, t, 1)
		SpellBook = GemRB.GetVar ("MageSpellBook")
		MemoBook = GemRB.GetVar ("MageMemorized")
		j=1
		for i in range (len(Learnable) ):
			if SpellBook & j:
				if MemoBook & j:
					memorize = LS_MEMO
				else:
					memorize = 0
				GemRB.LearnSpell (MyChar, Learnable[i], memorize)
			j=j<<1

	#priest spells
	TableName = CommonTables.ClassSkills.GetValue (Class, 1, 0)
	# druids and rangers have a column of their own
	if TableName == "*":
		TableName = CommonTables.ClassSkills.GetValue (Class, 0, 0)
	if TableName != "*":
		if TableName == "MXSPLPRS" or TableName == "MXSPLPAL":
			ClassFlag = 0x8000
		elif TableName == "MXSPLDRU":
			#there is no separate druid table, falling back to priest
			TableName = "MXSPLPRS"
			ClassFlag = 0x4000
		elif TableName == "MXSPLRAN":
			ClassFlag = 0x4000
		else:
			ClassFlag = 0
		#todo: set up ALL spell levels not just level 1
		Spellbook.SetupSpellLevels (MyChar, TableName, IE_SPELL_TYPE_PRIEST, 1)
		Learnable = Spellbook.GetLearnablePriestSpells (ClassFlag, t, 1)
		PriestMemorized = GemRB.GetVar ("PriestMemorized")
		j = 1
		while (PriestMemorized and PriestMemorized != 1<<(j-1)):
			j = j + 1
		for i in range (len(Learnable) ):
			GemRB.LearnSpell (MyChar, Learnable[i], 0)
		GemRB.MemorizeSpell (MyChar, IE_SPELL_TYPE_PRIEST, 0, j, 1)

	# ranger tracking is a hardcoded innate
	if GUICommon.HasHOW():
		if CommonTables.ClassSkills.GetValue (GemRB.GetPlayerStat (MyChar, IE_CLASS), 0) != "*":
			GemRB.LearnSpell (MyChar, "spin139", LS_MEMO)

	# save all the skills
	LUSkillsSelection.SkillsSave (MyChar)

	TmpTable = GemRB.LoadTable ("repstart")
	t = AlignmentTable.FindValue (3, t)
	t = TmpTable.GetValue (t, 0) * 10
	GemRB.SetPlayerStat (MyChar, IE_REPUTATION, t)
	# set the party rep if this in the main char
	if MyChar == 1:
		GemRB.GameSetReputation (t)

	print "Reputation", t
	TmpTable = GemRB.LoadTable ("strtgold")
	a = TmpTable.GetValue (Class, 1) #number of dice
	b = TmpTable.GetValue (Class, 0) #size
	c = TmpTable.GetValue (Class, 2) #adjustment
	d = TmpTable.GetValue (Class, 3) #external multiplier
	e = TmpTable.GetValue (Class, 4) #level bonus rate
	t = GemRB.GetPlayerStat (MyChar, IE_LEVEL) #FIXME: calculate multiclass average
	if t>1:
		e=e*(t-1)
	else:
		e=0
	t = GemRB.Roll (a,b,c)*d+e
	GemRB.SetPlayerStat (MyChar, IE_GOLD, t)
	GemRB.SetPlayerStat (MyChar, IE_EA, 2 )
	#GemRB.SetPlayerStat (MyChar, IE_HATEDRACE, GemRB.GetVar ("HatedRace") )
	#Str = GemRB.GetVar ("Ability1")
	#GemRB.SetPlayerStat (MyChar, IE_STR, Str)
	#if Str == 18:
	#	GemRB.SetPlayerStat (MyChar, IE_STREXTRA, GemRB.GetVar ("StrExtra"))
	#else:
	#	GemRB.SetPlayerStat (MyChar, IE_STREXTRA, 0)

	#GemRB.SetPlayerStat (MyChar, IE_DEX, GemRB.GetVar ("Ability2"))
	#GemRB.SetPlayerStat (MyChar, IE_CON, GemRB.GetVar ("Ability3"))
	#GemRB.SetPlayerStat (MyChar, IE_INT, GemRB.GetVar ("Ability4"))
	#GemRB.SetPlayerStat (MyChar, IE_WIS, GemRB.GetVar ("Ability5"))
	#GemRB.SetPlayerStat (MyChar, IE_CHR, GemRB.GetVar ("Ability6"))

	GemRB.SetPlayerName (MyChar, GemRB.GetToken ("CHARNAME"), 0)
	GemRB.SetToken ("CHARNAME","")
	# don't reset imported char's xp back to start
	if not ImportedChar:
		GemRB.SetPlayerStat (MyChar, IE_XP, CommonTables.ClassSkills.GetValue (Class, 3) )

	GUICommon.SetColorStat (MyChar, IE_SKIN_COLOR, GemRB.GetVar ("SkinColor") )
	GUICommon.SetColorStat (MyChar, IE_HAIR_COLOR, GemRB.GetVar ("HairColor") )
	GUICommon.SetColorStat (MyChar, IE_MAJOR_COLOR, GemRB.GetVar ("MajorColor") )
	GUICommon.SetColorStat (MyChar, IE_MINOR_COLOR, GemRB.GetVar ("MinorColor") )
	GUICommon.SetColorStat (MyChar, IE_METAL_COLOR, 0x1B )
	GUICommon.SetColorStat (MyChar, IE_LEATHER_COLOR, 0x16 )
	GUICommon.SetColorStat (MyChar, IE_ARMOR_COLOR, 0x17 )

	#does all the rest
	LargePortrait = GemRB.GetToken ("LargePortrait")
	SmallPortrait = GemRB.GetToken ("SmallPortrait")
	GemRB.FillPlayerInfo (MyChar, LargePortrait, SmallPortrait)

	#10 is a weapon slot (see slottype.2da row 10)
	GemRB.CreateItem (MyChar, "staf01", 10, 1, 0, 0)
	GemRB.SetEquippedQuickSlot (MyChar, 0)

	if CharGenWindow:
		CharGenWindow.Unload ()
	GemRB.SetNextScript ("PartyFormation")
	return

def SetCharacterDescription():
	global CharGenWindow, TextArea, CharGenState, ClassFlag
	global MyChar

	TextArea.Clear()
	if CharGenState > 7:
		TextArea.Append (1047)
		TextArea.Append (": ")
		TextArea.Append (GemRB.GetToken ("CHARNAME"))
		TextArea.Append ("", -1)
	if CharGenState > 0:
		TextArea.Append (12135)
		TextArea.Append (": ")
		if GemRB.GetPlayerStat (MyChar, IE_SEX) == 1:
			TextArea.Append (1050)
		else:
			TextArea.Append (1051)
	if CharGenState > 2:
		Class = CommonTables.Classes.FindValue (5, GemRB.GetPlayerStat (MyChar, IE_CLASS) )
		TextArea.Append (12136, -1)
		TextArea.Append (": ")
		#this is only mage school in iwd
		Kit = GemRB.GetPlayerStat (MyChar, IE_KIT)
		KitIndex = KitTable.FindValue (3, Kit)
		if KitIndex <= 0:
			ClassTitle = CommonTables.Classes.GetValue (Class, 2)
		else:
			ClassTitle = KitTable.GetValue (KitIndex, 2)
		TextArea.Append (ClassTitle)

	if CharGenState > 1:
		TextArea.Append (1048, -1)
		TextArea.Append (": ")
		Race = GemRB.GetPlayerStat (MyChar, IE_RACE)
		Race = CommonTables.Races.FindValue (3, GemRB.GetPlayerStat (MyChar, IE_RACE) )
		TextArea.Append (CommonTables.Races.GetValue (Race, 2) )
	if CharGenState > 3:
		TextArea.Append (1049, -1)
		TextArea.Append (": ")
		Alignment = AlignmentTable.FindValue (3, GemRB.GetPlayerStat(MyChar, IE_ALIGNMENT) )
		TextArea.Append (AlignmentTable.GetValue (Alignment, 2) )
	if CharGenState > 4:
		strextra = GemRB.GetPlayerStat (MyChar, IE_STREXTRA)
		TextArea.Append ("", -1)
		for i in range (6):
			TextArea.Append (AbilitiesTable.GetValue (i, 2), -1)
			TextArea.Append (": " )
			StatID = AbilitiesTable.GetValue (i, 3)
			stat = GemRB.GetPlayerStat (MyChar, StatID)
			if (i == 0) and HasStrExtra and (stat==18):
				TextArea.Append (str(stat) + "/" + str(strextra) )
			else:
				TextArea.Append (str(stat) )
	if CharGenState > 5:
		ClassName = CommonTables.Classes.GetRowName (Class)
		Row = CommonTables.Classes.GetValue (Class, 5)
		DruidSpell = CommonTables.ClassSkills.GetValue (Row, 0)
		PriestSpell = CommonTables.ClassSkills.GetValue (Row, 1)
		MageSpell = CommonTables.ClassSkills.GetValue (Row, 2)
		IsBard = CommonTables.ClassSkills.GetValue (Row, 4)
		IsThief = CommonTables.ClassSkills.GetValue (Row, 5)

		if IsThief!="*":
			TextArea.Append ("", -1)
			TextArea.Append (8442, -1)
			for i in range (4):
				TextArea.Append (SkillsTable.GetValue (i+2, 2), -1)
				StatID = SkillsTable.GetValue (i+2, 3)
				TextArea.Append (": " )
				TextArea.Append (str(GemRB.GetPlayerStat (MyChar, StatID)) )
				TextArea.Append ("%" )
		elif DruidSpell!="*":
			TextArea.Append ("", -1)
			TextArea.Append (8442, -1)
			for i in range (4):
				StatID = SkillsTable.GetValue (i+2, 3)
				Stat = GemRB.GetPlayerStat (MyChar, StatID)
				if Stat>0:
					TextArea.Append (SkillsTable.GetValue (i+2, 2), -1)
					TextArea.Append (": " )
					TextArea.Append (str(Stat) )
					TextArea.Append ("%" )
			TextArea.Append ("", -1)
			TextArea.Append (15982, -1)
			TextArea.Append (": " )
			RacialEnemy = GemRB.GetVar ("RacialEnemyIndex") + GemRB.GetVar ("RacialEnemy") - 1
			TextArea.Append (RacialEnemyTable.GetValue (RacialEnemy, 3) )
		elif IsBard!="*":
			TextArea.Append ("", -1)
			TextArea.Append (8442, -1)
			for i in range (4):
				StatID = SkillsTable.GetValue (i+2, 3)
				Stat = GemRB.GetPlayerStat (MyChar, StatID)
				if Stat>0:
					TextArea.Append (SkillsTable.GetValue (i+2, 2), -1)
					TextArea.Append (": " )
					TextArea.Append (str(Stat) )
					TextArea.Append ("%" )

		TextArea.Append ("", -1)
		TextArea.Append (9466, -1)
		for i in range (15):
			StatID = ProficienciesTable.GetValue (i, 0)
			ProficiencyValue = GemRB.GetPlayerStat (MyChar, StatID )
			if ProficiencyValue > 0:
				TextArea.Append (ProficienciesTable.GetValue (i, 3), -1)
				TextArea.Append (" ")
				j = 0
				while j < ProficiencyValue:
					TextArea.Append ("+")
					j = j + 1

		if MageSpell !="*":
			TextArea.Append ("", -1)
			TextArea.Append (11027, -1)
			TextArea.Append (": " )
			t = GemRB.GetPlayerStat (MyChar, IE_ALIGNMENT)
			Learnable = Spellbook.GetLearnableMageSpells (GemRB.GetPlayerStat (MyChar, IE_KIT), t,1)
			MageSpellBook = GemRB.GetVar ("MageSpellBook")
			MageMemorized = GemRB.GetVar ("MageMemorized")
			for i in range (len(Learnable)):
				if (1 << i) & MageSpellBook:
					Spell = GemRB.GetSpell (Learnable[i])
					TextArea.Append (Spell["SpellName"], -1)
					if (1 << i) & MageMemorized:
						TextArea.Append (" +")
					TextArea.Append (" ")

		if PriestSpell == "*":
			PriestSpell = DruidSpell
		if PriestSpell!="*":
			TextArea.Append ("", -1)
			TextArea.Append (11028, -1)
			TextArea.Append (": " )
			t = GemRB.GetPlayerStat (MyChar, IE_ALIGNMENT)
			if PriestSpell == "MXSPLPRS" or PriestSpell == "MXSPLPAL":
				ClassFlag = 0x4000
			elif PriestSpell == "MXSPLDRU" or PriestSpell == "MXSPLRAN":
				ClassFlag = 0x8000
			else:
				ClassFlag = 0

			Learnable = Spellbook.GetLearnablePriestSpells( ClassFlag, t, 1)
			PriestMemorized = GemRB.GetVar ("PriestMemorized")
			for i in range (len(Learnable)):
				if (1 << i) & PriestMemorized:
					Spell = GemRB.GetSpell (Learnable[i])
					TextArea.Append (Spell["SpellName"], -1)
					TextArea.Append (" +")
	return


# Gender Selection

def GenderPress():
	global CharGenWindow, GenderWindow, GenderDoneButton, GenderTextArea
	global MyChar

	CharGenWindow.SetVisible (WINDOW_INVISIBLE)
	GenderWindow = GemRB.LoadWindow (1)
	GemRB.SetVar ("Gender", 0)
	GemRB.CreatePlayer ("charbase", MyChar | 0x8000 )

	MaleButton = GenderWindow.GetControl (2)
	MaleButton.SetState (IE_GUI_BUTTON_ENABLED)
	MaleButton.SetFlags (IE_GUI_BUTTON_RADIOBUTTON,OP_OR)
	MaleButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, MalePress)

	FemaleButton = GenderWindow.GetControl (3)
	FemaleButton.SetState (IE_GUI_BUTTON_ENABLED)
	FemaleButton.SetFlags (IE_GUI_BUTTON_RADIOBUTTON,OP_OR)
	FemaleButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, FemalePress)

	MaleButton.SetVarAssoc ("Gender", 1)
	FemaleButton.SetVarAssoc ("Gender", 2)

	GenderTextArea = GenderWindow.GetControl (5)
	GenderTextArea.SetText (17236)

	GenderDoneButton = GenderWindow.GetControl (0)
	GenderDoneButton.SetState (IE_GUI_BUTTON_DISABLED)
	GenderDoneButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, GenderDonePress)
	GenderDoneButton.SetText (11973)
	GenderDoneButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	GenderCancelButton = GenderWindow.GetControl (6)
	GenderCancelButton.SetState (IE_GUI_BUTTON_ENABLED)
	GenderCancelButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, GenderCancelPress)
	GenderCancelButton.SetText (13727)
	GenderCancelButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	GenderWindow.SetVisible (WINDOW_VISIBLE)
	return

def MalePress():
	global GenderWindow, GenderDoneButton, GenderTextArea

	GenderTextArea.SetText (13083)
	GenderDoneButton.SetState (IE_GUI_BUTTON_ENABLED)
	return

def FemalePress():
	global GenderWindow, GenderDoneButton, GenderTextArea

	GenderTextArea.SetText (13084)
	GenderDoneButton.SetState (IE_GUI_BUTTON_ENABLED)
	return

def GenderDonePress():
	global CharGenWindow, GenderWindow
	global MyChar

	if GenderWindow:
		GenderWindow.Unload ()
	Gender = GemRB.GetVar ("Gender")
	GemRB.SetPlayerStat (MyChar, IE_SEX, Gender)

	CharGenWindow.SetVisible (WINDOW_VISIBLE)
	PortraitSelect()
	return

def GenderCancelPress():
	global CharGenWindow, GenderWindow
	global MyChar

	GemRB.SetVar ("Gender", 0)
	GemRB.SetPlayerStat (MyChar, IE_SEX, 0)
	if GenderWindow:
		GenderWindow.Unload ()
	CharGenWindow.SetVisible (WINDOW_VISIBLE)
	return

def PortraitSelect():
	global CharGenWindow, PortraitWindow, Portrait, PortraitPortraitButton
	global MyChar

	CharGenWindow.SetVisible (WINDOW_INVISIBLE)
	PortraitWindow = GemRB.LoadWindow (11)

	# this is not the correct one, but I don't know which is
	Portrait = 0

	PortraitPortraitButton = PortraitWindow.GetControl (1)
	PortraitPortraitButton.SetFlags (IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_NO_IMAGE, OP_SET)

	PortraitLeftButton = PortraitWindow.GetControl (2)
	PortraitLeftButton.SetState (IE_GUI_BUTTON_ENABLED)
	PortraitLeftButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, CGPortraitLeftPress)
	PortraitLeftButton.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_OR)

	PortraitRightButton = PortraitWindow.GetControl (3)
	PortraitRightButton.SetState (IE_GUI_BUTTON_ENABLED)
	PortraitRightButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, CGPortraitRightPress)
	PortraitRightButton.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_OR)

	PortraitCustomButton = PortraitWindow.GetControl (6)
	PortraitCustomButton.SetState (IE_GUI_BUTTON_ENABLED)
	PortraitCustomButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, PortraitCustomPress)
	PortraitCustomButton.SetText (17545)

	PortraitDoneButton = PortraitWindow.GetControl (0)
	PortraitDoneButton.SetState (IE_GUI_BUTTON_ENABLED)
	PortraitDoneButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, CGPortraitDonePress)
	PortraitDoneButton.SetText (11973)
	PortraitDoneButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	PortraitCancelButton = PortraitWindow.GetControl (5)
	PortraitCancelButton.SetState (IE_GUI_BUTTON_ENABLED)
	PortraitCancelButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, CGPortraitCancelPress)
	PortraitCancelButton.SetText (13727)
	PortraitCancelButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	while PortraitsTable.GetValue (Portrait, 0) != GemRB.GetPlayerStat (MyChar, IE_SEX):
		Portrait = Portrait + 1
	PortraitPortraitButton.SetPicture (PortraitsTable.GetRowName (Portrait) + "G")

	PortraitWindow.SetVisible (WINDOW_VISIBLE)
	return

def CGPortraitLeftPress():
	global PortraitWindow, Portrait, PortraitPortraitButton
	global MyChar

	while True:
		Portrait = Portrait - 1
		if Portrait < 0:
			Portrait = PortraitsTable.GetRowCount () - 1
		if PortraitsTable.GetValue (Portrait, 0) == GemRB.GetPlayerStat(MyChar, IE_SEX):
			PortraitPortraitButton.SetPicture (PortraitsTable.GetRowName (Portrait) + "G")
			return

def CGPortraitRightPress():
	global PortraitWindow, Portrait, PortraitPortraitButton
	global MyChar

	while True:
		Portrait = Portrait + 1
		if Portrait == PortraitsTable.GetRowCount():
			Portrait = 0
		if PortraitsTable.GetValue (Portrait, 0) == GemRB.GetPlayerStat(MyChar, IE_SEX):
			PortraitPortraitButton.SetPicture (PortraitsTable.GetRowName (Portrait) + "G")
			return

def CustomDone():
	global CharGenWindow, PortraitWindow
	global PortraitButton, GenderButton, RaceButton
	global CharGenState, Portrait

	Window = CustomWindow

	PortraitName = PortraitList2.QueryText ()
	GemRB.SetToken ("SmallPortrait", PortraitName)
	PortraitName = PortraitList1.QueryText ()
	GemRB.SetToken ("LargePortrait", PortraitName)
	if Window:
		Window.Unload ()

	if PortraitWindow:
		PortraitWindow.Unload ()
	PortraitButton.SetPicture(PortraitName)
	GenderButton.SetState (IE_GUI_BUTTON_DISABLED)
	GenderButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_NAND)
	RaceButton.SetState (IE_GUI_BUTTON_ENABLED)
	RaceButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)
	CharGenState = 1
	Portrait = -1
	SetCharacterDescription()
	CharGenWindow.SetVisible (WINDOW_VISIBLE)
	return

def CustomAbort():
	if CustomWindow:
		CustomWindow.Unload ()
	return

def CGLargeCustomPortrait():
	Window = CustomWindow

	Portrait = PortraitList1.QueryText ()
	#small hack
	if GemRB.GetVar ("Row1") == RowCount1:
		return

	Label = Window.GetControl (0x10000007)
	Label.SetText (Portrait)

	Button = Window.GetControl (6)
	if Portrait=="":
		Portrait = "NOPORTMD"
		Button.SetState (IE_GUI_BUTTON_DISABLED)
	else:
		if PortraitList2.QueryText ()!="":
			Button.SetState (IE_GUI_BUTTON_ENABLED)

	Button = Window.GetControl (0)
	Button.SetPicture (Portrait, "NOPORTMD")
	return

def CGSmallCustomPortrait():
	Window = CustomWindow

	Portrait = PortraitList2.QueryText ()
	#small hack
	if GemRB.GetVar ("Row2") == RowCount2:
		return

	Label = Window.GetControl (0x10000008)
	Label.SetText (Portrait)

	Button = Window.GetControl (6)
	if Portrait=="":
		Portrait = "NOPORTSM"
		Button.SetState (IE_GUI_BUTTON_DISABLED)
	else:
		if PortraitList1.QueryText ()!="":
			Button.SetState (IE_GUI_BUTTON_ENABLED)

	Button = Window.GetControl (1)
	Button.SetPicture (Portrait, "NOPORTSM")
	return

def PortraitCustomPress():
	global PortraitList1, PortraitList2
	global RowCount1, RowCount2
	global CustomWindow

	CustomWindow = Window = GemRB.LoadWindow (18)
	PortraitList1 = Window.GetControl (2)
	RowCount1 = PortraitList1.GetPortraits (0)
	PortraitList1.SetEvent (IE_GUI_TEXTAREA_ON_CHANGE, CGLargeCustomPortrait)
	GemRB.SetVar ("Row1", RowCount1)
	PortraitList1.SetVarAssoc ("Row1",RowCount1)

	PortraitList2 = Window.GetControl (4)
	RowCount2 = PortraitList2.GetPortraits (1)
	PortraitList2.SetEvent (IE_GUI_TEXTAREA_ON_CHANGE, CGSmallCustomPortrait)
	GemRB.SetVar ("Row2", RowCount2)
	PortraitList2.SetVarAssoc ("Row2",RowCount2)

	Button = Window.GetControl (6)
	Button.SetText (11973)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, CustomDone)
	Button.SetState (IE_GUI_BUTTON_DISABLED)

	Button = Window.GetControl (7)
	Button.SetText (13727)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, CustomAbort)

	Button = Window.GetControl (0)
	PortraitName = PortraitsTable.GetRowName (Portrait)+"L"
	Button.SetPicture (PortraitName, "NOPORTMD")
	Button.SetState (IE_GUI_BUTTON_LOCKED)

	Button = Window.GetControl (1)
	PortraitName = PortraitsTable.GetRowName (Portrait)+"S"
	Button.SetPicture (PortraitName, "NOPORTSM")
	Button.SetState (IE_GUI_BUTTON_LOCKED)

	Window.ShowModal (MODAL_SHADOW_NONE)
	return

def CGPortraitDonePress():
	global CharGenWindow, PortraitWindow, PortraitButton, GenderButton, RaceButton
	global CharGenState, Portrait

	PortraitName = PortraitsTable.GetRowName (Portrait )
	GemRB.SetToken ("SmallPortrait", PortraitName+"S")
	GemRB.SetToken ("LargePortrait", PortraitName+"L")
	PortraitButton.SetPicture(PortraitsTable.GetRowName (Portrait) + "L")
	GenderButton.SetState (IE_GUI_BUTTON_DISABLED)
	GenderButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_NAND)
	RaceButton.SetState (IE_GUI_BUTTON_ENABLED)
	RaceButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)
	CharGenState = 1
	SetCharacterDescription()
	CharGenWindow.SetVisible (WINDOW_VISIBLE)
	if PortraitWindow:
		PortraitWindow.Unload ()
	return

def CGPortraitCancelPress():
	global CharGenWindow, PortraitWindow

	if PortraitWindow:
		PortraitWindow.Unload ()
	CharGenWindow.SetVisible (WINDOW_VISIBLE)
	return

# Race Selection

def RacePress():
	global CharGenWindow, RaceWindow, RaceDoneButton, RaceTextArea

	CharGenWindow.SetVisible (WINDOW_INVISIBLE)
	RaceWindow = GemRB.LoadWindow (8)
	GemRB.SetVar ("Race", 0)

	for i in range (2, 8):
		RaceSelectButton = RaceWindow.GetControl (i)
		RaceSelectButton.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_OR)

	for i in range (2, 8):
		RaceSelectButton = RaceWindow.GetControl (i)
		RaceSelectButton.SetState (IE_GUI_BUTTON_ENABLED)
		RaceSelectButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, RaceSelectPress)
		RaceSelectButton.SetText (CommonTables.Races.GetValue (i - 2, 0))
		RaceSelectButton.SetVarAssoc ("Race", i - 1)

	RaceTextArea = RaceWindow.GetControl (8)
	RaceTextArea.SetText (17237)

	RaceDoneButton = RaceWindow.GetControl (0)
	RaceDoneButton.SetState (IE_GUI_BUTTON_DISABLED)
	RaceDoneButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, RaceDonePress)
	RaceDoneButton.SetText (11973)
	RaceDoneButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	RaceCancelButton = RaceWindow.GetControl (10)
	RaceCancelButton.SetState (IE_GUI_BUTTON_ENABLED)
	RaceCancelButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, RaceCancelPress)
	RaceCancelButton.SetText (13727)
	RaceCancelButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	RaceWindow.SetVisible (WINDOW_VISIBLE)
	return

def RaceSelectPress():
	global RaceWindow, RaceDoneButton, RaceTextArea

	Race = GemRB.GetVar ("Race") - 1
	RaceTextArea.SetText (CommonTables.Races.GetValue (Race, 1) )
	RaceDoneButton.SetState (IE_GUI_BUTTON_ENABLED)
	return

def RaceDonePress():
	global CharGenWindow, CharGenState, RaceWindow, RaceButton, ClassButton

	if RaceWindow:
		RaceWindow.Unload ()
	RaceButton.SetState (IE_GUI_BUTTON_DISABLED)
	RaceButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_NAND)
	ClassButton.SetState (IE_GUI_BUTTON_ENABLED)
	ClassButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)
	CharGenState = 2

	Race = GemRB.GetVar ("Race")-1
	Race = CommonTables.Races.GetValue (Race, 3)
	GemRB.SetPlayerStat (MyChar, IE_RACE, Race)
	SetCharacterDescription()
	CharGenWindow.SetVisible (WINDOW_VISIBLE)
	return

def RaceCancelPress():
	global CharGenWindow, RaceWindow

	if RaceWindow:
		RaceWindow.Unload ()
	CharGenWindow.SetVisible (WINDOW_VISIBLE)
	return

# Class Selection

def ClassPress():
	global CharGenWindow, ClassWindow, ClassTextArea, ClassDoneButton

	CharGenWindow.SetVisible (WINDOW_INVISIBLE)
	ClassWindow = GemRB.LoadWindow (2)
	ClassCount = CommonTables.Classes.GetRowCount ()
	RaceRow = CommonTables.Races.FindValue (3, GemRB.GetPlayerStat (MyChar, IE_RACE) )
	RaceName = CommonTables.Races.GetRowName (RaceRow)
	GemRB.SetVar ("Class", 0)
	GemRB.SetVar ("Class Kit", 0)
	GemRB.SetVar ("MAGESCHOOL", 0)

	for i in range (2, 10):
		ClassSelectButton = ClassWindow.GetControl (i)
		ClassSelectButton.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_SET)

	HasMulti = 0
	j = 2
	for i in range (ClassCount):
		Allowed = CommonTables.Classes.GetValue (CommonTables.Classes.GetRowName (i), RaceName)
		if CommonTables.Classes.GetValue (i, 4):
			if Allowed != 0:
				HasMulti = 1
		else:
			ClassSelectButton = ClassWindow.GetControl (j)
			j = j + 1
			if Allowed > 0:
				ClassSelectButton.SetState (IE_GUI_BUTTON_ENABLED)
			else:
				ClassSelectButton.SetState (IE_GUI_BUTTON_DISABLED)
			ClassSelectButton.SetEvent (IE_GUI_BUTTON_ON_PRESS,  ClassSelectPress)
			ClassSelectButton.SetText (CommonTables.Classes.GetValue (i, 0) )
			ClassSelectButton.SetVarAssoc ("Class", i + 1)

	ClassMultiButton = ClassWindow.GetControl (10)
	if HasMulti == 0:
		ClassMultiButton.SetState (IE_GUI_BUTTON_DISABLED)
	else:
		ClassMultiButton.SetState (IE_GUI_BUTTON_ENABLED)
	ClassMultiButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, ClassMultiPress)
	ClassMultiButton.SetText (11993)

	KitButton = ClassWindow.GetControl (11)
	#only the mage class has schools
	Allowed = CommonTables.Classes.GetValue ("MAGE", RaceName)
	if Allowed:
		KitButton.SetState (IE_GUI_BUTTON_ENABLED)
	else:
		KitButton.SetState (IE_GUI_BUTTON_DISABLED)
	KitButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, KitPress)
	KitButton.SetText (11994)

	ClassTextArea = ClassWindow.GetControl (13)
	ClassTextArea.SetText (17242)

	ClassDoneButton = ClassWindow.GetControl (0)
	ClassDoneButton.SetState (IE_GUI_BUTTON_DISABLED)
	ClassDoneButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, ClassDonePress)
	ClassDoneButton.SetText (11973)
	ClassDoneButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	ClassCancelButton = ClassWindow.GetControl (14)
	ClassCancelButton.SetState (IE_GUI_BUTTON_ENABLED)
	ClassCancelButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, ClassCancelPress)
	ClassCancelButton.SetText (13727)
	ClassCancelButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	ClassWindow.SetVisible (WINDOW_VISIBLE)
	return

def ClassSelectPress():
	global ClassWindow, ClassTextArea, ClassDoneButton

	Class = GemRB.GetVar ("Class") - 1
	ClassTextArea.SetText (CommonTables.Classes.GetValue (Class, 1) )
	ClassDoneButton.SetState (IE_GUI_BUTTON_ENABLED)
	return

def ClassMultiPress():
	global ClassWindow, ClassMultiWindow, ClassMultiTextArea, ClassMultiDoneButton

	ClassWindow.SetVisible (WINDOW_INVISIBLE)
	ClassMultiWindow = GemRB.LoadWindow (10)
	ClassCount = CommonTables.Classes.GetRowCount ()
	RaceRow = CommonTables.Races.FindValue (3, GemRB.GetPlayerStat (MyChar, IE_RACE) )
	RaceName = CommonTables.Races.GetRowName (RaceRow)

	print "Multi racename:", RaceName
	for i in range (2, 10):
		ClassMultiSelectButton = ClassMultiWindow.GetControl (i)
		ClassMultiSelectButton.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_SET)

	j = 2
	for i in range (ClassCount):
		ClassName = CommonTables.Classes.GetRowName (i)
		if (CommonTables.Classes.GetValue (ClassName, "MULTI") > 0):
			ClassMultiSelectButton = ClassMultiWindow.GetControl (j)
			j = j + 1
			if (CommonTables.Classes.GetValue (ClassName, RaceName) > 0):
				ClassMultiSelectButton.SetState (IE_GUI_BUTTON_ENABLED)
			else:
				ClassMultiSelectButton.SetState (IE_GUI_BUTTON_DISABLED)
			ClassMultiSelectButton.SetEvent (IE_GUI_BUTTON_ON_PRESS,  ClassMultiSelectPress)
			ClassMultiSelectButton.SetText (CommonTables.Classes.GetValue (i, 0) )
			ClassMultiSelectButton.SetVarAssoc ("Class", i + 1)

	ClassMultiTextArea = ClassMultiWindow.GetControl (12)
	ClassMultiTextArea.SetText (17244)

	ClassMultiDoneButton = ClassMultiWindow.GetControl (0)
	ClassMultiDoneButton.SetState (IE_GUI_BUTTON_DISABLED)
	ClassMultiDoneButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, ClassMultiDonePress)
	ClassMultiDoneButton.SetText (11973)
	ClassMultiDoneButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	ClassMultiCancelButton = ClassMultiWindow.GetControl (14)
	ClassMultiCancelButton.SetState (IE_GUI_BUTTON_ENABLED)
	ClassMultiCancelButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, ClassMultiCancelPress)
	ClassMultiCancelButton.SetText (13727)
	ClassMultiCancelButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	ClassMultiWindow.SetVisible (WINDOW_VISIBLE)
	return

def ClassMultiSelectPress():
	global ClassMultiWindow, ClassMultiTextArea, ClassMultiDoneButton

	Class = GemRB.GetVar ("Class") - 1
	ClassMultiTextArea.SetText (CommonTables.Classes.GetValue (Class, 1) )
	ClassMultiDoneButton.SetState (IE_GUI_BUTTON_ENABLED)
	return

def ClassMultiDonePress():
	global ClassMultiWindow

	if ClassMultiWindow:
		ClassMultiWindow.Unload ()
	ClassDonePress()
	return

def ClassMultiCancelPress():
	global ClassWindow, ClassMultiWindow

	if ClassMultiWindow:
		ClassMultiWindow.Unload ()
	ClassWindow.SetVisible (WINDOW_VISIBLE)
	return

def KitPress():
	global ClassWindow, KitWindow, KitTextArea, KitDoneButton

	ClassWindow.SetVisible (WINDOW_INVISIBLE)
	KitWindow = GemRB.LoadWindow (12)

	#only mage class (1) has schools. It is the sixth button
	GemRB.SetVar ("Class", 6)
	GemRB.SetVar ("Class Kit",0)
	GemRB.SetVar ("MAGESCHOOL",0)

	for i in range (8):
		Button = KitWindow.GetControl (i+2)
		Button.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
		Button.SetText (KitTable.GetValue (i+1, 0) )
		Button.SetVarAssoc ("MAGESCHOOL", i+1)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, KitSelectPress)

	KitTextArea = KitWindow.GetControl (11)
	KitTextArea.SetText (17245)

	KitDoneButton = KitWindow.GetControl (0)
	KitDoneButton.SetState (IE_GUI_BUTTON_DISABLED)
	KitDoneButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, KitDonePress)
	KitDoneButton.SetText (11973)
	KitDoneButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	KitCancelButton = KitWindow.GetControl (12)
	KitCancelButton.SetState (IE_GUI_BUTTON_ENABLED)
	KitCancelButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, KitCancelPress)
	KitCancelButton.SetText (13727)
	KitCancelButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	KitWindow.SetVisible (WINDOW_VISIBLE)
	return

def KitSelectPress():
	global KitWindow, KitTextArea

	Kit = GemRB.GetVar ("MAGESCHOOL")
	KitTextArea.SetText (KitTable.GetValue (Kit, 1))
	KitDoneButton.SetState (IE_GUI_BUTTON_ENABLED)
	return

def KitDonePress():
	global KitWindow

	if KitWindow:
		KitWindow.Unload ()
	ClassDonePress()
	return

def KitCancelPress():
	global ClassWindow, KitWindow

	if KitWindow:
		KitWindow.Unload ()
	ClassWindow.SetVisible (WINDOW_VISIBLE)
	return

def ClassDonePress():
	global CharGenWindow, CharGenState, ClassWindow, ClassButton, AlignmentButton
	global MyChar

	if ClassWindow:
		ClassWindow.Unload ()
	ClassButton.SetState (IE_GUI_BUTTON_DISABLED)
	ClassButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_NAND)
	AlignmentButton.SetState (IE_GUI_BUTTON_ENABLED)
	AlignmentButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	ClassIndex = GemRB.GetVar ("Class")-1
	Class = CommonTables.Classes.GetValue (ClassIndex, 5)
	GemRB.SetPlayerStat (MyChar, IE_CLASS, Class)

	Kit = KitTable.GetValue (GemRB.GetVar ("MAGESCHOOL"), 3 )
	if (Kit == -1 ):
		Kit = 0x4000

	GemRB.SetPlayerStat (MyChar, IE_KIT, Kit)

	CharGenState = 3
	SetCharacterDescription()
	CharGenWindow.SetVisible (WINDOW_VISIBLE)
	return

def ClassCancelPress():
	global CharGenWindow, ClassWindow

	if ClassWindow:
		ClassWindow.Unload ()
	CharGenWindow.SetVisible (WINDOW_VISIBLE)
	return

# Alignment Selection

def AlignmentPress():
	global CharGenWindow, AlignmentWindow, AlignmentTextArea, AlignmentDoneButton

	CharGenWindow.SetVisible (WINDOW_INVISIBLE)
	AlignmentWindow = GemRB.LoadWindow (3)
	ClassAlignmentTable = GemRB.LoadTable ("alignmnt")
	Class = GemRB.GetPlayerStat (MyChar, IE_CLASS)
	ClassIndex = CommonTables.Classes.FindValue (5, Class)
	ClassName = CommonTables.Classes.GetRowName (ClassIndex)
	GemRB.SetVar ("Alignment", 0)

	for i in range (2, 11):
		AlignmentSelectButton = AlignmentWindow.GetControl (i)
		AlignmentSelectButton.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_OR)

	for i in range (9):
		AlignmentSelectButton = AlignmentWindow.GetControl (i + 2)
		if ClassAlignmentTable.GetValue (ClassName, AlignmentTable.GetValue(i, 4)) == 0:
			AlignmentSelectButton.SetState (IE_GUI_BUTTON_DISABLED)
		else:
			AlignmentSelectButton.SetState (IE_GUI_BUTTON_ENABLED)
		AlignmentSelectButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, AlignmentSelectPress)
		AlignmentSelectButton.SetText (AlignmentTable.GetValue (i, 0) )
		AlignmentSelectButton.SetVarAssoc ("Alignment", i + 1)

	AlignmentTextArea = AlignmentWindow.GetControl (11)
	AlignmentTextArea.SetText (9602)

	AlignmentDoneButton = AlignmentWindow.GetControl (0)
	AlignmentDoneButton.SetState (IE_GUI_BUTTON_DISABLED)
	AlignmentDoneButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, AlignmentDonePress)
	AlignmentDoneButton.SetText (11973)
	AlignmentDoneButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	AlignmentCancelButton = AlignmentWindow.GetControl (13)
	AlignmentCancelButton.SetState (IE_GUI_BUTTON_ENABLED)
	AlignmentCancelButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, AlignmentCancelPress)
	AlignmentCancelButton.SetText (13727)
	AlignmentCancelButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	AlignmentWindow.SetVisible (WINDOW_VISIBLE)
	return

def AlignmentSelectPress():
	global AlignmentWindow, AlignmentTextArea, AlignmentDoneButton

	Alignment = GemRB.GetVar ("Alignment") - 1
	AlignmentTextArea.SetText (AlignmentTable.GetValue (Alignment, 1))
	AlignmentDoneButton.SetState (IE_GUI_BUTTON_ENABLED)
	return

def AlignmentDonePress():
	global CharGenWindow, CharGenState, AlignmentWindow, AlignmentButton, AbilitiesButton
	global MyChar

	if AlignmentWindow:
		AlignmentWindow.Unload ()
	AlignmentButton.SetState (IE_GUI_BUTTON_DISABLED)
	AlignmentButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_NAND)
	AbilitiesButton.SetState (IE_GUI_BUTTON_ENABLED)
	AbilitiesButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	Alignment = AlignmentTable.GetValue (GemRB.GetVar ("Alignment")-1, 3)
	GemRB.SetPlayerStat (MyChar, IE_ALIGNMENT, Alignment )

	CharGenState = 4
	SetCharacterDescription()
	CharGenWindow.SetVisible (WINDOW_VISIBLE)
	return

def AlignmentCancelPress():
	global CharGenWindow, AlignmentWindow

	if AlignmentWindow:
		AlignmentWindow.Unload ()
	CharGenWindow.SetVisible (WINDOW_VISIBLE)
	return

# Abilities Selection

def AbilitiesPress():
	global CharGenWindow, AbilitiesWindow
	global AbilitiesTextArea, AbilitiesRecallButton, AbilitiesDoneButton
	global AbilitiesRaceAddTable, AbilitiesRaceReqTable, AbilitiesClassReqTable
	global HasStrExtra

	GemRB.SetRepeatClickFlags(GEM_RK_DISABLE, OP_NAND)
	CharGenWindow.SetVisible (WINDOW_INVISIBLE)
	AbilitiesWindow = GemRB.LoadWindow (4)
	AbilitiesRaceAddTable = GemRB.LoadTable ("ABRACEAD")
	AbilitiesRaceReqTable = GemRB.LoadTable ("ABRACERQ")
	AbilitiesClassReqTable = GemRB.LoadTable ("ABCLASRQ")

	PointsLeftLabel = AbilitiesWindow.GetControl (0x10000002)
	PointsLeftLabel.SetUseRGB (1)

	Class = GemRB.GetPlayerStat (MyChar, IE_CLASS)
	Class = CommonTables.Classes.FindValue (5, Class)
	HasStrExtra = CommonTables.Classes.GetValue (Class, 3)=="SAVEWAR"

	for i in range (6):
		AbilitiesLabelButton = AbilitiesWindow.GetControl (30 + i)
		AbilitiesLabelButton.SetState (IE_GUI_BUTTON_ENABLED)
		AbilitiesLabelButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, AbilitiesLabelPress)
		AbilitiesLabelButton.SetVarAssoc ("AbilityIndex", i + 1)

		AbilitiesPlusButton = AbilitiesWindow.GetControl (16 + i * 2)
		AbilitiesPlusButton.SetState (IE_GUI_BUTTON_ENABLED)
		AbilitiesPlusButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, AbilitiesPlusPress)
		AbilitiesPlusButton.SetVarAssoc ("AbilityIndex", i + 1)

		AbilitiesMinusButton = AbilitiesWindow.GetControl (17 + i * 2)
		AbilitiesMinusButton.SetState (IE_GUI_BUTTON_ENABLED)
		AbilitiesMinusButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, AbilitiesMinusPress)
		AbilitiesMinusButton.SetVarAssoc ("AbilityIndex", i + 1)

		AbilityLabel = AbilitiesWindow.GetControl (0x10000003 + i)
		AbilityLabel.SetUseRGB (1)

	AbilitiesStoreButton = AbilitiesWindow.GetControl (37)
	AbilitiesStoreButton.SetState (IE_GUI_BUTTON_ENABLED)
	AbilitiesStoreButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, AbilitiesStorePress)
	AbilitiesStoreButton.SetText (17373)

	AbilitiesRecallButton = AbilitiesWindow.GetControl (38)
	AbilitiesRecallButton.SetState (IE_GUI_BUTTON_DISABLED)
	AbilitiesRecallButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, AbilitiesRecallPress)
	AbilitiesRecallButton.SetText (17374)

	AbilitiesRerollButton = AbilitiesWindow.GetControl (2)
	AbilitiesRerollButton.SetState (IE_GUI_BUTTON_ENABLED)
	AbilitiesRerollButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, AbilitiesRerollPress)
	AbilitiesRerollButton.SetText (11982)

	AbilitiesTextArea = AbilitiesWindow.GetControl (29)
	AbilitiesTextArea.SetText (17247)

	AbilitiesDoneButton = AbilitiesWindow.GetControl (0)
	AbilitiesDoneButton.SetState (IE_GUI_BUTTON_ENABLED)
	AbilitiesDoneButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, AbilitiesDonePress)
	AbilitiesDoneButton.SetText (11973)
	AbilitiesDoneButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	AbilitiesCancelButton = AbilitiesWindow.GetControl (36)
	AbilitiesCancelButton.SetState (IE_GUI_BUTTON_ENABLED)
	AbilitiesCancelButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, AbilitiesCancelPress)
	AbilitiesCancelButton.SetText (13727)
	AbilitiesCancelButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	AbilitiesRerollPress()

	AbilitiesWindow.SetVisible (WINDOW_VISIBLE)
	return

def AbilitiesCalcLimits(Index):
	global AbilitiesRaceReqTable, AbilitiesRaceAddTable, AbilitiesClassReqTable
	global AbilitiesMinimum, AbilitiesMaximum, AbilitiesModifier

	RaceName = CommonTables.Races.GetRowName (GemRB.GetPlayerStat (MyChar, IE_RACE) - 1)
	Race = AbilitiesRaceReqTable.GetRowIndex (RaceName)
	AbilitiesMinimum = AbilitiesRaceReqTable.GetValue (Race, Index * 2)
	AbilitiesMaximum = AbilitiesRaceReqTable.GetValue (Race, Index * 2 + 1)
	AbilitiesModifier = AbilitiesRaceAddTable.GetValue (Race, Index)

	Class = CommonTables.Classes.FindValue (5, GemRB.GetPlayerStat (MyChar, IE_CLASS) )
	ClassName = CommonTables.Classes.GetRowName (Class)
	ClassIndex = AbilitiesClassReqTable.GetRowIndex (ClassName)
	Min = AbilitiesClassReqTable.GetValue (ClassIndex, Index)
	if Min > 0 and AbilitiesMinimum < Min:
		AbilitiesMinimum = Min

	AbilitiesMinimum = AbilitiesMinimum + AbilitiesModifier
	AbilitiesMaximum = AbilitiesMaximum + AbilitiesModifier
	return

def AbilitiesLabelPress():
	global AbilitiesWindow, AbilitiesTextArea

	AbilityIndex = GemRB.GetVar ("AbilityIndex") - 1
	AbilitiesCalcLimits(AbilityIndex)
	GemRB.SetToken ("MINIMUM", str(AbilitiesMinimum) )
	GemRB.SetToken ("MAXIMUM", str(AbilitiesMaximum) )
	AbilitiesTextArea.SetText (AbilitiesTable.GetValue (AbilityIndex, 1) )
	return

def AbilitiesPlusPress():
	global AbilitiesWindow, AbilitiesTextArea
	global AbilitiesMinimum, AbilitiesMaximum

	Abidx = GemRB.GetVar ("AbilityIndex") - 1
	AbilitiesCalcLimits(Abidx)
	GemRB.SetToken ("MINIMUM", str(AbilitiesMinimum) )
	GemRB.SetToken ("MAXIMUM", str(AbilitiesMaximum) )
	AbilitiesTextArea.SetText (AbilitiesTable.GetValue (Abidx, 1) )
	PointsLeft = GemRB.GetVar ("Ability0")
	Ability = GemRB.GetVar ("Ability" + str(Abidx + 1) )
	if PointsLeft > 0 and Ability < AbilitiesMaximum:
		PointsLeft = PointsLeft - 1
		GemRB.SetVar ("Ability0", PointsLeft)
		PointsLeftLabel = AbilitiesWindow.GetControl (0x10000002)
		PointsLeftLabel.SetText (str(PointsLeft) )
		Ability = Ability + 1
		GemRB.SetVar ("Ability" + str(Abidx + 1), Ability)
		Label = AbilitiesWindow.GetControl (0x10000003 + Abidx)
		StrExtra = GemRB.GetVar("StrExtra")
		if Abidx==0 and Ability==18 and HasStrExtra:
			Label.SetText("18/"+str(StrExtra) )
		else:
			Label.SetText(str(Ability) )

		if PointsLeft == 0:
			AbilitiesDoneButton.SetState (IE_GUI_BUTTON_ENABLED)
	return

def AbilitiesMinusPress():
	global AbilitiesWindow, AbilitiesTextArea
	global AbilitiesMinimum, AbilitiesMaximum

	Abidx = GemRB.GetVar ("AbilityIndex") - 1
	AbilitiesCalcLimits(Abidx)
	GemRB.SetToken ("MINIMUM", str(AbilitiesMinimum) )
	GemRB.SetToken ("MAXIMUM", str(AbilitiesMaximum) )
	AbilitiesTextArea.SetText (AbilitiesTable.GetValue (Abidx, 1) )
	PointsLeft = GemRB.GetVar ("Ability0")
	Ability = GemRB.GetVar ("Ability" + str(Abidx + 1) )
	if Ability > AbilitiesMinimum:
		Ability = Ability - 1
		GemRB.SetVar ("Ability" + str(Abidx + 1), Ability)
		Label = AbilitiesWindow.GetControl (0x10000003 + Abidx)
		StrExtra = GemRB.GetVar("StrExtra")
		if Abidx==0 and Ability==18 and HasStrExtra:
			Label.SetText("18/"+str(StrExtra) )
		else:
			Label.SetText(str(Ability) )

		PointsLeft = PointsLeft + 1
		GemRB.SetVar ("Ability0", PointsLeft)
		PointsLeftLabel = AbilitiesWindow.GetControl (0x10000002)
		PointsLeftLabel.SetText (str(PointsLeft) )
		AbilitiesDoneButton.SetState (IE_GUI_BUTTON_DISABLED)
	return

def AbilitiesStorePress():
	global AbilitiesWindow, AbilitiesRecallButton

	GemRB.SetVar("StoredStrExtra", GemRB.GetVar ("StrExtra") )
	for i in range (7):
		GemRB.SetVar ("Stored" + str(i), GemRB.GetVar ("Ability" + str(i)))
	AbilitiesRecallButton.SetState (IE_GUI_BUTTON_ENABLED)
	return

def AbilitiesRecallPress():
	global AbilitiesWindow

	AbilitiesWindow.Invalidate ()
	e=GemRB.GetVar("StoredStrExtra")
	GemRB.SetVar("StrExtra",e)
	for i in range (7):
		v =  GemRB.GetVar ("Stored" + str(i))
		GemRB.SetVar ("Ability" + str(i), v)
		Label = AbilitiesWindow.GetControl (0x10000002 + i)
		if i==0 and v==18 and HasStrExtra==1:
			Label.SetText("18/"+str(e) )
		else:
			Label.SetText(str(v) )

	PointsLeft = GemRB.GetVar("Ability0")
	if PointsLeft == 0:
		AbilitiesDoneButton.SetState(IE_GUI_BUTTON_ENABLED)
	else:
		AbilitiesDoneButton.SetState(IE_GUI_BUTTON_DISABLED)
	return

def AbilitiesRerollPress():
	global AbilitiesWindow, AbilitiesMinimum, AbilitiesMaximum, AbilitiesModifier

	AbilitiesWindow.Invalidate ()
	GemRB.SetVar ("Ability0", 0)
	PointsLeftLabel = AbilitiesWindow.GetControl (0x10000002)
	PointsLeftLabel.SetText ("0")
	Dices = 3
	Sides = 5

	#roll strextra even when the current stat is not 18
	if HasStrExtra:
		e = GemRB.Roll (1,100,0)
	else:
		e = 0
	GemRB.SetVar("StrExtra", e)
	for i in range (6):
		AbilitiesCalcLimits(i)
		Value = GemRB.Roll (Dices, Sides, AbilitiesModifier+3)
		if Value < AbilitiesMinimum:
			Value = AbilitiesMinimum
		if Value > AbilitiesMaximum:
			Value = AbilitiesMaximum
		GemRB.SetVar ("Ability" + str(i + 1), Value)
		Label = AbilitiesWindow.GetControl (0x10000003 + i)

		if i==0 and HasStrExtra and Value==18:
			Label.SetText("18/"+str(e) )
		else:
			Label.SetText(str(Value) )

	AbilitiesDoneButton.SetState(IE_GUI_BUTTON_ENABLED)
	return

def AbilitiesDonePress():
	global CharGenWindow, CharGenState, AbilitiesWindow, AbilitiesButton, SkillsButton, SkillsState

	if AbilitiesWindow:
		AbilitiesWindow.Unload ()
	AbilitiesButton.SetState (IE_GUI_BUTTON_DISABLED)
	AbilitiesButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_NAND)
	SkillsButton.SetState (IE_GUI_BUTTON_ENABLED)
	SkillsButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	Str = GemRB.GetVar ("Ability1")
	GemRB.SetPlayerStat (MyChar, IE_STR, Str)
	if Str == 18:
		GemRB.SetPlayerStat (MyChar, IE_STREXTRA, GemRB.GetVar ("StrExtra"))
	else:
		GemRB.SetPlayerStat (MyChar, IE_STREXTRA, 0)

	GemRB.SetPlayerStat (MyChar, IE_DEX, GemRB.GetVar ("Ability2"))
	GemRB.SetPlayerStat (MyChar, IE_CON, GemRB.GetVar ("Ability3"))
	GemRB.SetPlayerStat (MyChar, IE_INT, GemRB.GetVar ("Ability4"))
	GemRB.SetPlayerStat (MyChar, IE_WIS, GemRB.GetVar ("Ability5"))
	GemRB.SetPlayerStat (MyChar, IE_CHR, GemRB.GetVar ("Ability6"))

	CharGenState = 5
	SkillsState = 0
	SetCharacterDescription()
	GemRB.SetRepeatClickFlags(GEM_RK_DISABLE, OP_OR)
	CharGenWindow.SetVisible (WINDOW_VISIBLE)
	return

def AbilitiesCancelPress():
	global CharGenWindow, AbilitiesWindow

	if AbilitiesWindow:
		AbilitiesWindow.Unload ()
	GemRB.SetRepeatClickFlags(GEM_RK_DISABLE, OP_OR)
	CharGenWindow.SetVisible (WINDOW_VISIBLE)
	return

# Skills Selection

def SkillsPress():
	global CharGenWindow, AppearanceButton
	global SkillsState, SkillsButton, CharGenState, ClassFlag

	Level = 1
	SpellLevel = 1
	Class = GemRB.GetPlayerStat (MyChar, IE_CLASS)
	DruidSpell = CommonTables.ClassSkills.GetValue (Class, 0)
	PriestSpell = CommonTables.ClassSkills.GetValue (Class, 1)
	MageSpell = CommonTables.ClassSkills.GetValue (Class, 2)
	IsBard = CommonTables.ClassSkills.GetValue (Class, 4)
	IsThief = CommonTables.ClassSkills.GetValue (Class, 5)

	if SkillsState == 0:
		GemRB.SetVar ("HatedRace", 0)
		if IsThief!="*":
			SkillsSelect()
		elif DruidSpell!="*":
			Skill = GemRB.LoadTable("SKILLRNG").GetValue(str(Level), "STEALTH")
			GemRB.SetPlayerStat (MyChar, IE_STEALTH, Skill)
			RacialEnemySelect()
		elif IsBard!="*":
			Skill = GemRB.LoadTable(IsBard).GetValue(str(Level), "PICK_POCKETS")
			GemRB.SetPlayerStat (MyChar, IE_PICKPOCKET, Skill)
			SkillsState = 1
		else:
			SkillsState = 1

	if SkillsState == 1:
		ProficienciesSelect()

	if SkillsState == 2:
		if MageSpell!="*":
			MageSpellsSelect(MageSpell, Level, SpellLevel)
		else:
			SkillsState = 3

	if SkillsState == 3:
		if MageSpell!="*":
			MageSpellsMemorize(MageSpell, Level, SpellLevel)
		else:
			SkillsState = 4

	if SkillsState == 4:
		if PriestSpell=="MXSPLPRS" or PriestSpell =="MXSPLPAL":
			ClassFlag = 0x4000
			PriestSpellsMemorize(PriestSpell, Level, SpellLevel)
		elif DruidSpell=="MXSPLDRU" or DruidSpell =="MXSPLRAN":
			#no separate spell progression
			if DruidSpell == "MXSPLDRU":
				DruidSpell = "MXSPLPRS"
			ClassFlag = 0x8000
			PriestSpellsMemorize(DruidSpell, Level, SpellLevel)
		else:
			SkillsState = 5

	if SkillsState == 5:
		SkillsButton.SetState (IE_GUI_BUTTON_DISABLED)
		SkillsButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_NAND)
		AppearanceButton.SetState (IE_GUI_BUTTON_ENABLED)
		AppearanceButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

		Race = GemRB.GetVar ("HatedRace")
		GemRB.SetPlayerStat (MyChar, IE_HATEDRACE, Race)

		ProfCount = ProficienciesTable.GetRowCount ()
		for i in range(ProfCount):
			StatID = ProficienciesTable.GetValue (i, 0)
			Value = GemRB.GetVar ("Proficiency"+str(i) )
			GemRB.SetPlayerStat (MyChar, StatID, Value )

		CharGenState = 6
		SetCharacterDescription()
	return

def SkillsSelect():
	global CharGenWindow, SkillsWindow, SkillsTextArea, SkillsDoneButton, SkillsPointsLeft

	CharGenWindow.SetVisible (WINDOW_INVISIBLE)
	SkillsWindow = GemRB.LoadWindow (6)

	Levels = [GemRB.GetPlayerStat (MyChar, IE_LEVEL), \
		GemRB.GetPlayerStat (MyChar, IE_LEVEL2), \
		GemRB.GetPlayerStat (MyChar, IE_LEVEL3)]

	LUSkillsSelection.SetupSkillsWindow (MyChar, \
		LUSkillsSelection.LUSKILLS_TYPE_CHARGEN, SkillsWindow, RedrawSkills, [0,0,0], Levels, 0, False)

	SkillsPointsLeft = GemRB.GetVar ("SkillPointsLeft")
	if SkillsPointsLeft<=0:
		SkillsDonePress()
		return

	SkillsDoneButton = SkillsWindow.GetControl (0)
	SkillsDoneButton.SetState (IE_GUI_BUTTON_DISABLED)
	SkillsDoneButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, SkillsDonePress)
	SkillsDoneButton.SetText (11973)
	SkillsDoneButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	SkillsCancelButton = SkillsWindow.GetControl (25)
	SkillsCancelButton.SetState (IE_GUI_BUTTON_ENABLED)
	SkillsCancelButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, SkillsCancelPress)
	SkillsCancelButton.SetText (13727)
	SkillsCancelButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)
	GemRB.SetRepeatClickFlags(GEM_RK_DISABLE, OP_NAND)

	RedrawSkills()
	SkillsWindow.SetVisible (WINDOW_VISIBLE)
	return

def RedrawSkills():
	PointsLeft = GemRB.GetVar ("SkillPointsLeft")
	if PointsLeft == 0:
		SkillsDoneButton.SetState(IE_GUI_BUTTON_ENABLED)
	else:
		SkillsDoneButton.SetState(IE_GUI_BUTTON_DISABLED)
	return

def SkillsDonePress():
	global CharGenWindow, SkillsWindow, SkillsState

	if SkillsWindow:
		SkillsWindow.Unload ()
	SkillsState = 1
	CharGenWindow.SetVisible (WINDOW_VISIBLE)
	SkillsPress()
	return

def SkillsCancelPress():
	global CharGenWindow, SkillsWindow, SkillsState

	if SkillsWindow:
		SkillsWindow.Unload ()
	SkillsState = 0
	CharGenWindow.SetVisible (WINDOW_VISIBLE)
	return

# Racial Enemy Selection

def RacialEnemySelect():
	global CharGenWindow, RacialEnemyWindow, RacialEnemyTextArea, RacialEnemyDoneButton

	CharGenWindow.SetVisible (WINDOW_INVISIBLE)
	RacialEnemyWindow = GemRB.LoadWindow (15)
	RacialEnemyCount = RacialEnemyTable.GetRowCount ()

	for i in range (2, 8):
		RacialEnemySelectButton = RacialEnemyWindow.GetControl (i)
		RacialEnemySelectButton.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_OR)

	for i in range (2, 8):
		RacialEnemySelectButton = RacialEnemyWindow.GetControl (i)
		RacialEnemySelectButton.SetState (IE_GUI_BUTTON_ENABLED)
		RacialEnemySelectButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, RacialEnemySelectPress)
		RacialEnemySelectButton.SetVarAssoc ("RacialEnemy", i - 1)

	GemRB.SetVar ("RacialEnemyIndex", 0)
	GemRB.SetVar ("HatedRace", 0)
	RacialEnemyScrollBar = RacialEnemyWindow.GetControl (1)
	RacialEnemyScrollBar.SetVarAssoc ("RacialEnemyIndex", RacialEnemyCount - 5)
	RacialEnemyScrollBar.SetEvent (IE_GUI_SCROLLBAR_ON_CHANGE, DisplayRacialEnemies)

	RacialEnemyTextArea = RacialEnemyWindow.GetControl (8)
	RacialEnemyTextArea.SetText (17256)

	RacialEnemyDoneButton = RacialEnemyWindow.GetControl (11)
	RacialEnemyDoneButton.SetState (IE_GUI_BUTTON_DISABLED)
	RacialEnemyDoneButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, RacialEnemyDonePress)
	RacialEnemyDoneButton.SetText (11973)
	RacialEnemyDoneButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	RacialEnemyCancelButton = RacialEnemyWindow.GetControl (10)
	RacialEnemyCancelButton.SetState (IE_GUI_BUTTON_ENABLED)
	RacialEnemyCancelButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, RacialEnemyCancelPress)
	RacialEnemyCancelButton.SetText (13727)
	RacialEnemyCancelButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	DisplayRacialEnemies()
	RacialEnemyWindow.SetVisible (WINDOW_VISIBLE)
	return

def DisplayRacialEnemies():
	global RacialEnemyWindow

	RacialEnemyIndex = GemRB.GetVar ("RacialEnemyIndex")
	for i in range (2, 8):
		RacialEnemySelectButton = RacialEnemyWindow.GetControl (i)
		RacialEnemySelectButton.SetText (RacialEnemyTable.GetValue (RacialEnemyIndex + i - 2, 0))
	return

def RacialEnemySelectPress():
	global RacialEnemyWindow, RacialEnemyDoneButton, RacialEnemyTextArea

	RacialEnemy = GemRB.GetVar ("RacialEnemyIndex") + GemRB.GetVar ("RacialEnemy") - 1
	RacialEnemyTextArea.SetText (RacialEnemyTable.GetValue (RacialEnemy, 2) )
	GemRB.SetVar ("HatedRace", RacialEnemyTable.GetValue (RacialEnemy, 1) )
	RacialEnemyDoneButton.SetState (IE_GUI_BUTTON_ENABLED)
	return

def RacialEnemyDonePress():
	global CharGenWindow, RacialEnemyWindow, SkillsState

	if RacialEnemyWindow:
		RacialEnemyWindow.Unload ()

	SkillsState = 1
	CharGenWindow.SetVisible (WINDOW_VISIBLE)
	SkillsPress()
	return

def RacialEnemyCancelPress():
	global CharGenWindow, RacialEnemyWindow, SkillsState

	if RacialEnemyWindow:
		RacialEnemyWindow.Unload ()
	SkillsState = 0
	CharGenWindow.SetVisible (WINDOW_VISIBLE)
	return


# Weapon Proficiencies Selection

def ProficienciesSelect():
	global CharGenWindow, ProficienciesWindow, ProficienciesTextArea
	global ProficienciesPointsLeft, ProficienciesDoneButton, ProfsMaxTable

	CharGenWindow.SetVisible (WINDOW_INVISIBLE)
	ProficienciesWindow = GemRB.LoadWindow (9)
	ProfsTable = GemRB.LoadTable ("profs")
	ProfsMaxTable = GemRB.LoadTable ("profsmax")
	ClassWeaponsTable = GemRB.LoadTable ("clasweap")

	Class = GemRB.GetPlayerStat (MyChar, IE_CLASS)
	ClassIndex = CommonTables.Classes.FindValue (5, Class)
	ClassName = CommonTables.Classes.GetRowName (ClassIndex)
	Class = ProfsTable.GetRowIndex (ClassName)
	ProficienciesPointsLeft = ProfsTable.GetValue (Class, 0)
	PointsLeftLabel = ProficienciesWindow.GetControl (0x10000009)
	PointsLeftLabel.SetUseRGB (1)
	PointsLeftLabel.SetText (str(ProficienciesPointsLeft))

	for i in range (8):
		ProficienciesLabel = ProficienciesWindow.GetControl (69 + i)
		ProficienciesLabel.SetState (IE_GUI_BUTTON_ENABLED)
		ProficienciesLabel.SetEvent (IE_GUI_BUTTON_ON_PRESS, ProficienciesLabelPress)
		ProficienciesLabel.SetVarAssoc ("ProficienciesIndex", i + 1)

		for j in range (5):
			ProficienciesMark = ProficienciesWindow.GetControl (27 + i * 5 + j)
			ProficienciesMark.SetSprites("GUIPFC", 0, 0, 0, 0, 0)
			ProficienciesMark.SetState (IE_GUI_BUTTON_DISABLED)
			ProficienciesMark.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_OR)

		Allowed = ClassWeaponsTable.GetValue (ClassName, ProficienciesTable.GetRowName (i))

		ProficienciesPlusButton = ProficienciesWindow.GetControl (11 + i * 2)
		if Allowed == 0:
			ProficienciesPlusButton.SetState (IE_GUI_BUTTON_DISABLED)
			ProficienciesPlusButton.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_OR)
		else:
			ProficienciesPlusButton.SetState (IE_GUI_BUTTON_ENABLED)
		ProficienciesPlusButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, ProficienciesPlusPress)
		ProficienciesPlusButton.SetVarAssoc ("ProficienciesIndex", i + 1)

		ProficienciesMinusButton = ProficienciesWindow.GetControl (12 + i * 2)
		if Allowed == 0:
			ProficienciesMinusButton.SetState (IE_GUI_BUTTON_DISABLED)
			ProficienciesMinusButton.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_OR)
		else:
			ProficienciesMinusButton.SetState (IE_GUI_BUTTON_ENABLED)
		ProficienciesMinusButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, ProficienciesMinusPress)
		ProficienciesMinusButton.SetVarAssoc ("ProficienciesIndex", i + 1)

	for i in range (7):
		ProficienciesLabel = ProficienciesWindow.GetControl (85 + i)
		ProficienciesLabel.SetState (IE_GUI_BUTTON_ENABLED)
		ProficienciesLabel.SetEvent (IE_GUI_BUTTON_ON_PRESS, ProficienciesLabelPress)
		ProficienciesLabel.SetVarAssoc ("ProficienciesIndex", i + 9)

		for j in range (5):
			ProficienciesMark = ProficienciesWindow.GetControl (92 + i * 5 + j)
			ProficienciesMark.SetSprites("GUIPFC", 0, 0, 0, 0, 0)
			ProficienciesMark.SetState (IE_GUI_BUTTON_DISABLED)
			ProficienciesMark.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_OR)

		Allowed = ClassWeaponsTable.GetValue (ClassName, ProficienciesTable.GetRowName (i + 8))

		ProficienciesPlusButton = ProficienciesWindow.GetControl (127 + i * 2)
		if Allowed == 0:
			ProficienciesPlusButton.SetState (IE_GUI_BUTTON_DISABLED)
			ProficienciesPlusButton.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_OR)
		else:
			ProficienciesPlusButton.SetState (IE_GUI_BUTTON_ENABLED)
		ProficienciesPlusButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, ProficienciesPlusPress)
		ProficienciesPlusButton.SetVarAssoc ("ProficienciesIndex", i + 9)

		ProficienciesMinusButton = ProficienciesWindow.GetControl (128 + i * 2)
		if Allowed == 0:
			ProficienciesMinusButton.SetState (IE_GUI_BUTTON_DISABLED)
			ProficienciesMinusButton.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_OR)
		else:
			ProficienciesMinusButton.SetState (IE_GUI_BUTTON_ENABLED)
		ProficienciesMinusButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, ProficienciesMinusPress)
		ProficienciesMinusButton.SetVarAssoc ("ProficienciesIndex", i + 9)

	for i in range (15):
		GemRB.SetVar ("Proficiency" + str(i), 0)

	GemRB.SetToken ("number", str(ProficienciesPointsLeft) )
	ProficienciesTextArea = ProficienciesWindow.GetControl (68)
	ProficienciesTextArea.SetText (9588)

	ProficienciesDoneButton = ProficienciesWindow.GetControl (0)
	ProficienciesDoneButton.SetState (IE_GUI_BUTTON_DISABLED)
	ProficienciesDoneButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, ProficienciesDonePress)
	ProficienciesDoneButton.SetText (11973)
	ProficienciesDoneButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	ProficienciesCancelButton = ProficienciesWindow.GetControl (77)
	ProficienciesCancelButton.SetState (IE_GUI_BUTTON_ENABLED)
	ProficienciesCancelButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, ProficienciesCancelPress)
	ProficienciesCancelButton.SetText (13727)
	ProficienciesCancelButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	ProficienciesWindow.SetVisible (WINDOW_VISIBLE)
	return

def ProficienciesLabelPress():
	global ProficienciesWindow, ProficienciesTextArea

	ProficienciesIndex = GemRB.GetVar ("ProficienciesIndex") - 1
	ProficienciesTextArea.SetText (ProficienciesTable.GetValue (ProficienciesIndex, 2) )
	return

def ProficienciesPlusPress():
	global ProficienciesWindow, ProficienciesTextArea
	global ProficienciesPointsLeft, ProfsMaxTable

	ProficienciesIndex = GemRB.GetVar ("ProficienciesIndex") - 1
	ProficienciesValue = GemRB.GetVar ("Proficiency" + str(ProficienciesIndex) )
	Class = GemRB.GetPlayerStat (MyChar, IE_CLASS)
	ClassIndex = CommonTables.Classes.FindValue (5, Class)
	ClassName = CommonTables.Classes.GetRowName (ClassIndex)
	Class = ProfsMaxTable.GetRowIndex (ClassName)
	if ProficienciesPointsLeft > 0 and ProficienciesValue < ProfsMaxTable.GetValue (Class, 0):
		ProficienciesPointsLeft = ProficienciesPointsLeft - 1
		PointsLeftLabel = ProficienciesWindow.GetControl (0x10000009)
		PointsLeftLabel.SetText (str(ProficienciesPointsLeft))
		if ProficienciesPointsLeft == 0:
			ProficienciesDoneButton.SetState (IE_GUI_BUTTON_ENABLED)

		ProficienciesValue = ProficienciesValue + 1
		GemRB.SetVar ("Proficiency" + str(ProficienciesIndex), ProficienciesValue)
		if ProficienciesIndex < 8:
			ControlID = 26 + ProficienciesIndex * 5 + ProficienciesValue
		else:
			ControlID = 51 + ProficienciesIndex * 5 + ProficienciesValue
		ProficienciesMark = ProficienciesWindow.GetControl (ControlID)
		ProficienciesMark.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_NAND)

	ProficienciesTextArea.SetText (ProficienciesTable.GetValue (ProficienciesIndex, 2) )
	return

def ProficienciesMinusPress():
	global ProficienciesWindow, ProficienciesTextArea, ProficienciesPointsLeft

	ProficienciesIndex = GemRB.GetVar ("ProficienciesIndex") - 1
	ProficienciesValue = GemRB.GetVar ("Proficiency" + str(ProficienciesIndex) )
	if ProficienciesValue > 0:
		ProficienciesValue = ProficienciesValue - 1
		GemRB.SetVar ("Proficiency" + str(ProficienciesIndex), ProficienciesValue)
		if ProficienciesIndex < 8:
			ControlID = 27 + ProficienciesIndex * 5 + ProficienciesValue
		else:
			ControlID = 52 + ProficienciesIndex * 5 + ProficienciesValue
		ProficienciesMark = ProficienciesWindow.GetControl (ControlID)
		ProficienciesMark.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_OR)

		ProficienciesPointsLeft = ProficienciesPointsLeft + 1
		PointsLeftLabel = ProficienciesWindow.GetControl (0x10000009)
		PointsLeftLabel.SetText (str(ProficienciesPointsLeft))
		ProficienciesDoneButton.SetState (IE_GUI_BUTTON_DISABLED)

	ProficienciesTextArea.SetText (ProficienciesTable.GetValue (ProficienciesIndex, 2) )
	return

def ProficienciesDonePress():
	global CharGenWindow, ProficienciesWindow, SkillsState

	if ProficienciesWindow:
		ProficienciesWindow.Unload ()
	SkillsState = 2
	CharGenWindow.SetVisible (WINDOW_VISIBLE)
	SkillsPress()
	return

def ProficienciesCancelPress():
	global CharGenWindow, ProficienciesWindow, SkillsState

	if ProficienciesWindow:
		ProficienciesWindow.Unload ()
	SkillsState = 0
	CharGenWindow.SetVisible (WINDOW_VISIBLE)
	return

# Spells Selection

def MageSpellsSelect(SpellTable, Level, SpellLevel):
	global CharGenWindow, MageSpellsWindow, MageSpellsTextArea, MageSpellsDoneButton, MageSpellsSelectPointsLeft, Learnable

	CharGenWindow.SetVisible (WINDOW_INVISIBLE)
	MageSpellsWindow = GemRB.LoadWindow (7)
	#kit (school), alignment, level
	k = GemRB.GetPlayerStat (MyChar, IE_KIT)
	t = GemRB.GetPlayerStat (MyChar, IE_ALIGNMENT)
	Learnable = Spellbook.GetLearnableMageSpells(k, t, SpellLevel)
	GemRB.SetVar ("MageSpellBook", 0)
	GemRB.SetVar ("SpellMask", 0)

	if len(Learnable)<1:
		MageSpellsDonePress()
		return

	if k>0:
		MageSpellsSelectPointsLeft = 3
	else:
		MageSpellsSelectPointsLeft = 2
	PointsLeftLabel = MageSpellsWindow.GetControl (0x1000001b)
	PointsLeftLabel.SetUseRGB (1)
	PointsLeftLabel.SetText (str(MageSpellsSelectPointsLeft))

	for i in range (24):
		SpellButton = MageSpellsWindow.GetControl (i + 2)
		SpellButton.SetFlags (IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_CHECKBOX, OP_OR)
		if i < len(Learnable):
			Spell = GemRB.GetSpell (Learnable[i])
			SpellButton.SetSpellIcon(Learnable[i], 1)
			SpellButton.SetState (IE_GUI_BUTTON_ENABLED)
			SpellButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, MageSpellsSelectPress)
			SpellButton.SetVarAssoc ("SpellMask", 1 << i)
			SpellButton.SetTooltip(Spell["SpellName"])
		else:
			SpellButton.SetState (IE_GUI_BUTTON_DISABLED)

	GemRB.SetToken ("number", str(MageSpellsSelectPointsLeft))
	MageSpellsTextArea = MageSpellsWindow.GetControl (27)
	MageSpellsTextArea.SetText (17250)

	MageSpellsDoneButton = MageSpellsWindow.GetControl (0)
	MageSpellsDoneButton.SetState (IE_GUI_BUTTON_DISABLED)
	MageSpellsDoneButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, MageSpellsDonePress)
	MageSpellsDoneButton.SetText (11973)
	MageSpellsDoneButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	MageSpellsCancelButton = MageSpellsWindow.GetControl (29)
	MageSpellsCancelButton.SetState (IE_GUI_BUTTON_ENABLED)
	MageSpellsCancelButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, MageSpellsCancelPress)
	MageSpellsCancelButton.SetText (13727)
	MageSpellsCancelButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	MageSpellsWindow.SetVisible (WINDOW_VISIBLE)
	return

def MageSpellsSelectPress():
	global MageSpellsWindow, MageSpellsTextArea, MageSpellsDoneButton, MageSpellsSelectPointsLeft, Learnable

	MageSpellBook = GemRB.GetVar ("MageSpellBook")
	SpellMask = GemRB.GetVar ("SpellMask")

	#getting the bit index
	Spell = abs(MageSpellBook - SpellMask)
	i = -1
	while (Spell > 0):
		i = i + 1
		Spell = Spell >> 1

	Spell = GemRB.GetSpell (Learnable[i])
	MageSpellsTextArea.SetText (Spell["SpellDesc"])

	if SpellMask < MageSpellBook:
		MageSpellsSelectPointsLeft = MageSpellsSelectPointsLeft + 1
	else:
		if MageSpellsSelectPointsLeft==0:
			SpellMask = MageSpellBook
			GemRB.SetVar ("SpellMask", SpellMask)
		else:
			MageSpellsSelectPointsLeft = MageSpellsSelectPointsLeft - 1

	if MageSpellsSelectPointsLeft == 0:
		MageSpellsDoneButton.SetState (IE_GUI_BUTTON_ENABLED)
	else:
		MageSpellsDoneButton.SetState (IE_GUI_BUTTON_DISABLED)

	for i in range (len(Learnable)):
		SpellButton = MageSpellsWindow.GetControl (i + 2)
		if ((1 << i) & SpellMask) == 0:
			SpellButton.SetState (IE_GUI_BUTTON_LOCKED)

	PointsLeftLabel = MageSpellsWindow.GetControl (0x1000001b)
	PointsLeftLabel.SetText (str(MageSpellsSelectPointsLeft))
	GemRB.SetVar ("MageSpellBook", SpellMask)
	return

def MageSpellsDonePress():
	global CharGenWindow, MageSpellsWindow, SkillsState

	if MageSpellsWindow:
		MageSpellsWindow.Unload ()
	SkillsState = 3
	CharGenWindow.SetVisible (WINDOW_VISIBLE)
	SkillsPress()
	return

def MageSpellsCancelPress():
	global CharGenWindow, MageSpellsWindow, SkillsState

	if MageSpellsWindow:
		MageSpellsWindow.Unload ()
	SkillsState = 0
	CharGenWindow.SetVisible (WINDOW_VISIBLE)
	return


# Mage Spells Memorize

def MageSpellsMemorize(SpellTable, Level, SpellLevel):
	global CharGenWindow, MageMemorizeWindow, MageMemorizeTextArea, MageMemorizeDoneButton, MageMemorizePointsLeft

	CharGenWindow.SetVisible (WINDOW_INVISIBLE)
	MageMemorizeWindow = GemRB.LoadWindow (16)
	MaxSpellsMageTable = GemRB.LoadTable (SpellTable)
	MageSpellBook = GemRB.GetVar ("MageSpellBook")
	GemRB.SetVar ("MageMemorized", 0)
	GemRB.SetVar ("SpellMask", 0)

	MageMemorizePointsLeft = MaxSpellsMageTable.GetValue (str(Level), str(SpellLevel) )
	if MageMemorizePointsLeft<1 or len(Learnable)<1:
		MageMemorizeDonePress()
		return

	PointsLeftLabel = MageMemorizeWindow.GetControl (0x1000001b)
	PointsLeftLabel.SetUseRGB (1)
	PointsLeftLabel.SetText (str(MageMemorizePointsLeft))

	j = 0
	for i in range (12):
		SpellButton = MageMemorizeWindow.GetControl (i + 2)
		SpellButton.SetFlags (IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_CHECKBOX, OP_OR)
		while (j < len(Learnable)) and (((1 << j) & MageSpellBook) == 0):
			j = j + 1
		if j < len(Learnable):
			Spell = GemRB.GetSpell (Learnable[j])
			SpellButton.SetTooltip(Spell["SpellName"])
			SpellButton.SetSpellIcon(Learnable[j], 1)
			SpellButton.SetState (IE_GUI_BUTTON_ENABLED)
			SpellButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, MageMemorizeSelectPress)
			SpellButton.SetVarAssoc ("SpellMask", 1 << j)
			j = j + 1
		else:
			SpellButton.SetState (IE_GUI_BUTTON_DISABLED)

	GemRB.SetToken ("number", str(MageMemorizePointsLeft))
	MageMemorizeTextArea = MageMemorizeWindow.GetControl (27)
	MageMemorizeTextArea.SetText (17253)

	MageMemorizeDoneButton = MageMemorizeWindow.GetControl (0)
	MageMemorizeDoneButton.SetState (IE_GUI_BUTTON_DISABLED)
	MageMemorizeDoneButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, MageMemorizeDonePress)
	MageMemorizeDoneButton.SetText (11973)
	MageMemorizeDoneButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	MageMemorizeCancelButton = MageMemorizeWindow.GetControl (29)
	MageMemorizeCancelButton.SetState (IE_GUI_BUTTON_ENABLED)
	MageMemorizeCancelButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, MageMemorizeCancelPress)
	MageMemorizeCancelButton.SetText (13727)
	MageMemorizeCancelButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	MageMemorizeWindow.SetVisible (WINDOW_VISIBLE)
	return

def MageMemorizeSelectPress():
	global MageMemorizeWindow, MageMemorizeTextArea, MageMemorizeDoneButton, MageMemorizePointsLeft, Learnable

	MageSpellBook = GemRB.GetVar ("MageSpellBook")
	MageMemorized = GemRB.GetVar ("MageMemorized")
	SpellMask = GemRB.GetVar ("SpellMask")

	Spell = abs(MageMemorized - SpellMask)
	i = -1
	while (Spell > 0):
		i = i + 1
		Spell = Spell >> 1

	Spell = GemRB.GetSpell (Learnable[i])
	MageMemorizeTextArea.SetText (Spell["SpellDesc"])

	if SpellMask < MageMemorized:
		MageMemorizePointsLeft = MageMemorizePointsLeft + 1
		j = 0
		for i in range (12):
			SpellButton = MageMemorizeWindow.GetControl (i + 2)
			while (j < len(Learnable) ) and (((1 << j) & MageSpellBook) == 0):
				j = j + 1
			if j < len(Learnable):
				SpellButton.SetState (IE_GUI_BUTTON_ENABLED)
				j = j + 1
		MageMemorizeDoneButton.SetState (IE_GUI_BUTTON_DISABLED)
	else:
		MageMemorizePointsLeft = MageMemorizePointsLeft - 1
		if MageMemorizePointsLeft == 0:
			j = 0
			for i in range (12):
				SpellButton = MageMemorizeWindow.GetControl (i + 2)
				while (j < len(Learnable) ) and (((1 << j) & MageSpellBook) == 0):
					j = j + 1
				if j < len(Learnable):
					if ((1 << j) & SpellMask) == 0:
						SpellButton.SetState (IE_GUI_BUTTON_DISABLED)
					j = j + 1
			MageMemorizeDoneButton.SetState (IE_GUI_BUTTON_ENABLED)

	PointsLeftLabel = MageMemorizeWindow.GetControl (0x1000001b)
	PointsLeftLabel.SetText (str(MageMemorizePointsLeft))
	GemRB.SetVar ("MageMemorized", SpellMask)
	return

def MageMemorizeDonePress():
	global CharGenWindow, MageMemorizeWindow, SkillsState

	if MageMemorizeWindow:
		MageMemorizeWindow.Unload ()
	SkillsState = 4
	CharGenWindow.SetVisible (WINDOW_VISIBLE)
	SkillsPress()
	return

def MageMemorizeCancelPress():
	global CharGenWindow, MageMemorizeWindow, SkillsState

	if MageMemorizeWindow:
		MageMemorizeWindow.Unload ()
	SkillsState = 0
	CharGenWindow.SetVisible (WINDOW_VISIBLE)
	return

# Priest Spells Memorize

def PriestSpellsMemorize(SpellTable, Level, SpellLevel):
	global CharGenWindow, PriestMemorizeWindow, Learnable, ClassFlag
	global PriestMemorizeTextArea, PriestMemorizeDoneButton, PriestMemorizePointsLeft

	CharGenWindow.SetVisible (WINDOW_INVISIBLE)
	PriestMemorizeWindow = GemRB.LoadWindow (17)
	t = AlignmentTable.GetValue ( GemRB.GetVar ("Alignment")-1, 3)
	Learnable = Spellbook.GetLearnablePriestSpells( ClassFlag, t, SpellLevel)

	MaxSpellsPriestTable = GemRB.LoadTable (SpellTable)
	GemRB.SetVar ("PriestMemorized", 0)
	GemRB.SetVar ("SpellMask", 0)

	PriestMemorizePointsLeft = MaxSpellsPriestTable.GetValue (str(Level), str(SpellLevel) )
	if PriestMemorizePointsLeft<1 or len(Learnable)<1:
		PriestMemorizeDonePress()
		return

	PointsLeftLabel = PriestMemorizeWindow.GetControl (0x1000001b)
	PointsLeftLabel.SetUseRGB (1)
	PointsLeftLabel.SetText (str(PriestMemorizePointsLeft))

	for i in range (12):
		SpellButton = PriestMemorizeWindow.GetControl (i + 2)
		SpellButton.SetFlags (IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_CHECKBOX, OP_OR)
		if i < len(Learnable):
			Spell = GemRB.GetSpell (Learnable[i])
			SpellButton.SetTooltip(Spell["SpellName"])
			SpellButton.SetSpellIcon(Learnable[i], 1)
			SpellButton.SetState (IE_GUI_BUTTON_ENABLED)
			SpellButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, PriestMemorizeSelectPress)
			SpellButton.SetVarAssoc ("SpellMask", 1 << i)
		else:
			SpellButton.SetState (IE_GUI_BUTTON_DISABLED)

	GemRB.SetToken ("number", str(PriestMemorizePointsLeft))
	PriestMemorizeTextArea = PriestMemorizeWindow.GetControl (27)
	PriestMemorizeTextArea.SetText (17253)

	PriestMemorizeDoneButton = PriestMemorizeWindow.GetControl (0)
	PriestMemorizeDoneButton.SetState (IE_GUI_BUTTON_DISABLED)
	PriestMemorizeDoneButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, PriestMemorizeDonePress)
	PriestMemorizeDoneButton.SetText (11973)
	PriestMemorizeDoneButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	PriestMemorizeCancelButton = PriestMemorizeWindow.GetControl (29)
	PriestMemorizeCancelButton.SetState (IE_GUI_BUTTON_ENABLED)
	PriestMemorizeCancelButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, PriestMemorizeCancelPress)
	PriestMemorizeCancelButton.SetText (13727)
	PriestMemorizeCancelButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	PriestMemorizeWindow.SetVisible (WINDOW_VISIBLE)
	return

def PriestMemorizeSelectPress():
	global PriestMemorizeWindow, Learnable, PriestMemorizeTextArea, PriestMemorizeDoneButton, PriestMemorizePointsLeft

	PriestMemorized = GemRB.GetVar ("PriestMemorized")
	SpellMask = GemRB.GetVar ("SpellMask")
	Spell = abs(PriestMemorized - SpellMask)

	i = -1
	while (Spell > 0):
		i = i + 1
		Spell = Spell >> 1

	Spell=GemRB.GetSpell (Learnable[i])
	PriestMemorizeTextArea.SetText (Spell["SpellDesc"])

	if SpellMask < PriestMemorized:
		PriestMemorizePointsLeft = PriestMemorizePointsLeft + 1
		for i in range (len(Learnable)):
			SpellButton = PriestMemorizeWindow.GetControl (i + 2)
			if (((1 << i) & SpellMask) == 0):
				SpellButton.SetState (IE_GUI_BUTTON_ENABLED)
		PriestMemorizeDoneButton.SetState (IE_GUI_BUTTON_DISABLED)
	else:
		PriestMemorizePointsLeft = PriestMemorizePointsLeft - 1
		if PriestMemorizePointsLeft == 0:
			for i in range (len(Learnable)):
				SpellButton = PriestMemorizeWindow.GetControl (i + 2)
				if ((1 << i) & SpellMask) == 0:
					SpellButton.SetState (IE_GUI_BUTTON_DISABLED)
			PriestMemorizeDoneButton.SetState (IE_GUI_BUTTON_ENABLED)

	PointsLeftLabel = PriestMemorizeWindow.GetControl (0x1000001b)
	PointsLeftLabel.SetText (str(PriestMemorizePointsLeft))
	GemRB.SetVar ("PriestMemorized", SpellMask)
	return

def PriestMemorizeDonePress():
	global CharGenWindow, PriestMemorizeWindow, SkillsState

	if PriestMemorizeWindow:
		PriestMemorizeWindow.Unload ()
	SkillsState = 5
	CharGenWindow.SetVisible (WINDOW_VISIBLE)
	SkillsPress()
	return

def PriestMemorizeCancelPress():
	global CharGenWindow, PriestMemorizeWindow, SkillsState

	if PriestMemorizeWindow:
		PriestMemorizeWindow.Unload ()
	SkillsState = 0
	CharGenWindow.SetVisible (WINDOW_VISIBLE)
	return

# Appearance Selection

def AppearancePress():
	global CharGenWindow, AppearanceWindow, AppearanceTable
	global Portrait, AppearanceAvatarButton, PortraitName
	global AppearanceHairButton, AppearanceSkinButton
	global AppearanceMajorButton, AppearanceMinorButton
	global HairColor, SkinColor, MajorColor, MinorColor

	CharGenWindow.SetVisible (WINDOW_INVISIBLE)
	AppearanceWindow = GemRB.LoadWindow (13)
	AppearanceTable = GemRB.LoadTable ("PORTCOLR")

	if Portrait<0:
		PortraitIndex = 0
	else:
		PortraitName = PortraitsTable.GetRowName (Portrait)
		PortraitIndex = AppearanceTable.GetRowIndex (PortraitName + "L")

	HairColor = AppearanceTable.GetValue (PortraitIndex, 1)
	GemRB.SetVar ("HairColor", HairColor)
	SkinColor = AppearanceTable.GetValue (PortraitIndex, 0)
	GemRB.SetVar ("SkinColor", SkinColor)
	MajorColor = AppearanceTable.GetValue (PortraitIndex, 2)
	GemRB.SetVar ("MajorColor", MajorColor)
	MinorColor = AppearanceTable.GetValue (PortraitIndex, 3)
	GemRB.SetVar ("MinorColor", MinorColor)

	AppearanceAvatarButton = AppearanceWindow.GetControl (1)
	AppearanceAvatarButton.SetState (IE_GUI_BUTTON_LOCKED)
	AppearanceAvatarButton.SetFlags (IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_ANIMATED, OP_OR)
	DrawAvatar()

	AppearanceHairButton = AppearanceWindow.GetControl (2)
	AppearanceHairButton.SetFlags (IE_GUI_BUTTON_PICTURE, OP_OR)
	AppearanceHairButton.SetState (IE_GUI_BUTTON_ENABLED)
	AppearanceHairButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, AppearanceHairPress)
	AppearanceHairButton.SetBAM ("COLGRAD", 0, 0, HairColor)

	AppearanceSkinButton = AppearanceWindow.GetControl (3)
	AppearanceSkinButton.SetFlags (IE_GUI_BUTTON_PICTURE, OP_OR)
	AppearanceSkinButton.SetState (IE_GUI_BUTTON_ENABLED)
	AppearanceSkinButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, AppearanceSkinPress)
	AppearanceSkinButton.SetBAM ("COLGRAD", 0, 0, SkinColor)

	AppearanceMajorButton = AppearanceWindow.GetControl (4)
	AppearanceMajorButton.SetFlags (IE_GUI_BUTTON_PICTURE, OP_OR)
	AppearanceMajorButton.SetState (IE_GUI_BUTTON_ENABLED)
	AppearanceMajorButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, AppearanceMajorPress)
	AppearanceMajorButton.SetBAM ("COLGRAD", 0, 0, MajorColor)

	AppearanceMinorButton = AppearanceWindow.GetControl (5)
	AppearanceMinorButton.SetFlags (IE_GUI_BUTTON_PICTURE, OP_OR)
	AppearanceMinorButton.SetState (IE_GUI_BUTTON_ENABLED)
	AppearanceMinorButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, AppearanceMinorPress)
	AppearanceMinorButton.SetBAM ("COLGRAD", 0, 0, MinorColor)

	AppearanceDoneButton = AppearanceWindow.GetControl (0)
	AppearanceDoneButton.SetState (IE_GUI_BUTTON_ENABLED)
	AppearanceDoneButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, AppearanceDonePress)
	AppearanceDoneButton.SetText (11973)
	AppearanceDoneButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	AppearanceCancelButton = AppearanceWindow.GetControl (13)
	AppearanceCancelButton.SetState (IE_GUI_BUTTON_ENABLED)
	AppearanceCancelButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, AppearanceCancelPress)
	AppearanceCancelButton.SetText (13727)
	AppearanceCancelButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	AppearanceWindow.SetVisible (WINDOW_VISIBLE)
	return

def DrawAvatar():
	global AppearanceAvatarButton
	global MyChar

	AvatarID = 0x6000
	table = GemRB.LoadTable ("avprefr")
	lookup = CommonTables.Races.FindValue (3, GemRB.GetPlayerStat(MyChar, IE_RACE))
	lookup = CommonTables.Races.GetRowName (lookup)
	AvatarID = AvatarID+table.GetValue (lookup, "RACE")
	table = GemRB.LoadTable ("avprefc")
	lookup = CommonTables.Classes.FindValue (5, GemRB.GetPlayerStat(MyChar, IE_CLASS))
	lookup = CommonTables.Classes.GetRowName (lookup)
	AvatarID = AvatarID+table.GetValue (lookup, "PREFIX")
	table = GemRB.LoadTable ("avprefg")
	AvatarID = AvatarID+table.GetValue (GemRB.GetPlayerStat(MyChar,IE_SEX),0)

	AvatarRef = CommonTables.Pdolls.GetValue (hex(AvatarID), "LEVEL1")
	AppearanceAvatarButton.SetPLT(AvatarRef, 0, MinorColor, MajorColor, SkinColor, 0, 0, HairColor, 0)

	return

def AppearanceHairPress():
	GemRB.SetVar ("ColorType", 0)
	AppearanceColorChoice (GemRB.GetVar ("HairColor"))
	return

def AppearanceSkinPress():
	GemRB.SetVar ("ColorType", 1)
	AppearanceColorChoice (GemRB.GetVar ("SkinColor"))
	return

def AppearanceMajorPress():
	GemRB.SetVar ("ColorType", 2)
	AppearanceColorChoice (GemRB.GetVar ("MajorColor"))
	return

def AppearanceMinorPress():
	GemRB.SetVar ("ColorType", 3)
	AppearanceColorChoice (GemRB.GetVar ("MinorColor"))
	return

def AppearanceColorChoice (CurrentColor):
	global AppearanceWindow, AppearanceColorWindow

	AppearanceWindow.SetVisible (WINDOW_INVISIBLE)
	AppearanceColorWindow = GemRB.LoadWindow (14)
	AppearanceColorTable = GemRB.LoadTable ("clowncol")
	ColorType = GemRB.GetVar ("ColorType")
	GemRB.SetVar ("SelectedColor", CurrentColor)

	for i in range (34):
		ColorButton = AppearanceColorWindow.GetControl (i)
		ColorButton.SetState (IE_GUI_BUTTON_ENABLED)
		ColorButton.SetFlags (IE_GUI_BUTTON_PICTURE, OP_OR)

	for i in range (34):
		Color = AppearanceColorTable.GetValue (ColorType, i)
		if Color != "*":
			ColorButton = AppearanceColorWindow.GetControl (i)
			ColorButton.SetBAM ("COLGRAD", 2, 0, Color)
			ColorButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, AppearanceColorSelected)
			ColorButton.SetVarAssoc ("SelectedColor", Color)

	AppearanceColorWindow.SetVisible (WINDOW_VISIBLE)
	return

def AppearanceColorSelected():
	global HairColor, SkinColor, MajorColor, MinorColor
	global AppearanceWindow, AppearanceColorWindow
	global AppearanceHairButton, AppearanceSkinButton
	global AppearanceMajorButton, AppearanceMinorButton

	if AppearanceColorWindow:
		AppearanceColorWindow.Unload ()
	ColorType = GemRB.GetVar ("ColorType")
	if ColorType == 0:
		HairColor = GemRB.GetVar ("SelectedColor")
		GemRB.SetVar ("HairColor", HairColor)
		AppearanceHairButton.SetBAM ("COLGRAD", 0, 0, HairColor)
	elif ColorType == 1:
		SkinColor = GemRB.GetVar ("SelectedColor")
		GemRB.SetVar ("SkinColor", SkinColor)
		AppearanceSkinButton.SetBAM ("COLGRAD", 0, 0, SkinColor)
	elif ColorType == 2:
		MajorColor = GemRB.GetVar ("SelectedColor")
		GemRB.SetVar ("MajorColor", MajorColor)
		AppearanceMajorButton.SetBAM ("COLGRAD", 0, 0, MajorColor)
	elif ColorType == 3:
		MinorColor = GemRB.GetVar ("SelectedColor")
		GemRB.SetVar ("MinorColor", MinorColor)
		AppearanceMinorButton.SetBAM ("COLGRAD", 0, 0, MinorColor)
	DrawAvatar()
	AppearanceWindow.SetVisible (WINDOW_VISIBLE)
	return

def AppearanceDonePress():
	global CharGenWindow, AppearanceWindow

	if AppearanceWindow:
		AppearanceWindow.Unload ()
	CharGenWindow.SetVisible (WINDOW_VISIBLE)
	CharSoundSelect()
	return

def AppearanceCancelPress():
	global CharGenWindow, AppearanceWindow

	if AppearanceWindow:
		AppearanceWindow.Unload ()
	CharGenWindow.SetVisible (WINDOW_VISIBLE)
	return

def CharSoundSelect():
	global CharGenWindow, CharSoundWindow, CharSoundTable, CharSoundStrings
	global CharSoundVoiceList, VerbalConstants

	CharGenWindow.SetVisible (WINDOW_INVISIBLE)
	CharSoundWindow = GemRB.LoadWindow (19)
	CharSoundTable = GemRB.LoadTable ("CHARSND")
	CharSoundStrings = GemRB.LoadTable ("CHARSTR")

	VerbalConstants =  [CharSoundTable.GetRowName(i) for i in range(CharSoundTable.GetRowCount())]
	CharSoundVoiceList = CharSoundWindow.GetControl (45)
	CharSoundVoiceList.SetFlags (IE_GUI_TEXTAREA_SELECTABLE)
	RowCount=CharSoundVoiceList.GetCharSounds()

	CharSoundPlayButton = CharSoundWindow.GetControl (47)
	CharSoundPlayButton.SetState (IE_GUI_BUTTON_ENABLED)
	CharSoundPlayButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, CharSoundPlayPress)
	CharSoundPlayButton.SetText (17318)

	CharSoundTextArea = CharSoundWindow.GetControl (50)
	CharSoundTextArea.SetText (11315)

	CharSoundDoneButton = CharSoundWindow.GetControl (0)
	CharSoundDoneButton.SetState (IE_GUI_BUTTON_ENABLED)
	CharSoundDoneButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, CharSoundDonePress)
	CharSoundDoneButton.SetText (11973)
	CharSoundDoneButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	CharSoundCancelButton = CharSoundWindow.GetControl (10)
	CharSoundCancelButton.SetState (IE_GUI_BUTTON_ENABLED)
	CharSoundCancelButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, CharSoundCancelPress)
	CharSoundCancelButton.SetText (13727)
	CharSoundCancelButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	CharSoundWindow.SetVisible (WINDOW_VISIBLE)
	return

def CharSoundPlayPress():
	global CharGenWindow, CharSoundWindow, CharSoundTable, CharSoundStrings
	global CharSoundVoiceList, SoundIndex, VerbalConstants

	row = CharSoundVoiceList.QueryText ()

	GemRB.SetPlayerSound (MyChar, row)

	#play sound as sound slot
	GemRB.VerbalConstant (MyChar, int(VerbalConstants[SoundIndex]))

	SoundIndex += 1
	if SoundIndex >= len(VerbalConstants):
		SoundIndex = 0
	return

def CharSoundDonePress():
	global CharGenWindow, CharSoundWindow, AppearanceButton, BiographyButton, NameButton, CharGenState

	if CharSoundWindow:
		CharSoundWindow.Unload ()
	AppearanceButton.SetState (IE_GUI_BUTTON_DISABLED)
	AppearanceButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_NAND)
	BiographyButton.SetState (IE_GUI_BUTTON_ENABLED)
	NameButton.SetState (IE_GUI_BUTTON_ENABLED)
	NameButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)
	CharGenState = 7
	SetCharacterDescription()
	CharGenWindow.SetVisible (WINDOW_VISIBLE)
	return

def CharSoundCancelPress():
	global CharGenWindow, CharSoundWindow

	if CharSoundWindow:
		CharSoundWindow.Unload ()
	CharGenWindow.SetVisible (WINDOW_VISIBLE)
	return

# Biography Selection

def BiographyPress():
	global CharGenWindow, BiographyWindow, BiographyField

	CharGenWindow.SetVisible (WINDOW_INVISIBLE)
	BiographyWindow = GemRB.LoadWindow (51)

	BiographyField = BiographyWindow.GetControl (4)
	BiographyField.SetText (19423)
	BiographyField.SetBackground("")

	BiographyClearButton = BiographyWindow.GetControl (5)
	BiographyClearButton.SetState (IE_GUI_BUTTON_ENABLED)
	BiographyClearButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, BiographyClearPress)
	BiographyClearButton.SetText (18622)

	BiographyCancelButton = BiographyWindow.GetControl (2)
	BiographyCancelButton.SetState (IE_GUI_BUTTON_ENABLED)
	BiographyCancelButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, BiographyCancelPress)
	BiographyCancelButton.SetText (13727)
	BiographyCancelButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	BiographyDoneButton = BiographyWindow.GetControl (1)
	BiographyDoneButton.SetState (IE_GUI_BUTTON_ENABLED)
	BiographyDoneButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, BiographyDonePress)
	BiographyDoneButton.SetText (11973)
	BiographyDoneButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	BiographyWindow.SetVisible (WINDOW_VISIBLE)
	return

def BiographyClearPress():
	global BiographyWindow, BiographyField

	BiographyField.SetText ("")
	return

def BiographyCancelPress():
	global CharGenWindow, BiographyWindow

	if BiographyWindow:
		BiographyWindow.Unload ()
	CharGenWindow.SetVisible (WINDOW_VISIBLE)
	return

def BiographyDonePress():
	global CharGenWindow, BiographyWindow, BiographyField

	GemRB.SetToken ("Biography", BiographyField.QueryText () )
	if BiographyWindow:
		BiographyWindow.Unload ()
	CharGenWindow.SetVisible (WINDOW_VISIBLE)
	return

# Name Selection

def NamePress():
	global CharGenWindow, NameWindow, NameDoneButton, NameField

	CharGenWindow.SetVisible (WINDOW_INVISIBLE)
	NameWindow = GemRB.LoadWindow (5)

	NameDoneButton = NameWindow.GetControl (0)
	NameDoneButton.SetState (IE_GUI_BUTTON_DISABLED)
	NameDoneButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, NameDonePress)
	NameDoneButton.SetText (11973)
	NameDoneButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	NameCancelButton = NameWindow.GetControl (3)
	NameCancelButton.SetState (IE_GUI_BUTTON_ENABLED)
	NameCancelButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, NameCancelPress)
	NameCancelButton.SetText (13727)
	NameCancelButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	NameField = NameWindow.GetControl (2)
	NameField.SetEvent (IE_GUI_EDIT_ON_CHANGE, NameEditChange)
	NameField.SetText (GemRB.GetToken ("CHARNAME") )
	NameField.SetStatus (IE_GUI_CONTROL_FOCUSED)

	NameWindow.SetVisible (WINDOW_VISIBLE)
	NameEditChange()
	return

def NameEditChange():
	global NameField

	if NameField.QueryText () == "":
		NameDoneButton.SetState (IE_GUI_BUTTON_DISABLED)
	else:
		NameDoneButton.SetState (IE_GUI_BUTTON_ENABLED)
	return

def NameDonePress():
	global CharGenWindow, CharGenState, NameWindow, NameField, AcceptButton

	GemRB.SetToken ("CHARNAME", NameField.QueryText () )
	if NameWindow:
		NameWindow.Unload ()
	CharGenState = 8
	AcceptButton.SetState (IE_GUI_BUTTON_ENABLED)
	AcceptButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)
	SetCharacterDescription()
	CharGenWindow.SetVisible (WINDOW_VISIBLE)
	return

def NameCancelPress():
	global CharGenWindow, NameWindow

	GemRB.SetToken ("CHARNAME", "")
	if NameWindow:
		NameWindow.Unload ()
	CharGenWindow.SetVisible (WINDOW_VISIBLE)
	return

# Import Character

def ImportPress():
	global CharGenWindow, ImportWindow
	global CharImportList

	CharGenWindow.SetVisible (WINDOW_INVISIBLE)
	ImportWindow = GemRB.LoadWindow (20)

	TextAreaControl = ImportWindow.GetControl(4)
	TextAreaControl.SetText(10963)

	GemRB.SetVar ("Selected", 0)
	CharImportList = ImportWindow.GetControl(2)
	CharImportList.SetFlags (IE_GUI_TEXTAREA_SELECTABLE)
	CharImportList.SetVarAssoc ("Selected",0)
	CharImportList.GetCharacters()

	ImportDoneButton = ImportWindow.GetControl (0)
	ImportDoneButton.SetState (IE_GUI_BUTTON_ENABLED)
	ImportDoneButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, ImportDonePress)
	ImportDoneButton.SetText (11973)
	ImportDoneButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	ImportCancelButton = ImportWindow.GetControl (1)
	ImportCancelButton.SetState (IE_GUI_BUTTON_ENABLED)
	ImportCancelButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, ImportCancelPress)
	ImportCancelButton.SetText (13727)
	ImportCancelButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	ImportWindow.SetVisible (WINDOW_VISIBLE)
	return

def ImportDonePress():
	global CharGenWindow, ImportWindow, CharImportList
	global CharGenState, SkillsState, Portrait, ImportedChar

	# Import the character from the chosen name
	GemRB.CreatePlayer (CharImportList.QueryText(), MyChar|0x8000, 1)

	GemRB.SetToken ("CHARNAME", GemRB.GetPlayerName (MyChar) )
	GemRB.SetToken ("SmallPortrait", GemRB.GetPlayerPortrait (MyChar, 1) )
	PortraitName = GemRB.GetPlayerPortrait (MyChar, 0)
	GemRB.SetToken ("LargePortrait", PortraitName )
	PortraitButton.SetPicture (PortraitName)
	Portrait = -1

	ImportedChar = 1
	CharGenState = 7
	SkillsState = 5
	SetCharacterDescription ()
	GenderButton.SetState (IE_GUI_BUTTON_DISABLED)
	GenderButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_NAND)
	RaceButton.SetState (IE_GUI_BUTTON_DISABLED)
	RaceButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_NAND)
	ClassButton.SetState (IE_GUI_BUTTON_DISABLED)
	ClassButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_NAND)
	AlignmentButton.SetState (IE_GUI_BUTTON_DISABLED)
	AlignmentButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_NAND)
	AbilitiesButton.SetState (IE_GUI_BUTTON_DISABLED)
	AbilitiesButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_NAND)
	SkillsButton.SetState (IE_GUI_BUTTON_DISABLED)
	SkillsButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_NAND)
	AppearanceButton.SetState (IE_GUI_BUTTON_ENABLED)
	AppearanceButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)
	BiographyButton.SetState (IE_GUI_BUTTON_ENABLED)
	BiographyButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)
	NameButton.SetState (IE_GUI_BUTTON_ENABLED)
	NameButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)
	CharGenWindow.SetVisible (WINDOW_VISIBLE)
	if ImportWindow:
		ImportWindow.Unload ()
	return

def ImportCancelPress():
	global CharGenWindow, ImportWindow

	if ImportWindow:
		ImportWindow.Unload ()
	CharGenWindow.SetVisible (WINDOW_VISIBLE)
	return
