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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
#
#
#character generation (GUICG 0)
import GemRB

import CharGenCommon
from CharGenGui import * 

def init():
	print("init")
		#(name,control,text)
		#(name,script | setFn,commentFn,unsetFn,gaurd)
	stages = [
	("gender"	 , 0		, 11956		),
	("Show name, create player" , None    ,getName, unsetPlayer,  setPlayer ),
	("setGender"	, "GUICG1"	, getGender	,unsetGender		, None		), #BG2: same
	("setPortrait"	, "GUICG12"	, None		,unsetPortrait		, None		), #BG2: different setPicture, PortraitName and extra unused control (TextAreaControl)
	("race"		, 1		,11957		),
	("setRace"	, "GUICG8"	, getRace	,unsetRace 		, None		), #BG2: different resources
	("class"	, 2		,11959		),
	("setClass"	, "GUICG2"	, getClass 	,unsetClass		, None 		), #BG2: different resources,other setLogic, has levels/XP
	("setMultiClass", "GUICG10"	, None		,unsetClass	, guardMultiClass), #BG2: not same?
	("setSpecialist", "GUICG22"	, None		,unsetClass	, guardSpecialist), #BG2: not same
	("alignment"	, 3		,11958		),
	("setAlignment"	, "GUICG3"	, getAlignment	,unsetAlignment		, None		), #bg2: BG1 doesn't take kits into account
	("abilities"	, 4		,11960		),
	("setAbilities"	, "GUICG4"	, getAbilities	,unsetAbilities		, None		), #bg2: BG2 has extra strength, overpress (functionality + resources), kits
	("skill"	, 5		,17372		),
	("divine spells" , None , getDivineSpells,unsetDivineSpells,setDivineSpells ),
	("hate race"	, "GUICG15"	, getHatedRace	,unsetHateRace		, guardHateRace), #BG2: other listsize, button offset, BG2 has TopIndex
	("mage spells"	, "GUICG7"	, getMageSpells,unsetMageSpells		, guardMageSpells),   #also sets priest in CharGen6, we do it at the end, other animations for buttons
	("setSkill"	, "GUICG6"	, getSkills		,unsetSkill		, guardSkills	),
	("proficiencies", "GUICG9"	, getProfi		,unsetProfi		, None		),
	("appearance"	, 6		,11961		),
	("colors"	, "GUICG13"	, None		,unsetColors	 	, None		), #bg2: other cycleids (SetBAM)
	("sounds"	, "GUICG19"	, None		,unsetSounds		, None		), #BG2: other: first male sound, logic
	("name"		, 7 		,11963		),
	("setName"	 ,"GUICG5"	, None 		,unsetName		, None 		), #BG2: same	
	("accept"	, 8		,11962		),
	("finish"	, setAccept	, None 		, None			, None )]
	
	CharGenCommon.CharGenMaster = CharGenCommon.CharGen(stages,16575,Imprt)

	return

def OnLoad():
	if(CharGenCommon.CharGenMaster == None):
		init()
	CharGenCommon.CharGenMaster.displayOverview()
	return
