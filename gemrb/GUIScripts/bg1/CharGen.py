# SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

#character generation (GUICG 0)

import CharGenCommon
import CharGenGui

def init():
	print("init")
	# (name, control, text)
	# (name, script | setFn, commentFn, unsetFn, guard)
	stages = [
		("gender", 0, 11956),
		("Show name, create player", None, CharGenGui.getName, CharGenGui.unsetPlayer, CharGenGui.setPlayer),
		("setGender", "GUICG1", CharGenGui.getGender, CharGenGui.unsetGender, None), #BG2: same
		("setPortrait", "GUIPortraitCommon", None, CharGenGui.unsetPortrait, None), #BG2: different setPicture, PortraitName and extra unused control (TextAreaControl)
		("race", 1, 11957),
		("setRace", "GUICG8", CharGenGui.getRace, CharGenGui.unsetRace, None), #BG2: different resources
		("class", 2, 11959),
		("setClass", "GUICG2", CharGenGui.getClass, CharGenGui.unsetClass, None), #BG2: different resources,other setLogic, has levels/XP
		("setMultiClass", "GUICG10", None, CharGenGui.unsetClass, CharGenGui.guardMultiClass), #BG2: not same?
		("setSpecialist", "GUICG22", None, CharGenGui.unsetClass, CharGenGui.guardSpecialist), #BG2: not same
		("alignment", 3, 11958),
		("setAlignment", "GUICG3", CharGenGui.getAlignment, CharGenGui.unsetAlignment, None), #bg2: BG1 doesn't take kits into account
		("abilities", 4, 11960),
		("setAbilities"	, "GUICG4"	, CharGenGui.getAbilities, CharGenGui.unsetAbilities, None), #bg2: BG2 has extra strength, overpress (functionality + resources), kits
		("skill", 5, 17372),
		("divine spells", None, CharGenGui.getDivineSpells, CharGenGui.unsetDivineSpells, CharGenGui.setDivineSpells),
		("hate race", "GUICG15", CharGenGui.getHatedRace, CharGenGui.unsetHateRace, CharGenGui.guardHateRace), #BG2: other listsize, button offset, BG2 has TopIndex
		("mage spells", "GUICG7", CharGenGui.getMageSpells, CharGenGui.unsetMageSpells, CharGenGui.guardMageSpells),   #also sets priest in CharGen6, we do it at the end, other animations for buttons
		("setSkill"	, "GUICG6", CharGenGui.getSkills, CharGenGui.unsetSkill, CharGenGui.guardSkills),
		("proficiencies", "GUICG9", CharGenGui.getProfi, CharGenGui.unsetProfi, None),
		("appearance", 6, 11961),
		("colors", "GUICG13", None, CharGenGui.unsetColors, None), #bg2: other cycleids (SetBAM)
		("sounds", "GUICG19", None, CharGenGui.unsetSounds, None), #BG2: other: first male sound, logic
		("name", 7, 11963),
		("setName", "GUICG5", None, CharGenGui.unsetName, None), #BG2: same	
		("accept", 8, 11962),
		("finish", CharGenGui.setAccept, None, None, None)
	]
	
	CharGenCommon.CharGenMaster = CharGenCommon.CharGen(stages,16575,CharGenGui.Imprt)

	return

def OnLoad():
	if(CharGenCommon.CharGenMaster == None):
		init()
	CharGenCommon.CharGenMaster.displayOverview()
	return
