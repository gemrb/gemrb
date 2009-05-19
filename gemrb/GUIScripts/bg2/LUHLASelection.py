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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#
# $Id:$

import GemRB
from GUIDefines import *
from ie_stats import *
from GUICommon import HasSpell
from GUIREC import GetKitIndex
from GUICommonWindows import IsDualClassed
from LevelUp import RedrawSkills

# HLA selection
HLAWindow = 0		# << HLA selection window
HLATopIndex = 0		# << HLA scrollbar index
HLAAbilities = []	# << all learnable HLA abilities
HLANewAbilities = []	# << selected HLA abilites
HLADoneButton = 0	# << done button
HLATextArea = 0		# << HLA ability description area
HLACount = 0		# << number of HLA selections left
pc = 0			# << the pc
NumClasses = 0		# << number of classes
Classes = []		# << classes (ids)
Level = []		# << levels for each class

# open our HLA window
def LevelUpHLAPress ():
	global HLAWindow, HLADoneButton, HLATextArea, HLACount, NumClasses, pc, Classes, Level

	# save our variables 
	pc = GemRB.GetVar ("PC")
	HLACount = GemRB.GetVar ("HLACount")
	NumClasses = GemRB.GetVar ("NumClasses")
	for i in range (NumClasses):
		Classes.append (GemRB.GetVar ("Class "+str(i)))
		Level.append (GemRB.GetVar ("Level "+str(i)))

	# setup our scroll index
	GemRB.SetVar ("HLATopIndex", 0)

	# we use the same window as sorcerer spell selection
	HLAWindow = GemRB.LoadWindowObject (8)

	# get all our HLAs (stored in HLAAbilities)
	GetHLAs ()

	# create the done button
	HLADoneButton = HLAWindow.GetControl (28)
	HLADoneButton.SetState(IE_GUI_BUTTON_DISABLED)
	HLADoneButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "HLADonePress")
	HLADoneButton.SetText(11973)
	HLADoneButton.SetFlags(IE_GUI_BUTTON_DEFAULT, OP_OR)

	# setup our text area
	HLATextArea = HLAWindow.GetControl(26)

	print "Number of HLAs:",len (HLAAbilities)

	# create a scrollbar if need-be
	if len (HLAAbilities) > 24:
		HLAWindow.CreateScrollBar (1000, 290,142, 16,252)
		ScrollBar = HLAWindow.GetControl (1000)
		ScrollBar.SetSprites ("GUISCRCW", 0, 0,1,2,3,5,4)
		ScrollBar.SetEvent (IE_GUI_SCROLLBAR_ON_CHANGE, "HLAShowAbilities")
		ScrollBar.SetVarAssoc ("HLATopIndex", len (HLAAbilities)-24)
		ScrollBar.SetDefaultScrollBar ()

	# draw our HLAs and show the window
	HLAShowAbilities ()
	HLAWindow.ShowModal (MODAL_SHADOW_GRAY)

	return

# close our HLA window
def HLADonePress ():
	# save all of our HLAs
	for i in range (len (HLANewAbilities)):
		# see if we're going to learn this ability
		if HLANewAbilities[i] == 0:
			continue

		# figure out the ability type
		HLARef = HLAAbilities[i][0]
		HLAType = HLARef[5:7]
		if HLAType == "PR":
			HLAType = IE_SPELL_TYPE_PRIEST
			HLALevel = int(HLARef[7])-1
		elif HLAType == "WI":
			HLAType = IE_SPELL_TYPE_WIZARD
			HLALevel = int(HLARef[7])-1
		else:
			HLAType = IE_SPELL_TYPE_INNATE
			HLALevel = 0

		# do we need to apply or learn it?
		if HLARef[:2] == "AP":
			GemRB.ApplySpell(pc, HLARef[3:])
		elif HLARef[:2] == "GA":
			# make sure it isn't already learned
			SpellIndex = HasSpell (pc, HLAType, HLALevel, HLARef[3:])
			if SpellIndex < 0: # gotta learn it
				GemRB.LearnSpell (pc, HLARef[3:], 8)
			else: # memorize it again
				GemRB.MemorizeSpell (pc, HLAType, HLALevel, SpellIndex)

	# close the window
	if HLAWindow:
		HLAWindow.Unload ()

	# and redraw the skills (to allow the Done button to be enabled)
	GemRB.SetVar ("HLACount", 0)
	RedrawSkills()

	return

# updates the HLA selection window
def HLAShowAbilities ():
	HLATopIndex = GemRB.GetVar ("HLATopIndex")

	# we have a grid of 24 abilites
	for i in range (24):
		# ensure we can learn this many abilites
		SpellButton = HLAWindow.GetControl (i)
		if i >= len (HLAAbilities):
			SpellButton.SetState (IE_GUI_BUTTON_DISABLED)
			continue

		# fill in the button with the spell data
		HLARef = HLAAbilities[i+HLATopIndex][0][3:]
		if not HLARef:
			continue
		Spell = GemRB.GetSpell (HLARef)
		SpellButton.SetTooltip(Spell['SpellName'])
		SpellButton.SetSpellIcon(HLARef, 1)
		SpellButton.SetVarAssoc("ButtonPressed", i)
		SpellButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "HLASelectPress")
		SpellButton.SetSprites("GUIBTBUT", 0,0,1,2,3)
		SpellButton.SetFlags(IE_GUI_BUTTON_PICTURE, OP_OR)

		# don't allow the selection of an un-learnable ability
		if HLAAbilities[i+HLATopIndex][1] == 0:
			SpellButton.SetState(IE_GUI_BUTTON_LOCKED)
			# shade red
			SpellButton.SetBorder (0, 0,0, 0,0, 200,0,0,100, 1,1)
		else:
			SpellButton.SetState (IE_GUI_BUTTON_ENABLED)
			# unset any borders on this button or an un-learnable from last level
			# will still shade red even though it is clickable
			SpellButton.SetBorder (0, 0,0, 0,0, 0,0,0,0, 0,0)

	# show which spells are selected
	HLAShowSelectedAbilities ()

	GemRB.SetToken("number", str(HLACount))
	HLATextArea.SetText(17250)

	# show the points left
	PointsLeftLabel = HLAWindow.GetControl (0x10000018)
	PointsLeftLabel.SetText (str (HLACount))

	return

# updates the abilites when one is selected
def HLASelectPress ():
	global HLACount, HLAAbilities, HLANewAbilities

	# get our variables
	HLATopIndex = GemRB.GetVar ("HLATopIndex")
	i = GemRB.GetVar ("ButtonPressed") + HLATopIndex

	# get the spell that's been pushed
	Spell = GemRB.GetSpell (HLAAbilities[i][0][3:])
	HLATextArea.SetText (Spell["SpellDesc"])

	# make sure we can learn the spell
	if HLAAbilities[i][1]:
		if HLANewAbilities[i]: # already picked -- unselecting
			# make we aren't the pre-req to another spell that is selected
			for j in range (len (HLAAbilities)):
				if (HLAAbilities[j][3] == HLAAbilities[i][0]) and (HLANewAbilities[j]):
					HLAShowSelectedAbilities () # so our pre-req is still highlighted
					return

			HLACount += 1
			HLANewAbilities[i] = 0
			HLAAbilities[i][2] -= 1 # internal counter
			HLADoneButton.SetState (IE_GUI_BUTTON_DISABLED)
		else: # selecting
			# we don't have any picks left
			if HLACount == 0:
				HLAMarkButton (i, 0)
				return

			# select the spell and change the done state if need be
			HLACount -= 1
			HLANewAbilities[i] = 1
			HLAAbilities[i][2] += 1 # increment internal counter
			if HLACount == 0:
				HLADoneButton.SetState (IE_GUI_BUTTON_ENABLED)

		# recheck internal exclusions and prereqs
		HLARecheckPrereqs (i)

	# show selected spells
	HLAShowAbilities ()
	HLAShowSelectedAbilities ()
	HLATextArea.SetText (Spell["SpellDesc"])

	# show the points left
	PointsLeftLabel = HLAWindow.GetControl (0x10000018)
	PointsLeftLabel.SetText (str (HLACount))
	return

# marks all of the selected abilities
def HLAShowSelectedAbilities ():
	HLATopIndex = GemRB.GetVar ("HLATopIndex")

	# mark all of the abilities picked thus far
	for i in range (24):
		if i >= len (HLANewAbilities): # make sure we don't call unavailable indexes
			break
		if HLANewAbilities[i+HLATopIndex]:
			HLAMarkButton (i+HLATopIndex, 1)
		else:
			HLAMarkButton (i+HLATopIndex, 0)

	return

# show ability as selected, enabled, or disabled
def HLAMarkButton (i, select):
	HLATopIndex = GemRB.GetVar ("HLATopIndex")

	if select:
		type = IE_GUI_BUTTON_SELECTED
	else:
		if HLAAbilities[i][1]:
			type = IE_GUI_BUTTON_ENABLED
		else: # can't learn
			type = IE_GUI_BUTTON_LOCKED

	# we have to use the index on the actual grid
	SpellButton = HLAWindow.GetControl(i-HLATopIndex)
	SpellButton.SetState(type)
	return

# return a 2d array with all the HLAs and whether or not prereqs have been met
# aka [0] = ("ref",0) and [1] = ("ref",1)
def GetHLAs ():
	global HLAAbilities, HLANewAbilities

	# get some needed values
	Kit = GetKitIndex (pc)
	IsDual = IsDualClassed (pc, 0)
	IsDual = IsDual[0] > 0

	# reset the abilities
	HLAAbilities = []
	HLANewAbilities = []

	# the HLA table lookup table
	HLAAbbrTable = GemRB.LoadTableObject ("luabbr")
	ClassTable = GemRB.LoadTableObject ("classes")

	# get all the HLAs for each class
	for i in range (NumClasses):
		ClassIndex = ClassTable.FindValue (5, Classes[i])
		ClassName = ClassTable.GetRowName (ClassIndex)
		CurrentLevel = Level[i]

		if Kit != 0 and NumClasses == 1 and not IsDual: # kitted single-class
			KitList = GemRB.LoadTableObject ("kitlist")
			KitName = KitList.GetValue (Kit, 0)
			HLAClassTable = "lu" + HLAAbbrTable.GetValue (KitName, "ABBREV")
			ClassName = KitName
		else: # everyone else
			HLAClassTable = "lu" + HLAAbbrTable.GetValue (ClassName, "ABBREV")

		# actually load the table
		HLAClassTable = GemRB.LoadTableObject (HLAClassTable)
		print "HLA Class/Kit:",ClassName

		# save all our HLAs from this class
		for j in range (HLAClassTable.GetRowCount ()):
			HLARef = HLAClassTable.GetValue (j, 0, 0)
			print "\tHLA",j,":",HLARef

			# make sure we have an ability here
			if HLARef == "*":
				print "\t\tEnd of HLAs"
				break	

			# [ref to hla, memorizable?, num memorized, pre-req ref, excluded ref]
			SaveArray = [\
				HLARef,\
				0,\
				GemRB.CountEffects (pc, "HLA", -1, -1, HLARef[3:]),\
				HLAClassTable.GetValue (j, 6, 0),\
				HLAClassTable.GetValue (j, 7, 0)]

			# make sure we fall within the min and max paramaters
			if HLAClassTable.GetValue (j, 3) > CurrentLevel or HLAClassTable.GetValue (j, 4) < CurrentLevel:
				print "\t\tNot within parameters"
				HLAAbilities.append(SaveArray)
				continue
		
			# see if we're alignment restricted (we never get them)
			HLAAlign = HLAClassTable.GetValue (j, 8, 0)
			if HLAAlign == "ALL_EVIL" and GemRB.GetPlayerStat (pc, IE_ALIGNMENT) < 6:
				# don't even save this one because we can never get it
				print "\t\tNeeds ALL_EVIL"
				continue
			elif HLAAlign == "ALL_GOOD" and GemRB.GetPlayerStat (pc, IE_ALIGNMENT) > 2:
				# ditto
				print "\t\tNeeds ALL_GOOD"
				continue

			# make sure we haven't already surpassed the number of time memorizable
			HLANumAllowed = HLAClassTable.GetValue (j, 5)
			print "\t\tHLA count:",SaveArray[2]
			if SaveArray[2] >= HLANumAllowed:
				print "\t\tOnly allowed to learn",HLANumAllowed,"times"
				HLAAbilities.append(SaveArray)
				continue

			# make sure we haven't learned an HLA that excludes this one
			HLAMemorized = GemRB.CountEffects (pc, "HLA", -1, -1, SaveArray[4][3:])
			print "\t\tHLAExcluded count:",HLAMemorized
			if (SaveArray[4] != "*") and (HLAMemorized > 0):
				print "\t\tExcluded by:",SaveArray[4]
				HLAAbilities.append(SaveArray)
				continue

			# we meet the prereqs so we can learn the HLA
			HLAMemorized = GemRB.CountEffects (pc, "HLA", -1, -1, SaveArray[3][3:])
			print "\t\tHLAPre count:",HLAMemorized
			if (SaveArray[3] == "*") or (HLAMemorized > 0):
				print "\t\tWe can learn it!"
				SaveArray[1] = 1
				HLAAbilities.append (SaveArray)
				continue

			# we didn't meet prereqs :(
			print "\t\tNeed pre-req:",SaveArray[3]
			HLAAbilities.append (SaveArray)

	# create an array to store our abilities as they are selected
	HLANewAbilities = [0]*len (HLAAbilities)

	return

# rechecks pre-reqs for HLAs on the fly
# num memorized should be updated before calling this
def HLARecheckPrereqs (index):
	# the numer of times memorized
	Ref = HLAAbilities[index][0]
	Memorized = HLAAbilities[index][2]

	# check for new exclusions and pre-reqs
	for i in range (len (HLAAbilities)):
		# we don't need to check the index
		# this also fixes the assassination bug (it is excluded by itself)
		if i == index:
			continue
		# check for exclusions first
		if HLAAbilities[i][4] == Ref:
			if Memorized > 0: # can't learn it
				HLAAbilities[i][1] = 0
			else: # can, if it meets pre-reqs
				if HLAAbilities[i][3] != "*": # check prereqs
					for j in range (len (HLAAblities)): # search for the prereq ref
						if (HLAAbilities[j][0] == HLAAbilities[i][3]) and (HLAAbilities[j][2] > 0): # can learn
							HLAAbilities[i][1] = 1
							break
				else: # no prereqs
					HLAAbilities[i][1] = 1

		# check for prereqs
		if HLAAbilities[i][3] == Ref:
			if Memorized > 0: # can learn if not excluded
				if HLAAbilities[i][4] != "*": # check for exclusions
					for j in range (len (HLAAbilities)): # search for the exclusion ref
						if (HLAAbilities[j][0] == HLAAbilities[i][4]) and (HLAAbilities[j][2] <= 0): # can learn
							HLAAbilities[i][1] = 1
							break
				else: # no exlusions
					HLAAbilities[i][1] = 1
			else: # prereqs not met
				HLAAbilities[i][1] = 0

	return


