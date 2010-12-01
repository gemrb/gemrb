# a library of any functions that are common to bg1 and bg2, but not useful for others
import GemRB
import CommonTables
from ie_stats import IE_RACE, IE_CLASS, IE_SEX

def RefreshPDoll(button, MinorColor, MajorColor, SkinColor, HairColor):
	MyChar = GemRB.GetVar ("Slot")
	AnimID = 0x6000
	table = GemRB.LoadTable ("avprefr")
	Race = GemRB.GetPlayerStat (MyChar, IE_RACE)
	AnimID = AnimID + table.GetValue(Race, 0)
	table = GemRB.LoadTable ("avprefc")
	Class = GemRB.GetPlayerStat (MyChar, IE_CLASS)
	AnimID = AnimID + table.GetValue (Class, 0)
	table = GemRB.LoadTable ("avprefg")
	Gender = GemRB.GetPlayerStat (MyChar, IE_SEX)
	AnimID = AnimID + table.GetValue (Gender, 0)
	ResRef = CommonTables.Pdolls.GetValue (hex(AnimID), "LEVEL1")

	button.SetPLT (ResRef, 0, MinorColor, MajorColor, SkinColor, 0, 0, HairColor, 0)
	return
