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
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#
# $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/GUIScripts/GUICommon.py,v 1.5 2004/12/09 22:08:04 avenger_teambg Exp $

import GemRB
# GUICommon.py - common functions for GUIScripts of all game types

OtherWindowFn = None
#global OtherWindowFn

def CloseOtherWindow (NewWindowFn):
	global OtherWindowFn

	if OtherWindowFn and OtherWindowFn != NewWindowFn:
		OtherWindowFn ()
		OtherWindowFn = NewWindowFn
		return 0
	elif OtherWindowFn:
		OtherWindowFn = None
		return 1
	else:
		OtherWindowFn = NewWindowFn
		return 0

def GetLearnableMageSpells (Kit, Alignment, Level):
	Learnable =[]

	Table=GemRB.LoadTable("aligns")
	v = GemRB.FindTableValue(Table, 3, Alignment)
	print "%x=%x"%(Alignment,v)
	#usability is the bitset we look for
	Usability=Kit | GemRB.GetTableValue(Table, v, 5)

	print "Alignment: %x, Kit:%x  Usability %x"%(Alignment, Kit, Usability)
	for i in range(100):
		SpellName = "SPWI%d%02d"%(Level,i)
		ms = GemRB.GetSpell(SpellName)
		if ms == None:
			continue
		if Usability & ms['SpellSchool']:
			continue
		Learnable.append (SpellName)
	return Learnable

def GetLearnablePriestSpells (Class, Alignment, Level):
	Learnable =[]

	Table=GemRB.LoadTable("aligns")
	v = GemRB.FindTableValue(Table, 3, Alignment)
	#usability is the bitset we look for
	Usability=GemRB.GetTableValue(Table, v, 5)

	for i in range(100):
		SpellName = "SPPR%d%02d"%(Level,i)
		ms = GemRB.GetSpell(SpellName)
		if ms == None:
			continue
		if Class & ms['SpellDivine']:
			continue
		if Usability & ms['SpellSchool']:
			continue
		Learnable.append (SpellName)
	return Learnable

