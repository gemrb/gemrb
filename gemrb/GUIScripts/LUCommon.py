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
from BGCommon import ClassTable, NextLevelTable, IsMultiClassed, IsDualClassed, IsDualSwap
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
