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
# $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/GUIScripts/GUICommon.py,v 1.2 2004/12/05 09:50:06 avenger_teambg Exp $


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
	for i in range(99):
		SpellName = "SPWI%d%02d"%(Level,i)
		ms = GemRB.GetSpell(SpellName)
		if ms == None:
			continue
		Learnable.append (ms['SpellResRef'])
	return Learnable

def GetLearnablePriestSpells (Kit, Alignment, Level):
	Learnable =[]
	for i in range(99):
		SpellName = "SPPR%d%02d"%(Level,i)
		ms = GemRB.GetSpell(SpellName)
		if ms == None:
			continue
		Learnable.append (ms['SpellResRef'])
	return Learnable

