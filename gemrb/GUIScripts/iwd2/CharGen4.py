# SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

#character generation (GUICG 0)
import GemRB
import CharOverview
import IDLUCommon
import CommonTables
from ie_stats import *

#this is the same list as in GUIREC
#barbarian, bard, cleric, druid, fighter, monk, paladin, ranger, rogue, sorcerer, wizard
Classes = IDLUCommon.Levels

def OnLoad():
	MyChar = GemRB.GetVar ("Slot")
	#base class
	Class=GemRB.GetVar ("BaseClass")
	GemRB.SetPlayerStat (MyChar, IE_CLASS, Class)
	#kit
	Kit = GemRB.GetVar ("Class")
	if Kit == Class:
		Kit = 0
	else:
		KitName = CommonTables.Classes.GetRowName (Kit - 1)
		Kit = CommonTables.Classes.GetValue (KitName, "CLASSID")
	GemRB.SetPlayerStat (MyChar, IE_KIT, Kit)

	#works only for the first level character generation
	#if this code ever needs to be more versatile, consider saving the
	#class values somewhere
	for i in range(len(Classes)):
		GemRB.SetPlayerStat (MyChar, Classes[i], 0)

	GemRB.SetPlayerStat (MyChar, Classes[Class-1], 1)
	CharOverview.UpdateOverview(4)
	return
