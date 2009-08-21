# -*-python-*-
# GemRB - Infinity Engine Emulator
# Copyright (C) 2009 The GemRB Project
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
# $Id:$
#
# LUCommon.py - common functions related to leveling up

import GemRB
from GUICommon import *
from ie_stats import *

def GetNextLevelExp (Level, Class):
	"""Returns the amount of XP required to gain the next level."""
	Row = NextLevelTable.GetRowIndex (Class)
	if Level < NextLevelTable.GetColumnCount (Row):
		return str (NextLevelTable.GetValue (Row, Level) )

	return 0

def CanLevelUp(actor):
	"""Returns true if the actor can level up."""

	# get our class and placements for Multi'd and Dual'd characters
	Class = GemRB.GetPlayerStat (actor, IE_CLASS)
	Class = ClassTable.FindValue (5, Class)
	Class = ClassTable.GetRowName (Class)
	Multi = IsMultiClassed (actor, 1)
	Dual = IsDualClassed (actor, 1)

	# get all the levels and overall xp here
	xp = GemRB.GetPlayerStat (actor, IE_XP)
	Levels = [GemRB.GetPlayerStat (actor, IE_LEVEL), GemRB.GetPlayerStat (actor, IE_LEVEL2),\
		GemRB.GetPlayerStat (actor, IE_LEVEL3)]

	#TODO: double-check this
	if GemRB.GetPlayerStat(actor, IE_LEVELDRAIN)>0:
		return 0

	if Multi[0] > 1: # multiclassed
		xp = xp/Multi[0] # divide the xp evenly between the classes
		for i in range (Multi[0]):
			# if any class can level, return 1
			ClassIndex = ClassTable.FindValue (5, Multi[i+1])
			tmpNext = int(GetNextLevelExp (Levels[i], ClassTable.GetRowName (ClassIndex) ) )
			if tmpNext != 0 and tmpNext <= xp:
				return 1

		# didn't find a class that could level
		return 0
	elif Dual[0] > 0: # dual classed
		# get the class we can level
		Class = ClassTable.GetRowName (Dual[2])
		if IsDualSwap(actor):
			Levels = [Levels[1], Levels[0], Levels[2]]

	# check the class that can be level (single or dual)
	tmpNext = int(GetNextLevelExp (Levels[0], Class) )
	return (tmpNext != 0 and tmpNext <= xp)

def SetupSavingThrows (pc, Level=None):
	"""Updates an actors saving throws based upon level.

	Level should contain the actors current level.
	If Level is None, it is filled with the actors current level."""

	#storing levels as an array makes them easier to deal with
	if not Level:
		Levels = [GemRB.GetPlayerStat (pc, IE_LEVEL)-1, \
			GemRB.GetPlayerStat (pc, IE_LEVEL2)-1, \
			GemRB.GetPlayerStat (pc, IE_LEVEL3)-1]
	else:
		Levels = []
		for level in Level:
			Levels.append (level-1)

	#get some basic values
	Class = [GemRB.GetPlayerStat (pc, IE_CLASS)]
	Race = GemRB.GetPlayerStat (pc, IE_RACE)

	#adjust the class for multi/dual chars
	Multi = IsMultiClassed (pc, 1)
	Dual = IsDualClassed (pc, 1)
	NumClasses = 1
	if Multi[0]>1: #get each of the multi-classes
		NumClasses = Multi[0]
		Class = [Multi[1], Multi[2], Multi[3]]
	elif Dual[0]: #only worry about the newer class
		Class = [ClassTable.GetValue (Dual[2], 5)]
		#assume Level is correct if passed
		if IsDualSwap(pc) and not Level:
			Levels = [Levels[1], Levels[0], Levels[2]]
	if NumClasses>len(Levels):
		return

	#see if we can add racial bonuses to saves
	#default return is -1 NOT "*", so we convert always convert to str
	#I'm leaving the "*" just in case
	Race = RaceTable.GetRowName (RaceTable.FindValue (3, Race) )
	RaceSaveTableName = str(RaceTable.GetValue (Race, "SAVE") )
	RaceSaveTable = None
	if RaceSaveTableName != "-1" and RaceSaveTableName != "*":
		Con = GemRB.GetPlayerStat (pc, IE_CON, 1)-1
		RaceSaveTable = GemRB.LoadTableObject (RaceSaveTableName)
		if Con >= RaceSaveTable.GetRowCount ():
			Con = RaceSaveTable.GetRowCount ()-1

	#preload our tables to limit multi-classed lookups
	SaveTables = []
	for i in range (NumClasses):
		SaveName = ClassTable.GetValue (ClassTable.FindValue (5, Class[i]), 3, 0)
		SaveTables.append (GemRB.LoadTableObject (SaveName) )
	if not len (SaveTables):
		return

	#make sure to limit the levels to the table allowable
	MaxLevel = SaveTables[0].GetColumnCount ()-1
	for i in range (len(Levels)):
		if Levels[i] > MaxLevel:
			Levels[i] = MaxLevel

	#save the saves
	for row in range (5):
		CurrentSave = GemRB.GetPlayerStat(pc, IE_SAVEVSDEATH+i, 1)
		for i in range (NumClasses):
			#loop through each class and update the save value if we have
			#a better save
			TmpSave = SaveTables[i].GetValue (row, Levels[i])
			if TmpSave and (TmpSave < CurrentSave or i == 0):
				CurrentSave = TmpSave

		#add racial bonuses if applicable (small pc's)
		if RaceSaveTable:
			CurrentSave += RaceSaveTable.GetValue (row, Con)
		GemRB.SetPlayerStat (pc, IE_SAVEVSDEATH+row, CurrentSave)
	return
