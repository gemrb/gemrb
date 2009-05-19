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
from GUICommon import GetMageSpells, HasSpell
from GUIREC import GetKitIndex

# storage variables
pc = 0

# sorcerer spell selection
SpellsWindow = 0		# << spell selection window
SpellKnownTable = 0		# << known spells per level (table)
DoneButton = 0			# << done/next button
SpellsTextArea = 0		# << spell description area
SpellsSelectPointsLeft = [0]*9	# << spell selections left per level
Spells = [0]*9			# << spells learnable per level
SpellTopIndex = 0		# << scroll bar index
SpellBook = []			# << array containing all the spell indexes to learn
SpellLevel = 0			# << current level of spells

def OpenSpellsWindow (actor, table, level, diff):
	global SpellsWindow, DoneButton, SpellsSelectPointsLeft, Spells
	global SpellsTextArea, SpellsKnownTable, SpellTopIndex, SpellBook, SpellLevel, pc

	# save our pc
	pc = actor

	# FIXME: this currently only works for sorcerers, but i'd like to convert it to the
	# GUICG version and make it compatible with the CharGen code so there is less repeated
	# code throughout; that's why this code is fairly broad for just sorc leveling
	Kit = GetKitIndex (pc)
	if Kit == 0:
		KitMask = 0x4000
	else: # need to implement this if converted to CharGen
		KitMask = 0x4000

	# load our window
	SpellsWindow = GemRB.LoadWindowObject (8)

	# setup our variables
	GemRB.SetVar ("SpellTopIndex", 0)

	# the done button also doubles as a next button
	DoneButton = SpellsWindow.GetControl (28)
	DoneButton.SetState(IE_GUI_BUTTON_DISABLED)
	DoneButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "SpellsDonePress")
	DoneButton.SetText(11973)
	DoneButton.SetFlags(IE_GUI_BUTTON_DEFAULT, OP_OR)

	# create our scroll bar
	SpellsWindow.CreateScrollBar (1000, 290,142, 16,252)

	# setup our text area (spell description)
	SpellsTextArea = SpellsWindow.GetControl(26)

	# get all our spell info
	SpellsKnownTable = GemRB.LoadTableObject (table)
	AlreadyShown = 0
	for i in range (9):
		# make sure we get more spells of each class before continuing
		SpellsSelectPointsLeft[i] = SpellsKnownTable.GetValue (level-1, i) - SpellsKnownTable.GetValue (level-diff-1, i)
		if SpellsSelectPointsLeft[i] <= 0:
			continue

		# get all the spells of the given level
		Spells[i] = GetMageSpells (KitMask, GemRB.GetPlayerStat (pc, IE_ALIGNMENT), i+1)

		# dump all the spells we already know
		NumDeleted = 0
		for j in range (len (Spells[i])):
			CurrentIndex = j - NumDeleted # this ensure we don't go out of range
			if HasSpell (pc, IE_SPELL_TYPE_WIZARD, i, Spells[i][CurrentIndex][0]) >= 0:
				del Spells[i][CurrentIndex]
				NumDeleted += 1

		# display these spells if it's the first non-zero level
		if AlreadyShown == 0:
			# save the level and spellbook data
			SpellLevel = i
			SpellBook = [0]*len(Spells[i])

			# setup the scrollbar
			ScrollBar = SpellsWindow.GetControl (1000)
			ScrollBar.SetSprites ("GUISCRCW", 0, 0,1,2,3,5,4)
			ScrollBar.SetEvent (IE_GUI_SCROLLBAR_ON_CHANGE, "ShowSpells")
			ScrollBar.SetDefaultScrollBar ()

			# only scroll if we have more than 24 spells
			if len (Spells[i]) > 24:
				ScrollBar.SetVarAssoc ("SpellTopIndex", len (Spells[i])-24)
			else:
				ScrollBar.SetVarAssoc ("SpellTopIndex", 0)

			# show our spells
			ShowSpells ()
			AlreadyShown = 1

	# show the selection window
	SpellsWindow.ShowModal (MODAL_SHADOW_GRAY)

	return

# save all the current spells and move to the next level if required;
# otherwise, close the window and update our records
def SpellsDonePress ():
	global SpellBook, SpellLevel

	# save all the spells
	for i in range (len (Spells[SpellLevel])):
		if SpellBook[i]: # we need to learn this spell
			GemRB.LearnSpell (pc, Spells[SpellLevel][i][0])

	# check to see if we need to update again
	for i in range (SpellLevel+1, 9):
		if SpellsSelectPointsLeft[i] > 0:
			# reset the variables
			GemRB.SetVar ("SpellTopIndex", 0)
			SpellLevel = i
			SpellBook = [0]*len(Spells[i])

			# setup the scrollbar
			ScrollBar = SpellsWindow.GetControl (1000)
			if len (Spells[i]) > 24:
				ScrollBar.SetVarAssoc ("SpellTopIndex", len (Spells[i])-24)
			else:
				ScrollBar.SetVarAssoc ("SpellTopIndex", 0)

			# show the spells and set the done button to off
			ShowSpells ()
			DoneButton.SetState (IE_GUI_BUTTON_DISABLED)
			return

	# close our window and update our records
	if SpellsWindow:
		SpellsWindow.Unload ()

	return

# shows 24 spells on the grid and scrolls with a scrollbar to allow more spells
def ShowSpells ():
	SpellTopIndex = GemRB.GetVar ("SpellTopIndex")

	# we have a grid of 24 spells
	for i in range (24):
		# ensure we can learn this many spells
		SpellButton = SpellsWindow.GetControl (i)
		if i >= len (Spells[SpellLevel]):
			SpellButton.SetState (IE_GUI_BUTTON_DISABLED)
			continue

		# fill in the button with the spell data
		Spell = GemRB.GetSpell (Spells[SpellLevel][i+SpellTopIndex][0])
		SpellButton.SetTooltip(Spell['SpellName'])
		SpellButton.SetSpellIcon(Spells[SpellLevel][i+SpellTopIndex][0], 1)
		SpellButton.SetVarAssoc("ButtonPressed", i)
		SpellButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "SpellsSelectPress")
		SpellButton.SetSprites("GUIBTBUT", 0,0,1,2,3)
		SpellButton.SetFlags(IE_GUI_BUTTON_PICTURE, OP_OR)

		# don't allow the selection of an un-learnable spell
		if Spells[SpellLevel][i+SpellTopIndex][1] == 0:
			SpellButton.SetState(IE_GUI_BUTTON_LOCKED)
			# shade red
			SpellButton.SetBorder (0, 0,0, 0,0, 200,0,0,100, 1,1)
		elif Spells[SpellLevel][i+SpellTopIndex][1] == 1: # learnable
			SpellButton.SetState (IE_GUI_BUTTON_ENABLED)
			# unset any borders on this button or an un-learnable from last level
			# will still shade red even though it is clickable
			SpellButton.SetBorder (0, 0,0, 0,0, 0,0,0,0, 0,0)
		else: # specialist (shouldn't get here)
			# use the green border state for matching specialist spells
			SpellButton.SetState (IE_GUI_BUTTON_THIRD)

	# show which spells are selected
	ShowSelectedSpells ()

	GemRB.SetToken("number", str(SpellsSelectPointsLeft[SpellLevel]))
	SpellsTextArea.SetText(17250)

	# show how many spell picks are left
	PointsLeftLabel = SpellsWindow.GetControl (0x10000018)
	PointsLeftLabel.SetText (str (SpellsSelectPointsLeft[SpellLevel]))

	return

# called when a spell is pressed -- either selects or unselects the spell, if we're
# able to learn it, and displays the spell information in the text window
def SpellsSelectPress ():
	global SpellsSelectPointsLeft, Spells, SpellBook

	# get our variables
	SpellTopIndex = GemRB.GetVar ("SpellTopIndex")
	i = GemRB.GetVar ("ButtonPressed") + SpellTopIndex

	# get the spell that's been pushed
	Spell = GemRB.GetSpell (Spells[SpellLevel][i][0])
	SpellsTextArea.SetText (Spell["SpellDesc"])

	# make sure we can learn the spell
	if Spells[SpellLevel][i][1]:
		if SpellBook[i]: # already picked -- unselecting
			SpellsSelectPointsLeft[SpellLevel] = SpellsSelectPointsLeft[SpellLevel] + 1
			SpellBook[i] = 0
			DoneButton.SetState (IE_GUI_BUTTON_DISABLED)
		else: # selecting
			# we don't have any picks left
			if SpellsSelectPointsLeft[SpellLevel] == 0:
				MarkButton (i, 0)
				return

			# select the spell and change the done state if need be
			SpellsSelectPointsLeft[SpellLevel] = SpellsSelectPointsLeft[SpellLevel] - 1
			SpellBook[i] = 1
			if SpellsSelectPointsLeft[SpellLevel] == 0:
				DoneButton.SetState (IE_GUI_BUTTON_ENABLED)

	# show selected spells
	ShowSelectedSpells ()

	# show how many spell picks are left
	PointsLeftLabel = SpellsWindow.GetControl (0x10000018)
	PointsLeftLabel.SetText (str (SpellsSelectPointsLeft[SpellLevel]))
	return

# marks the button as selected, enabled, locked, or specialist
# make sure i is sent with SpellTopIndex added!
def MarkButton (i, select):
	SpellTopIndex = GemRB.GetVar ("SpellTopIndex")

	if select:
		type = IE_GUI_BUTTON_SELECTED
	else:
		if Spells[SpellLevel][i][1] == 1:
			type = IE_GUI_BUTTON_ENABLED
		elif Spells[SpellLevel][i][1] == 2:
			# specialist spell
			type = IE_GUI_BUTTON_THIRD
		else: # can't learn
			type = IE_GUI_BUTTON_LOCKED

	# we have to use the index on the actual grid
	SpellButton = SpellsWindow.GetControl(i-SpellTopIndex)
	SpellButton.SetState(type)
	return

# marks all selected spells
def ShowSelectedSpells ():
	SpellTopIndex = GemRB.GetVar ("SpellTopIndex")

	# mark all of the spells picked thus far
	for j in range (24):
		if j >= len (SpellBook): # make sure we don't call unavailable indexes
			break
		if SpellBook[j+SpellTopIndex]: # selected
			MarkButton (j+SpellTopIndex, 1)
		else: # not selected
			MarkButton (j+SpellTopIndex, 0)

	return
