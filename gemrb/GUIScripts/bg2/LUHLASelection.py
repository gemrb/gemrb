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

import GemRB
from GUIDefines import *
from ie_stats import *
from ie_spells import LS_MEMO
import GUICommon
import Spellbook
import CommonTables

# HLA selection
HLAWindow = 0		# << HLA selection window
HLAAbilities = []	# << all learnable HLA abilities
HLANewAbilities = []	# << selected HLA abilites
HLADoneButton = 0	# << done button
HLATextArea = 0		# << HLA ability description area
HLACount = 0		# << number of HLA selections left
pc = 0			# << the pc
NumClasses = 0		# << number of classes
Classes = []		# << classes (ids)
Level = []		# << levels for each class
EnhanceGUI = 0		# << toggle for scrollbar and 25th hla slot

def OpenHLAWindow (actor, numclasses, classes, levels):
	"""Opens the HLA selection window."""

	global HLAWindow, HLADoneButton, HLATextArea, HLACount, NumClasses, pc, Classes, Level
	global EnhanceGUI

	#enhance GUI?
	if (GemRB.GetVar("GUIEnhancements")&GE_SCROLLBARS):
		EnhanceGUI = 1

	# save our variables 
	pc = actor
	NumClasses = numclasses
	Classes = classes
	Level = levels
	HLACount = GemRB.GetVar ("HLACount")

	# we use the same window as sorcerer spell selection
	HLAWindow = GemRB.LoadWindow (8)

	# get all our HLAs (stored in HLAAbilities)
	GetHLAs ()

	# change the title to ABILITIES
	TitleLabel = HLAWindow.GetControl (0x10000017)
	TitleLabel.SetText (63818)

	# create the done button
	HLADoneButton = HLAWindow.GetControl (28)
	HLADoneButton.SetState(IE_GUI_BUTTON_DISABLED)
	HLADoneButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, HLADonePress)
	HLADoneButton.SetText(11973)
	HLADoneButton.SetFlags(IE_GUI_BUTTON_DEFAULT, OP_OR)

	# setup our text area
	HLATextArea = HLAWindow.GetControl(26)

	print "Number of HLAs:",len (HLAAbilities)

	# create a scrollbar if need-be
	if ( len (HLAAbilities) >= 25 ) and EnhanceGUI:
		# setup extra 25th HLA slot:
		HLAWindow.CreateButton (24, 231, 345, 42, 42)
		if ( len (HLAAbilities) > 25):
			# setup our scroll index
			GemRB.SetVar("HLATopIndex", 0)
			# setup scrollbar
			HLAWindow.CreateScrollBar (1000, 290,142, 16,252)
			ScrollBar = HLAWindow.GetControl (1000)
			ScrollBar.SetSprites ("GUISCRCW", 0, 0,1,2,3,5,4)
			ScrollBar.SetEvent (IE_GUI_SCROLLBAR_ON_CHANGE, HLAShowAbilities)
			#with enhanced GUI we have 5 rows of 5 abilities (the last one is 'the extra slot')
			ScrollBar.SetVarAssoc ("HLATopIndex", GUICommon.ceildiv ( ( len (HLAAbilities)-25 ) , 5 ) + 1 )
			ScrollBar.SetDefaultScrollBar ()

	# draw our HLAs and show the window
	HLAShowAbilities ()
	HLAWindow.ShowModal (MODAL_SHADOW_GRAY)

	return

def HLADonePress ():
	"""Saves the new HLAs and closes the HLA selection window."""

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
			SpellIndex = Spellbook.HasSpell (pc, HLAType, HLALevel, HLARef[3:])
			if SpellIndex < 0: # gotta learn it
				GemRB.LearnSpell (pc, HLARef[3:], LS_MEMO)
			else: # memorize it again
				GemRB.MemorizeSpell (pc, HLAType, HLALevel, SpellIndex)

		#save the number of this HLA memorized
		#TODO: check param2 (0 seems to work ok)
		GemRB.ApplyEffect(pc, "HLA", HLAAbilities[i][2], 0, HLARef[3:])

	# close the window
	if HLAWindow:
		HLAWindow.Unload ()

	# so redraw skills knows we're done
	GemRB.SetVar ("HLACount", 0)

	return

def HLAShowAbilities ():
	"""Updates the HLA selections window.

	Called whenever an HLA is pressed."""

	j = ( GemRB.GetVar("HLATopIndex") + 1 ) * 5 - 5

	# we have a grid of 24 abilites
	for i in range (24+EnhanceGUI):
		# ensure we can learn this many abilites
		if len (HLAAbilities) < 25 and i == 24: #break if we don't need extra 25th button
			break
		SpellButton = HLAWindow.GetControl (i)
		if i + j >= len (HLAAbilities):
			SpellButton.SetState (IE_GUI_BUTTON_DISABLED)
			SpellButton.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_SET)
			continue
		else:
			SpellButton.SetState(IE_GUI_BUTTON_ENABLED)
			SpellButton.SetFlags(IE_GUI_BUTTON_NO_IMAGE, OP_NAND)

		# fill in the button with the spell data
		HLARef = HLAAbilities[i+j][0][3:]
		if not HLARef:
			continue
		Spell = GemRB.GetSpell (HLARef)
		SpellButton.SetTooltip(Spell['SpellName'])
		SpellButton.SetSpellIcon(HLARef, 1)
		SpellButton.SetVarAssoc("ButtonPressed", i)
		SpellButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, HLASelectPress)
		SpellButton.SetSprites("GUIBTBUT", 0,0,1,2,3)
		SpellButton.SetFlags(IE_GUI_BUTTON_PICTURE, OP_OR)

		# don't allow the selection of an un-learnable ability
		if HLAAbilities[i+j][1] == 0:
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
	HLATextArea.SetText(63817)

	# show the points left
	PointsLeftLabel = HLAWindow.GetControl (0x10000018)
	PointsLeftLabel.SetText (str (HLACount))

	return

def HLASelectPress ():
	"""Toggles the HLA and displays a description string."""

	global HLACount, HLAAbilities, HLANewAbilities

	# get our variables
	j = ( GemRB.GetVar("HLATopIndex") + 1 ) * 5 - 5
	i = GemRB.GetVar ("ButtonPressed") + j

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

def HLAShowSelectedAbilities ():
	"""Marks all of the selected abilities."""

	j = ( GemRB.GetVar("HLATopIndex") + 1 ) * 5 - 5

	# mark all of the abilities picked thus far
	for i in range (24+EnhanceGUI):
		if i + j >= len (HLANewAbilities): # make sure we don't call unavailable indexes
			break
		if HLANewAbilities[i+j]:
			HLAMarkButton (i+j, 1)
		else:
			HLAMarkButton (i+j, 0)

	return

def HLAMarkButton (i, select):
	"""Enables, disables, or highlights the given button.

	If select is true, the button is highlighted."""

	j = ( GemRB.GetVar("HLATopIndex") + 1 ) * 5 - 5

	if select:
		type = IE_GUI_BUTTON_SELECTED
	else:
		if HLAAbilities[i][1]:
			type = IE_GUI_BUTTON_ENABLED
		else: # can't learn
			type = IE_GUI_BUTTON_LOCKED

	# we have to use the index on the actual grid
	SpellButton = HLAWindow.GetControl(i-j)
	SpellButton.SetState(type)
	return

def GetHLAs ():
	"""Updates HLAAbilites with all the choosable class HLAs.

	HLAAbilities[x][0] is the given HLAs spell reference.
	HLAAbilities[x][1] is true if the HLAs prerequisites have been met."""

	global HLAAbilities, HLANewAbilities, HLACount

	# get some needed values
	Kit = GUICommon.GetKitIndex (pc)
	IsDual = GUICommon.IsDualClassed (pc, 0)
	IsDual = IsDual[0] > 0
	MaxHLACount = 0

	# reset the abilities
	HLAAbilities = []
	HLANewAbilities = []

	# the HLA table lookup table
	HLAAbbrTable = GemRB.LoadTable ("luabbr")

	# get all the HLAs for each class
	for i in range (NumClasses):
		ClassName = GUICommon.GetClassRowName (Classes[i], "class")
		CurrentLevel = Level[i]

		if Kit != 0 and NumClasses == 1 and not IsDual: # kitted single-class
			KitName = CommonTables.KitList.GetValue (Kit, 0)
			HLAClassTable = "lu" + HLAAbbrTable.GetValue (KitName, "ABBREV")
			ClassName = KitName
		else: # everyone else
			HLAClassTable = "lu" + HLAAbbrTable.GetValue (ClassName, "ABBREV")

		# actually load the table
		HLAClassTable = GemRB.LoadTable (HLAClassTable)
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
				MaxHLACount += 1
				SaveArray[1] = 1
				HLAAbilities.append (SaveArray)
				continue

			# we didn't meet prereqs :(
			print "\t\tNeed pre-req:",SaveArray[3]
			HLAAbilities.append (SaveArray)

	# create an array to store our abilities as they are selected
	HLANewAbilities = [0]*len (HLAAbilities)

	#make sure we don't get stuck with HLAs we can't apply
	if MaxHLACount < HLACount:
		HLACount = MaxHLACount
		GemRB.SetVar ("HLACount", HLACount)

	return

def HLARecheckPrereqs (index):
	"""Rechecks the HLA prequisites on the index on the fly."""

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


