# SPDX-FileCopyrightText: 2009 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

import BGCommon
import GemRB
from GUIDefines import *
from ie_stats import *
import CharGenCommon
import GUICommon
import Spellbook
import CommonTables
import LUCommon
import LUProfsSelection
import PaperDoll

def Imprt():
	GemRB.SetToken("NextScript","CharGen")
	GemRB.SetNextScript("ImportFile") #import
	return

def setPlayer():
	MyChar = GemRB.GetVar ("Slot")
	GemRB.CreatePlayer ("charbase", MyChar | 0x8000 )
	GemRB.SetVar ("ImportedChar", 0)
	return False

def unsetPlayer():
	MyChar = GemRB.GetVar ("Slot")
	GemRB.CreatePlayer ("", MyChar | 0x8000 )
	return False

def unsetGender():
	MyChar = GemRB.GetVar ("Slot")
	GemRB.SetPlayerStat (MyChar, IE_SEX, 0)
	GemRB.SetVar ("ImportedChar", 0)

def unsetPortrait():
	GemRB.SetToken("SmallPortrait","")
	GemRB.SetToken("LargePortrait","")

def getGender(area):
	MyChar = GemRB.GetVar ("Slot")
	area.SetText(12135)
	area.Append (": ")
	if GemRB.GetPlayerStat(MyChar,IE_SEX) == 1:
		return area.Append(1050)
	else:
		return area.Append(1051)

#race
def unsetRace():
	MyChar = GemRB.GetVar ("Slot")
	GemRB.SetPlayerStat (MyChar, IE_RACE, 0)

def getRace(area):
	MyChar = GemRB.GetVar ("Slot")
	RaceName = GUICommon.GetRaceRowName (MyChar)
	RaceCap = CommonTables.Races.GetValue (RaceName, "UPPERCASE", GTV_INT)
	area.Append("\n")
	area.Append(1048)
	area.Append(": ")
	area.Append(RaceCap)

#class
def unsetClass():
	MyChar = GemRB.GetVar ("Slot")
	GemRB.SetPlayerStat (MyChar, IE_CLASS, 0)
	GemRB.SetPlayerStat (MyChar, IE_KIT, 0)
	GemRB.SetVar ("MAGESCHOOL", 0)

def getClass(area):
	MyChar = GemRB.GetVar ("Slot")
	ClassTitle = GUICommon.GetActorClassTitle(MyChar)

	area.Append("\n")
	area.Append(12136)
	area.Append(": ")
	area.Append(ClassTitle)
	
def guardSpecialist():
	return GemRB.GetVar("Specialist") == 1

def guardMultiClass():
	return GemRB.GetVar("Multi Class") == 1


#Alignment
def unsetAlignment():
	MyChar = GemRB.GetVar ("Slot")
	GemRB.SetPlayerStat (MyChar, IE_ALIGNMENT,0)
	
def getAlignment(area):
	MyChar = GemRB.GetVar ("Slot")
	AlignID = GemRB.GetPlayerStat (MyChar, IE_ALIGNMENT)
	
	area.Append("\n")
	area.Append(1049)
	area.Append(": ")
	AlignIndex = CommonTables.Aligns.FindValue (3, AlignID)
	AlignCap = CommonTables.Aligns.GetValue (AlignIndex, 2)
	area.Append(AlignCap)
	area.Append("\n")

# Abilities
def unsetAbilities():
	MyChar = GemRB.GetVar ("Slot")
	AbilityCount = CommonTables.Ability.GetRowCount ()
	
	# set all our abilites to zero
	GemRB.SetPlayerStat (MyChar, IE_STREXTRA, 0)
	for i in range(AbilityCount):
		StatID = CommonTables.Ability.GetValue (i, 3, GTV_INT)
		GemRB.SetPlayerStat (MyChar, StatID, 0)

def getAbilities(area):
	MyChar = GemRB.GetVar ("Slot")
	AbilityCount = CommonTables.Ability.GetRowCount ()
	area.Append("\n")
	for i in range(AbilityCount):
		v = AbilityTable.GetValue (i, 2, GTV_REF)
		stat = AbilityTable.GetValue (i, 3, GTV_INT)
		area.Append(v)
		area.Append (": " + str(GemRB.GetPlayerStat (MyChar, stat)) + "\n")
	area.Append("\n")

#Skill
def unsetHateRace():
	MyChar = GemRB.GetVar ("Slot")
	GemRB.SetPlayerStat(MyChar, IE_HATEDRACE, 0 )

def guardHateRace():
	MyChar = GemRB.GetVar ("Slot")
	ClassName = GUICommon.GetClassRowName (MyChar)
	TableName = CommonTables.ClassSkills.GetValue(ClassName, "HATERACE")
	return TableName != "*"

def getHatedRace(TextAreaControl):
	MyChar = GemRB.GetVar ("Slot")
	Race = GemRB.GetPlayerStat(MyChar, IE_HATEDRACE)
	if Race:
		HateRaceTable = GemRB.LoadTable ("HATERACE")
		Row = HateRaceTable.FindValue (1, Race)
		info = HateRaceTable.GetValue (Row, 0, GTV_REF)
		if info != "":
			info = ": " + info + "\n"
			TextAreaControl.Append(15982)
			TextAreaControl.Append(info)

def unsetMageSpells():
	print("unsetMageSpells")
	MyChar = GemRB.GetVar ("Slot")

	Spellbook.RemoveKnownSpells (MyChar, IE_SPELL_TYPE_WIZARD, 1, 5, 1)

def guardMageSpells():
	MyChar = GemRB.GetVar ("Slot")
	ClassName = GUICommon.GetClassRowName (MyChar)
	TableName = CommonTables.ClassSkills.GetValue(ClassName, "MAGESPELL")
	return TableName != "*"

def getMageSpells(TextAreaControl):
	MyChar = GemRB.GetVar ("Slot")
	# arcane spells
	info = ""
	for level in range(0, 9):
		for j in range(0, GemRB.GetKnownSpellsCount (MyChar, IE_SPELL_TYPE_WIZARD, level) ):
			Spell = GemRB.GetKnownSpell (MyChar, IE_SPELL_TYPE_WIZARD, level, j)
			Spell = GemRB.GetSpell (Spell['SpellResRef'], 1)['SpellName']
			info += GemRB.GetString (Spell) + "\n"
	if info != "":
		info = "\n" + info + ""
		TextAreaControl.Append (11027)
		TextAreaControl.Append (info)

def guardSkills():
	SkillTable = GemRB.LoadTable ("thiefscl")
	RowCount = SkillTable.GetRowCount()

	MyChar = GemRB.GetVar ("Slot")
	KitName = GUICommon.GetKitRowName (MyChar, not GUICommon.IsMultiClassed (MyChar, 0))
	ClassName = GUICommon.GetClassRowName (MyChar)
	for i in range(RowCount):
		SkillName = SkillTable.GetRowName(i)
		if SkillTable.GetValue(SkillName, KitName) != 0 or SkillTable.GetValue(SkillName, ClassName) != 0:
			return True

	return False
		
def unsetSkill():
	import LUSkillsSelection
	MyChar = GemRB.GetVar ("Slot")
	LUSkillsSelection.SkillsNullify (MyChar)
	
def getSkills(TextAreaControl):
	MyChar = GemRB.GetVar ("Slot")
	# thieving and other skills
	info = ""
	SkillTable = GemRB.LoadTable ("skills")
	SkillPtsTable = GemRB.LoadTable ("thiefskl")
	SkillMapTable = GemRB.LoadTable ("thiefscl")
	ClassName = GUICommon.GetClassRowName (MyChar)
	RangerSkills = CommonTables.ClassSkills.GetValue (ClassName, "RANGERSKILL")
	BardSkills = CommonTables.ClassSkills.GetValue (ClassName, "BARDSKILL")
	KitName = GUICommon.GetKitRowName (MyChar)

	if SkillPtsTable.GetValue (KitName, "LEVEL_POINTS") != 0 or BardSkills != "*" or RangerSkills != "*":
		for skill in range(SkillTable.GetRowCount ()):
			skillRow = SkillTable.GetRowName (skill)
			name = SkillTable.GetValue (skillRow, "CAP_REF", GTV_REF)
			available = SkillMapTable.GetValue (skillRow, KitName, GTV_INT)
			available += SkillMapTable.GetValue (skillRow, ClassName, GTV_INT)
			statID = SkillTable.GetValue (skillRow, "ID", GTV_INT)
			value = GemRB.GetPlayerStat (MyChar, statID)
			if value >= 0 and available != 0:
				info += name + ": " + str(value) + "\n"
				
	if info != "":
		info = "\n" + info + ""
		TextAreaControl.Append (8442)
		TextAreaControl.Append (info)
	
def unsetProfi():
	MyChar = GemRB.GetVar ("Slot")
	LUProfsSelection.ProfsNullify ()
	LUProfsSelection.ProfsSave(MyChar, LUProfsSelection.LUPROFS_TYPE_CHARGEN)

def getProfi(TextAreaControl):
	MyChar = GemRB.GetVar ("Slot")
	# weapon proficiencies
	TextAreaControl.Append ("\n")
	TextAreaControl.Append (9466)
	TextAreaControl.Append ("\n")
	TmpTable=GemRB.LoadTable ("weapprof")
	ProfCount = TmpTable.GetRowCount ()
	#bg2 weapprof.2da contains the bg1 proficiencies too, skipping those
	for i in range(ProfCount):
		# 4294967296 overflows to -1 on some arches, so we use a smaller invalid strref
		stat = TmpTable.GetValue (i, 0) + IE_PROFICIENCYBASTARDSWORD
		Weapon = TmpTable.GetValue (i, 1, GTV_REF)
		Value = GemRB.GetPlayerStat (MyChar, stat)
		if Value:
			pluses = " " + "+" * Value
			TextAreaControl.Append (Weapon + pluses + "\n")

#Appearance
def unsetColors():
	MyChar = GemRB.GetVar ("Slot")
	PaperDoll.ResetStats (MyChar)

def unsetSounds():
	MyChar = GemRB.GetVar ("Slot")
	GemRB.SetPlayerSound(MyChar,"")


#name
def unsetName():
	MyChar = GemRB.GetVar ("Slot")
	GemRB.SetPlayerName (MyChar, "", 0)
	
def getName(TextAreaControl):
	MyChar = GemRB.GetVar ("Slot")
	name = GemRB.GetPlayerName(MyChar)
	if(name != ""):
		TextAreaControl.Append(name + "\n")

#divine spells
def setDivineSpells():
	MyChar = GemRB.GetVar ("Slot")
	
	ClassName = GUICommon.GetClassRowName (MyChar)
	
	DruidTable = CommonTables.ClassSkills.GetValue (ClassName, "DRUIDSPELL")
	ClericTable = CommonTables.ClassSkills.GetValue (ClassName, "CLERICSPELL")

	# we need to use the maximum of both, if there are two classes
	# cleric/rangers otherwise don't get any slots
	# luckily it's level 1 only, so we don't need to compare several spell levels
	maxSlots = (0, "")
	for tableName in (ClericTable, DruidTable):
		if tableName == "*":
			continue
		slots = Spellbook.SetupSpellLevels (MyChar, tableName, IE_SPELL_TYPE_PRIEST, 1)
		if slots > maxSlots[0]:
			maxSlots = (slots, tableName)

	if maxSlots[0] == 0: # rangers and paladins don't get spells right away
		return False

	for tableName in (ClericTable, DruidTable):
		if tableName == "*":
			continue
		classFlag = Spellbook.GetClassFlag (tableName)

		Spellbook.SetupSpellLevels (MyChar, maxSlots[1], IE_SPELL_TYPE_PRIEST, 1)
		Spellbook.LearnPriestSpells (MyChar, 1, classFlag)

	return False

def unsetDivineSpells():
	print("unsetDivineSpells")
	MyChar = GemRB.GetVar ("Slot")
	Spellbook.RemoveKnownSpells (MyChar, IE_SPELL_TYPE_PRIEST, 1, 1, 1)

def getDivineSpells(TextAreaControl):
	MyChar = GemRB.GetVar ("Slot")
	# divine spells
	info = ""
	for level in range(0, 7):
		for j in range(0, GemRB.GetKnownSpellsCount (MyChar, IE_SPELL_TYPE_PRIEST, level) ):
			Spell = GemRB.GetKnownSpell (MyChar, IE_SPELL_TYPE_PRIEST, level, j)
			Spell = GemRB.GetSpell (Spell['SpellResRef'], 1)['SpellName']
			info += GemRB.GetString (Spell) + "\n"
	if info != "":
		info = "\n" + info + ""
		TextAreaControl.Append (11028)
		TextAreaControl.Append (info)
		
#finish
def setAccept():	
	#set my character up
	MyChar = GemRB.GetVar ("Slot")

	#reputation
	BGCommon.SetReputation ()

	# don't reset most stats of imported chars
	if GemRB.GetVar ("ImportedChar"):
		CustomizeChar (MyChar)
		RunGame (MyChar)
		return

	ClassName = GUICommon.GetClassRowName (MyChar)

	#lore, thac0, hp, and saves
	GemRB.SetPlayerStat (MyChar, IE_MAXHITPOINTS, 0)
	GemRB.SetPlayerStat (MyChar, IE_HITPOINTS, 0)
	LUCommon.SetupSavingThrows (MyChar)
	LUCommon.SetupThaco (MyChar)
	LUCommon.SetupLore (MyChar)
	LUCommon.SetupHP (MyChar)

	#gold
	TmpTable=GemRB.LoadTable ("strtgold")
	t = GemRB.Roll (TmpTable.GetValue (ClassName,"ROLLS"),TmpTable.GetValue(ClassName,"SIDES"), TmpTable.GetValue (ClassName,"MODIFIER") )
	GemRB.SetPlayerStat (MyChar, IE_GOLD, t*TmpTable.GetValue (ClassName,"MULTIPLIER") )

	#set the base number of attacks; effects will add the proficiency bonus
	GemRB.SetPlayerStat (MyChar, IE_NUMBEROFATTACKS, 2)

	CustomizeChar (MyChar)

	#10 is a weapon slot (see slottype.2da row 10)
	GemRB.CreateItem (MyChar, "staf01", 10, 1, 0, 0)
	GemRB.SetEquippedQuickSlot (MyChar, 0)

	# apply class/kit abilities
	GUICommon.ResolveClassAbilities (MyChar, ClassName)

	RunGame (MyChar)

def CustomizeChar(MyChar):
	LargePortrait = GemRB.GetToken ("LargePortrait")
	SmallPortrait = GemRB.GetToken ("SmallPortrait")
	GemRB.FillPlayerInfo (MyChar, LargePortrait, SmallPortrait)
	GemRB.SetPlayerString (MyChar, 74, 11863)

def RunGame(MyChar):
	#LETS PLAY!!
	playmode = GemRB.GetVar ("PlayMode")
	GemRB.SetVar ("ImportedChar", 0)
		
	if playmode is not None:
		CharGenCommon.close()
		GemRB.SaveCharacter (MyChar, "gembak")
		GemRB.EnterGame()
	else:
		#show the export window
		GemRB.SetToken("NextScript","CharGen")
		GemRB.SetNextScript ("ExportFile")
