#character generation (GUICG 0)
import GemRB
from CharOverview import *

#this is the same list as in GUIREC
#barbarian, bard, cleric, druid, fighter, monk, paladin, ranger, rogue, sorcerer, wizard
Classes = [IE_LEVELBARBARIAN, IE_LEVELBARD, IE_LEVELCLERIC, IE_LEVELDRUID, \
IE_LEVEL, IE_LEVELMONK, IE_LEVELPALADIN, IE_LEVELRANGER, IE_LEVEL3, \
IE_LEVELSORCEROR, IE_LEVEL2]

def OnLoad():
	MyChar = GemRB.GetVar ("Slot")
	#base class
	Class=GemRB.GetVar ("BaseClass")
	GemRB.SetPlayerStat (MyChar, IE_CLASS, Class)
	#kit
	GemRB.SetPlayerStat (MyChar, IE_KIT, GemRB.GetVar ("Class") )

	#works only for the first level character generation
	#if this code ever needs to be more versatile, consider saving the
	#class values somewhere
	for i in range(len(Classes)):
		GemRB.SetPlayerStat (MyChar, Classes[i], 0)

	GemRB.SetPlayerStat (MyChar, Classes[Class-1], 1)
	print "Set class stat ",Classes[Class-1], " to 1"
	UpdateOverview(4)
	return
