# -*-python-*-
# GemRB - Infinity Engine Emulator
# Copyright (C) 2003-2004 The GemRB Project
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
#
# $Id:$

import GemRB
from GUIDefines import *
from ie_stats import *
from GUICommon import HasTOB, GetLearnablePriestSpells, HasSpell, AddClassAbilities, RemoveClassAbilities
from GUIREC import UpdateRecordsWindow
from GUICommonWindows import IsDualSwap, GetActorClassTitle, GetKitIndex, KitListTable, ClassTable
from LUSpellSelection import *
from LUProfsSelection import *

#######################
pc = 0
OldClassName = 0
ClassName = 0
NewMageSpells = 0
NewThiefSkills = 0
NewPriestMask = 0
#######################
DCMainWindow = 0
DCMainClassButton = 0
DCMainSkillsButton = 0
DCMainDoneButton = 0
DCMainStep = 0
#######################
DCClassWindow = 0
DCClassDoneButton = 0
DCClass = 0
DCClasses = []
#######################
DCProfsWindow = 0
DCProfsDoneButton = 0
#######################
DCSkillsWindow = 0
DCSkillsDoneButton = 0
DCSkillsLeft = 0
DCSkillsRowCount = 0
DCSkillsTable = 0
DCSkillsIndices = []
OldPos = 0
OldDirection = 0
ClickCount = 0
#######################

# open the dual class window
def DualClassWindow ():
	global pc, OldClassName, ClassTable, NewMageSpells, NewThiefSkills, NewPriestMask, DCSkillsIndices
	global DCMainWindow, DCMainClassButton, DCMainDoneButton, DCMainSkillsButton, DCMainStep

	# get our basic globals
	pc = GemRB.GameGetSelectedPCSingle ()
	DCMainStep = 1

	# make sure to nullify old values
	NewPriestMask = 0
	NewMageSpells = 0
	NewThiefSkills = 0
	DCSkillsIndices = []
	
	# set up our main window
	DCMainWindow = GemRB.LoadWindowObject (5)

	# done button (off)
	DCMainDoneButton = DCMainWindow.GetControl (2)
	DCMainDoneButton.SetText (11973)
	DCMainDoneButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "DCMainDonePress")
	DCMainDoneButton.SetState (IE_GUI_BUTTON_DISABLED)
	DCMainDoneButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	# cancel button (on)
	DCMainCancelButton = DCMainWindow.GetControl (1)
	DCMainCancelButton.SetText (13727)
	DCMainCancelButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "DCMainCancelPress")
	DCMainCancelButton.SetState (IE_GUI_BUTTON_ENABLED)
	DCMainCancelButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	# class button (on)
	DCMainClassButton = DCMainWindow.GetControl (3)
	DCMainClassButton.SetText (11959)
	DCMainClassButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "DCMainClassPress")
	DCMainClassButton.SetState (IE_GUI_BUTTON_ENABLED)
	DCMainClassButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	# skills button (off)
	DCMainSkillsButton = DCMainWindow.GetControl (4)
	DCMainSkillsButton.SetText (17372)
	DCMainSkillsButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "DCMainSkillsPress")
	DCMainSkillsButton.SetState (IE_GUI_BUTTON_DISABLED)
	DCMainSkillsButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	# back button (on)
	DCMainBackButton = DCMainWindow.GetControl (5)
	DCMainBackButton.SetText (15416)
	DCMainBackButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "DCMainBackPress")
	DCMainBackButton.SetState (IE_GUI_BUTTON_ENABLED)
	DCMainBackButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	# picture of character
	DCMainPictureButton = DCMainWindow.GetControl (6)
	DCMainPictureButton.SetState (IE_GUI_BUTTON_LOCKED)
	DCMainPictureButton.SetFlags (IE_GUI_BUTTON_NO_IMAGE | IE_GUI_BUTTON_PICTURE, OP_SET)
	DCMainPictureButton.SetPicture (GemRB.GetPlayerPortrait (pc, 0), "NOPORTMD")

	# text area warning
	DCTextArea = DCMainWindow.GetControl (7)
	DCTextArea.SetText (10811)

	# character name
	DCLabel = DCMainWindow.GetControl (0x10000008)
	DCLabel.SetText (GemRB.GetPlayerName (pc, 0))

	# class name
	OldClassName = GetActorClassTitle (pc)
	DCLabel = DCMainWindow.GetControl (0x10000009)
	DCLabel.SetText (OldClassName)

	# get the names of the classes we can dual to
	DualClassTable = GemRB.LoadTableObject ("dualclas")
	for i in range (DualClassTable.GetColumnCount ()):
		DCClasses.append (DualClassTable.GetColumnName (i))

	# show our window
	DCMainWindow.ShowModal (MODAL_SHADOW_GRAY)
	return

# we're done!
def DCMainDonePress ():
	# save our proficiencies
	ProfsSave (pc, LUPROFS_TYPE_DUALCLASS)

	# remove old class abilities
	ClassSkillsTable = GemRB.LoadTableObject ("clskills")
	KitIndex = GetKitIndex (pc)
	if KitIndex:
		ABTable = KitListTable.GetValue (str(KitIndex), "ABILITIES")
	else:
		ABTable = ClassSkillsTable.GetValue (OldClassName, "ABILITIES")
	if ABTable != "*" and ABTable[:6] != "CLABMA": # mage kits specify ability tables which don't exist
		RemoveClassAbilities (pc, ABTable, GemRB.GetPlayerStat (pc, IE_LEVEL))

	# remove old class casting
	if not NewMageSpells:
		for i in range (9):
			GemRB.SetMemorizableSpellsCount (pc, 0, IE_SPELL_TYPE_WIZARD, i)
	RemoveKnownSpells (pc, IE_SPELL_TYPE_PRIEST, 1,7, 1)

	# apply our class abilities
	ABTable = ClassSkillsTable.GetValue (ClassName, "ABILITIES")
	if ABTable != "*" and ABTable[:6] != "CLABMA": # mage kits specify ability tables which don't exist
		AddClassAbilities (pc, ABTable)

	# learn our new priest spells
	if NewPriestMask:
		LearnPriestSpells (pc, 1, NewPriestMask)
		GemRB.SetMemorizableSpellsCount (pc, 1, IE_SPELL_TYPE_PRIEST, 0)

	# save our thief skills if we have them
	for skill in DCSkillsIndices:
		SkillID = DCSkillsTable.GetValue (skill+2, 2)
		GemRB.SetPlayerStat (pc, SkillID, GemRB.GetVar ("Skill "+str(skill)))

	# save our new class and say was multi
	NewClassId = ClassTable.GetValue (ClassName, "ID", 1)
	OldClassId = GemRB.GetPlayerStat (pc, IE_CLASS)
	OldClassIndex = ClassTable.FindValue (5, OldClassId)
	MultClassId = (1 << (NewClassId-1)) | (1 << (OldClassId-1))
	MultClassId = ClassTable.FindValue (4, MultClassId)
	MultClassId = ClassTable.GetValue (MultClassId, 5)
	GemRB.SetPlayerStat (pc, IE_CLASS, MultClassId)
	GemRB.SetPlayerStat (pc, IE_MC_FLAGS, ClassTable.GetValue (OldClassIndex, 15))

	# update our levels and xp
	if IsDualSwap (pc):
		GemRB.SetPlayerStat (pc, IE_LEVEL2, 1)
	else:
		GemRB.SetPlayerStat (pc, IE_LEVEL2, GemRB.GetPlayerStat (pc, IE_LEVEL))
		GemRB.SetPlayerStat (pc, IE_LEVEL, 1)
	GemRB.SetPlayerStat (pc, IE_XP, 0)

	# new thac0
	ThacoTable = GemRB.LoadTableObject ("THAC0")
	GemRB.SetPlayerStat (pc, IE_THAC0, ThacoTable.GetValue (NewClassId-1, 0, 1))

	# new saves
	SavesTable = ClassTable.GetValue (ClassTable.FindValue (5, NewClassId), 3, 0)
	SavesTable = GemRB.LoadTableObject (SavesTable)
	for i in range (5):
		GemRB.SetPlayerStat (pc, IE_SAVEVSDEATH+i, SavesTable.GetValue (i, 0))

	# close our window
	if DCMainWindow:
		DCMainWindow.Unload ()
	UpdateRecordsWindow()
	return

# undo everything
def DCMainCancelPress ():
	# simulate pressing the back button until we get back to DCMainStep = 1
	# to unset all things from the new class
	while DCMainStep > 1:
		DCMainBackPress ()

	# close our window
	if DCMainWindow:
		DCMainWindow.Unload ()

	return

# goes to the previous step
def DCMainBackPress ():
	global DCMainStep, DCClass, NewMageSpells, DCSkillsIndices, NewThiefSkills
	global NewPriestMask

	if DCMainStep == 2: # class selected, wait to choose skills
		# disable the skills button and re-enable the class button
		# the class will be reset when the class button is clicked
		DCMainSkillsButton.SetState (IE_GUI_BUTTON_DISABLED)
		DCMainClassButton.SetState (IE_GUI_BUTTON_ENABLED)

		# back a step
		DCMainStep = 1
	elif DCMainStep == 3: # skills selected
		# re-enable our skills button
		DCMainSkillsButton.SetState (IE_GUI_BUTTON_ENABLED)

		# un-learn our spells and skills
		if NewMageSpells:
			RemoveKnownSpells (pc, IE_SPELL_TYPE_WIZARD, 1,1, 1)
			NewMageSpells = 0
		if NewThiefSkills:
			for skill in DCSkillsIndices:
				GemRB.SetVar ("Skill "+str(skill), 0)
			DCSkillsIndices = []
			NewThiefSkills = 0

		NewPriestMask = 0

		# go back a step
		DCMainStep = 2
	return

# opens the class selection window
def DCMainClassPress ():
	global DCClassWindow, DCClassDoneButton, DCClass

	# we default the class back down to -1
	DCClass = -1
	GemRB.SetVar ("DCClass", DCClass)

	# open the window
	DCClassWindow = GemRB.LoadWindowObject (6)

	# string refs for the given classes
	DCClassStrings = []
	for classname in DCClasses:
		DCClassStrings.append(ClassTable.GetValue (classname, "NAME_REF", 1))

	# setup the class buttons
	for i in range (6):
		# get the button and associate it with the correct var
		DCClassButton = DCClassWindow.GetControl (i+1)
		DCClassButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "DCClassSelect")
		DCClassButton.SetVarAssoc ("DCClass", i)
		DCClassButton.SetText (DCClassStrings[i])
		DCClassButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

		# enable only if we can dual into the given class
		if CanDualInto (i):
			DCClassButton.SetState (IE_GUI_BUTTON_ENABLED)
		else:
			DCClassButton.SetState (IE_GUI_BUTTON_DISABLED)

	# done button (off)
	DCClassDoneButton = DCClassWindow.GetControl (8)
	DCClassDoneButton.SetText (11973)
	DCClassDoneButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "DCClassDonePress")
	DCClassDoneButton.SetState (IE_GUI_BUTTON_DISABLED)
	DCClassDoneButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	# back button (on)
	DCClassBackButton = DCClassWindow.GetControl (7)
	DCClassBackButton.SetText (15416)
	DCClassBackButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "DCClassBackPress")
	DCClassBackButton.SetState (IE_GUI_BUTTON_ENABLED)
	DCClassBackButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	# setup the text area with default text
	DCClassTextArea = DCClassWindow.GetControl (9)
	DCClassTextArea.SetText (10949)

	# show the window
	DCClassWindow.ShowModal (MODAL_SHADOW_GRAY)

	return

# used whenever a class is clicked
def DCClassSelect ():
	global DCClass

	# un-select the old button and save and select the new one
	if DCClass >= 0:
		DCClassButton = DCClassWindow.GetControl (DCClass+1)
		DCClassButton.SetState (IE_GUI_BUTTON_ENABLED)

	# if we clicked the same class twice, turn the done button off (toggled)
	if DCClass == GemRB.GetVar ("DCClass"):
		DCClassDoneButton.SetState (IE_GUI_BUTTON_DISABLED)
		DCClass = -1
		
		# don't need to worry about setting a new text area, as it would be the same
		return

	# save the new class, select it's button, and enable the done button
	DCClass = GemRB.GetVar ("DCClass")
	DCClassButton = DCClassWindow.GetControl (DCClass+1)
	DCClassButton.SetState (IE_GUI_BUTTON_SELECTED)
	DCClassDoneButton.SetState (IE_GUI_BUTTON_ENABLED)

	# all the possible strrefs for the different classes
	DCClassStrings = []
	for classname in DCClasses:
		DCClassStrings.append (ClassTable.GetValue (classname, "DESC_REF", 1))

	# update the text are with the new string
	DCClassTextArea = DCClassWindow.GetControl (9)
	DCClassTextArea.SetText (DCClassStrings [DCClass])

	return

# used when a class is selected by pressing the done button
def DCClassDonePress ():
	global DCMainStep, ClassName

	# unload our class selection window
	if DCClassWindow:
		DCClassWindow.Unload ()

	# enable the skills button and disable the class selection button
	DCMainClassButton.SetState (IE_GUI_BUTTON_DISABLED)
	DCMainSkillsButton.SetState (IE_GUI_BUTTON_ENABLED)

	# save the class
	ClassName = DCClasses[DCClass]

	# set our step to 2 so that the back button knows where we are
	DCMainStep = 2

	return

# returns boolean
def CanDualInto (index):
	# make sure index isn't out of range
	if index < 0 or index >= len (DCClasses):
		return 0

	# find our row
	DCTable = GemRB.LoadTableObject ("dualclas")

	# return 0 if we can't dual into the class
	if not DCTable.GetValue (OldClassName, DCClasses[index], 1):
		return 0

	# make sure we aren't restricted by alignment
	AlignmentTable = GemRB.LoadTableObject ("alignmnt")
	AlignsTable = GemRB.LoadTableObject ("aligns")
	Alignment = GemRB.GetPlayerStat (pc, IE_ALIGNMENT) # our alignment
	Alignment = AlignsTable.FindValue (3, Alignment)
	Alignment = AlignsTable.GetValue (Alignment, 4) # convert the alignment
	if not AlignmentTable.GetValue (DCClasses[index], Alignment, 1): # check it
		return 0

	# make sure we have the minimum stats required for the next class
	StatTable = GemRB.LoadTableObject ("abdcdsrq")
	ClassStatIndex = StatTable.GetRowIndex (DCClasses[index])
	for stat in range (6): # loop through each stat
		minimum = StatTable.GetValue (ClassStatIndex, stat)
		name = StatTable.GetColumnName (stat)
		if GemRB.GetPlayerStat (pc, eval("IE_" + name[4:])) < minimum: # see if we're under the minimum
			return 0

	# if we made it here, we can dual to the class
	return 1

def DCClassBackPress ():
	# close the class window
	if DCClassWindow:
		DCClassWindow.Unload ()
	return

def DCMainSkillsPress ():
	global DCSkillsWindow, DCMainStep, DCHasProfs

	# we're onto the next step
	DCMainSkillsButton.SetState (IE_GUI_BUTTON_DISABLED)
	DCMainStep = 3

	# TODO: go to proficiencies first
	DCOpenProfsWindow ()
	return

# opens the proficiencies window
def DCOpenProfsWindow ():
	global DCProfsWindow, DCProfsDoneButton

	# load up our window and set some basic variables
	DCProfsWindow = GemRB.LoadWindowObject (15)
	NewClassId = ClassTable.GetValue (ClassName, "ID", 1)
	SetupProfsWindow (pc, LUPROFS_TYPE_DUALCLASS, DCProfsWindow, DCProfsRedraw, classid=NewClassId)

	# setup the done and cancel
	DCProfsDoneButton = DCProfsWindow.GetControl (76)
	DCProfsDoneButton.SetText (11973)
	DCProfsDoneButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "DCProfsDonePress")
	DCProfsDoneButton.SetState (IE_GUI_BUTTON_DISABLED)
	DCProfsDoneButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	DCProfsCancelButton = DCProfsWindow.GetControl (77)
	DCProfsCancelButton.SetText (13727)
	DCProfsCancelButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "DCProfsCancelPress")
	DCProfsCancelButton.SetState (IE_GUI_BUTTON_ENABLED)
	DCProfsCancelButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	# show the window and draw away
	DCProfsWindow.ShowModal (MODAL_SHADOW_GRAY)
	DCProfsRedraw ()
	return

# redraw the profs -- called whenever anything changes
def DCProfsRedraw ():
	ProfsPointsLeft = GemRB.GetVar ("ProfsPointsLeft")

	if ProfsPointsLeft == 0:
		DCProfsDoneButton.SetState (IE_GUI_BUTTON_ENABLED)
	else:
		DCProfsDoneButton.SetState (IE_GUI_BUTTON_DISABLED)
	return

# goes to the next scripts (mage or thief)
def DCProfsDonePress ():
	global NewMageSpells, NewThiefSkills, NewPriestMask

	# check for mage spells and thief skills
	ClassSkillsTable = GemRB.LoadTableObject ("clskills")
	SpellTable = ClassSkillsTable.GetValue (ClassName, "MAGESPELL")
	ThiefTable = ClassSkillsTable.GetValue (ClassName, "THIEFSKILL")
	ClericTable = ClassSkillsTable.GetValue (ClassName, "CLERICSPELL")
	DruidTable = ClassSkillsTable.GetValue (ClassName, "DRUIDSPELL")
	if SpellTable != "*":
		# we go 2,2 to get an extra spell
		# TODO: add a mod to the function instead?
		OpenSpellsWindow (pc, SpellTable, 2, 2, 0)
		SpellTable = GemRB.LoadTableObject (SpellTable)
		GemRB.SetMemorizableSpellsCount (pc, SpellTable.GetValue (0, 0), IE_SPELL_TYPE_WIZARD, 0)
		NewMageSpells = 1
	if ThiefTable != "*":
		# open the thieves selection window
		OpenSkillsWindow ()
		NewThiefSkills = 1
	if ClericTable != "*":
		print "Setting PriestMask"
		# make sure we can cast spells at this level (paladins)
		ClericTable = GemRB.LoadTableObject (ClericTable)
		if ClericTable.GetRowName (0) == "1":
			NewPriestMask = 0x4000
	elif DruidTable != "*":
		# make sure we can cast spells at this level (rangers)
		if HasTOB ():
			DruidTable = GemRB.LoadTableObject (DruidTable)
			if DruidTable.GetRowName (0) == "1":
				NewPriestMask = 0x8000
		else:
			NewPriestMask = 0x8000

	# we can be done now
	DCMainDoneButton.SetState (IE_GUI_BUTTON_ENABLED)

	# close out the profs window (don't assign yet!)
	if DCProfsWindow:
		DCProfsWindow.Unload ()
	return

# undoes our changes
def DCProfsCancelPress ():
	# close out the profs window and go back a step
	if DCProfsWindow:
		DCProfsWindow.Unload ()

	DCMainBackPress ()
	return

# open the window to select thief skills
def OpenSkillsWindow ():
	global DCSkillsWindow, DCSkillsLeft, DCSkillsDoneButton, DCSkillsTable
	global DCSkillsIndices, DCSkillRowCount

	# get some required values
	DCSkillsTable = GemRB.LoadTableObject ("skills")
	SkillRacTable = GemRB.LoadTableObject ("SKILLRAC")
	RaceTable = GemRB.LoadTableObject ("RACES")
	Race = RaceTable.FindValue (3, GemRB.GetPlayerStat (pc, IE_RACE))
	Race = RaceTable.GetRowName (Race)
	DCSkillsRowCount = DCSkillsTable.GetRowCount ()-2

	# setup our starting points
	for i in range (DCSkillsRowCount):
		SkillName = DCSkillsTable.GetRowName (i+2)
		SkillID = DCSkillsTable.GetValue (i+2, 2)
		
		# we need to add the racial bonuses to the starting value
		if DCSkillsTable.GetValue (SkillName, ClassName) == 1:
			value = SkillRacTable.GetValue (Race, SkillName)
			DCSkillsIndices.append(i)
			GemRB.SetVar ("Skill "+str(i), value)
			GemRB.SetVar ("SkillBase "+str(i), value)

	# figure out how many points we have left
	DCSkillsLeft = DCSkillsTable.GetValue ("FIRST_LEVEL", ClassName)

	DCSkillsWindow = GemRB.LoadWindowObject (7)

	# setup our scrollbar if needed
	if len(DCSkillsIndices) > 4:
		GemRB.SetVar("SkillsTopIndex", 0)
		ScrollBar = DCSkillsWindow.GetControl(26)
		ScrollBar.SetEvent(IE_GUI_SCROLLBAR_ON_CHANGE, "DCSkillsRedraw")
		#decrease it with the number of controls on screen (list size)
		ScrollBar.SetVarAssoc("SkillsTopIndex", DCSkillsRowCount-3)
		ScrollBar.SetDefaultScrollBar ()
	else:
		if len(DCSkillsIndices):
			# autoscroll to the first valid skill; luckily all three monk ones are adjacent
			# NOTE: leaving this here in case dualing to monks is enabled for some reason
			GemRB.SetVar("SkillTopIndex", DCSkillsIndices[0])

	# setup the skills buttons
	for i in range(len(DCSkillsIndices)):
		# we have only 4 controls
		if i == 4:
			break

		Button = DCSkillsWindow.GetControl(i+5)
		Button.SetVarAssoc("Skill",DCSkillsIndices[i])
		Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, "DCSkillsJustPress")

		Button = DCSkillsWindow.GetControl(i*2+14)
		Button.SetVarAssoc("Skill",DCSkillsIndices[i])
		Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, "DCSkillsLeftPress")

		Button = DCSkillsWindow.GetControl(i*2+15)
		Button.SetVarAssoc("Skill",DCSkillsIndices[i])
		Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, "DCSkillsRightPress")

	# setup the back and done buttons
	BackButton = DCSkillsWindow.GetControl(24)
	BackButton.SetText(15416)
	BackButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)
	BackButton.SetEvent(IE_GUI_BUTTON_ON_PRESS,"DCSkillsBackPress")

	DCSkillsDoneButton = DCSkillsWindow.GetControl(25)
	DCSkillsDoneButton.SetText(11973)
	DCSkillsDoneButton.SetFlags(IE_GUI_BUTTON_DEFAULT,OP_OR)
	DCSkillsDoneButton.SetEvent(IE_GUI_BUTTON_ON_PRESS,"DCSkillsDonePress")
	DCSkillsDoneButton.SetState(IE_GUI_BUTTON_DISABLED)

	# setup the default text area
	TextArea = DCSkillsWindow.GetControl(22)
	TextArea.SetText(17248)

	DCSkillsWindow.ShowModal (MODAL_SHADOW_GRAY)
	DCSkillsRedraw ()
	GemRB.SetRepeatClickFlags(GEM_RK_DISABLE, OP_NAND)
	return

# updates the window
def DCSkillsRedraw (direction=0):
	global OldDirection, ClickCount

	SkillsTopIndex = GemRB.GetVar ("SkillsTopIndex")

	# no points left? we can be done! :)
	if DCSkillsLeft == 0:
		DCSkillsDoneButton.SetState (IE_GUI_BUTTON_ENABLED)

	# update the points left label
	SumLabel = DCSkillsWindow.GetControl (0x10000008)
	SumLabel.SetText (str(DCSkillsLeft))

	# update each of the skills
	for i in range (4):
		# show nothing if we can only get less than 4 skills
		if len (DCSkillsIndices) <= i:
			Label = DCSkillsWindow.GetControl (0x1000000+i)
			Label.SetText ("")
			Button1 = DCSkillsWindow.GetControl (i*2+14)
			Button1.SetState (IE_GUI_BUTTON_DISABLED)
			Button1.SetFlags (IE_GUI_BUTTON_NO_IMAGE,OP_OR)
			Button2 = DCSkillsWindow.GetControl (i*2+15)
			Button2.SetState (IE_GUI_BUTTON_DISABLED)
			Button2.SetFlags (IE_GUI_BUTTON_NO_IMAGE,OP_OR)
			Label = DCSkillsWindow.GetControl (0x10000009+i)
			Label.SetText ("")
			continue

		# show the skill name
		Pos = DCSkillsIndices[SkillsTopIndex+i]
		SkillName = DCSkillsTable.GetValue (Pos+2,1)
		Label = DCSkillsWindow.GetControl (0x10000000+i)
		Label.SetText (SkillName)

		# enable or disable our buttons as appropriate
		SkillName = DCSkillsTable.GetRowName (Pos+2)
		Button1 = DCSkillsWindow.GetControl (i*2+14)
		Button2 = DCSkillsWindow.GetControl (i*2+15)
		if DCSkillsTable.GetValue (SkillName, ClassName) == -1:
			Button1.SetState (IE_GUI_BUTTON_DISABLED)
			Button2.SetState (IE_GUI_BUTTON_DISABLED)
			Button1.SetFlags (IE_GUI_BUTTON_NO_IMAGE,OP_OR)
			Button2.SetFlags (IE_GUI_BUTTON_NO_IMAGE,OP_OR)
		else: # can learn
			Button1.SetState (IE_GUI_BUTTON_ENABLED)
			Button2.SetState (IE_GUI_BUTTON_ENABLED)
			Button1.SetFlags (IE_GUI_BUTTON_NO_IMAGE,OP_NAND)
			Button2.SetFlags (IE_GUI_BUTTON_NO_IMAGE,OP_NAND)

		# update the points label
		Label = DCSkillsWindow.GetControl (0x10000009+i)
		ActPoint = GemRB.GetVar ("Skill "+str(Pos))
		Label.SetText (str(ActPoint))

	# go double-speed if we have enough clicks
	if direction:
		if OldDirection == direction:
			ClickCount += 1
			if ClickCount>10:
				GemRB.SetRepeatClickFlags (GEM_RK_DOUBLESPEED, OP_OR)
				return

	OldDirection = direction
	ClickCount = 0
	GemRB.SetRepeatClickFlags (GEM_RK_DOUBLESPEED, OP_NAND)
	return

# gives info about the skill
def DCSkillsJustPress():
	SkillsTopIndex = GemRB.GetVar ("SkillsTopIndex")
	Pos = GemRB.GetVar ("Skill")+SkillsTopIndex
	TextArea = DCSkillsWindow.GetControl (22)
	TextArea.SetText (DCSkillsTable.GetValue (Pos+2,0))
	return

# decreases the skill
def DCSkillsRightPress():
	global DCSkillsLeft, ClickCount, OldPos

	# get the points
	SkillsTopIndex = GemRB.GetVar ("SkillsTopIndex")
	Pos = GemRB.GetVar ("Skill")+SkillsTopIndex
	ActPoint = GemRB.GetVar ("Skill "+str(Pos))
	BasePoint = GemRB.GetVar ("SkillBase "+str(Pos))

	# make sure we can take the point away
	if ActPoint <= 0 or ActPoint == BasePoint:
		return
	
	# decrease the skill and increase the overall points left
	GemRB.SetVar("Skill "+str(Pos),ActPoint-1)
	DCSkillsLeft += 1

	# update the pos for double-speed
	if OldPos != Pos:
		OldPos = Pos
		ClickCount = 0

	DCSkillsJustPress ()
	DCSkillsRedraw (1)
	return

def DCSkillsLeftPress():
	global DCSkillsLeft, ClickCount, OldPos

	# make sure we can allocate points to the skill
	SkillsTopIndex = GemRB.GetVar ("SkillsTopIndex")
	Pos = GemRB.GetVar ("Skill")+SkillsTopIndex
	if DCSkillsLeft == 0:
		return
	ActPoint = GemRB.GetVar ("Skill "+str(Pos))
	if ActPoint >= 200:
		return

	# increase the skill and decrease the overall points left
	GemRB.SetVar ("Skill "+str(Pos), ActPoint+1)
	DCSkillsLeft -= 1

	# update double-speed
	if OldPos != Pos:
		OldPos = Pos
		ClickCount = 0

	DCSkillsJustPress ()
	DCSkillsRedraw (2)
	return

# undoes all changes so far
def DCSkillsBackPress ():
	if DCSkillsWindow:
		DCSkillsWindow.Unload ()
	for i in range(DCSkillsRowCount):
		GemRB.SetVar ("Skill "+str(i), 0)
		GemRB.SetVar ("SkillBase "+str(i), 0)
	DCMainBackPress ()
	GemRB.SetRepeatClickFlags (GEM_RK_DISABLE, OP_OR)
	return

# closes the skills window
def DCSkillsDonePress ():
	if DCSkillsWindow:
		DCSkillsWindow.Unload ()
	GemRB.SetRepeatClickFlags (GEM_RK_DISABLE, OP_OR)
	return

# learn all the priest spells up to a given spell level
# mask is 0x4000 = cleric and 0x8000 = druid
# level 1 is 1st level spells
def LearnPriestSpells (pc, level, mask):
	if level > 7: # make sure we don't have too high a level
		level = 7

	# go through each level
	for i in range (level):
		learnable = GetLearnablePriestSpells (mask, GemRB.GetPlayerStat (pc, IE_ALIGNMENT), i+1)

		for spell in learnable:
			# if the spell isn't learned, learn it
			if HasSpell (pc, IE_SPELL_TYPE_PRIEST, i, spell) < 0:
				GemRB.LearnSpell (pc, spell)
