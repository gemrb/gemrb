# this file is executed at gemrb startup, for the Console
from ie_stats import *

# /handy/ shorthand forms
def gps(stat, base=0):
	print GemRB.GetPlayerStat(GemRB.GameGetFirstSelectedPC(), stat, base)

def sps(stat, value, pcf=1):
	GemRB.SetPlayerStat(GemRB.GameGetFirstSelectedPC(), stat, value, pcf)

def mta(area):
	GemRB.MoveToArea(area)

def cc(cre, px=-1, py=-1):
	GemRB.CreateCreature(GemRB.GameGetFirstSelectedPC(), cre, px, py)

def ci(item, slot=-1, c0=1, c1=0, c2=0):
	GemRB.CreateItem(GemRB.GameGetFirstSelectedPC(), item, slot, c0, c1, c2)

def cv(var, context="GLOBAL"):
	GemRB.CheckVar(var, context)
