# -*-python-*-
# GemRB - Infinity Engine Emulator
# Copyright (C) 2003 The GemRB Project
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



# Portrait.py - scripts to control portrait selection and scrolling
###################################################

import GemRB
import GameCheck
import GUICommon
###################################################

PortraitCount = 0
PortraitsTable = None
Gender = None

# initializes gender and portrait table
# PortraitGender: 1 Male
#				  2 Female
def Init (PortraitGender):
	global PortraitsTable, PortraitCount, Gender

	if PortraitsTable is None:
		PortraitsTable = GemRB.LoadTable ("PICTURES")

	PortraitCount = 0
	Gender = PortraitGender

# sets index to given protraitname
def Set (PortraitName):
	global PortraitCount

	# removes the size marking character at the end
	if GameCheck.IsBG2():
		PortraitName = PortraitName.rstrip ("[ms]")
	else:
		PortraitName = PortraitName.rstrip ("[ls]")
	
	# capitalize PortraitName
	PortraitName = PortraitName.upper ()

	# search table
	for i in range(0, PortraitsTable.GetRowCount ()):
		if PortraitName == PortraitsTable.GetRowName (i).upper ():
			PortraitCount = i
			break

	return

# returns next portrait name
def Next ():
	global PortraitCount

	while True:
		PortraitCount = PortraitCount + 1
		if PortraitCount == PortraitsTable.GetRowCount ():
			PortraitCount = 0
		if PortraitsTable.GetValue (PortraitCount, 0) == Gender:
			return Name ()

# return previous portrait name
def Previous ():
	global PortraitCount

	while True:
		PortraitCount = PortraitCount - 1
		if PortraitCount < 0:
			PortraitCount = PortraitsTable.GetRowCount () - 1
		if PortraitsTable.GetValue (PortraitCount, 0) == Gender:
			return Name ()

# gets current portrait name
def Name ():
	global PortraitCount

	# if portrait matches not current gender, it will be skipped to
	# the next portrait that matches
	while PortraitsTable.GetValue (PortraitCount, 0) != Gender:
		PortraitCount = PortraitCount + 1

	PortraitName = PortraitsTable.GetRowName (PortraitCount) 
	return PortraitName
