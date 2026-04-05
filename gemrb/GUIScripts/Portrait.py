# SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later



# Portrait.py - scripts to control portrait selection and scrolling
###################################################

import GemRB
import GameCheck
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
	if GameCheck.IsBG2OrEE ():
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
