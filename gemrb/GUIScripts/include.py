# this file is executed at gemrb startup, for the Console
from ie_stats import *

# /handy/ shorthand forms
def gps(stat, base=0):
	print GemRB.GetPlayerStat(GemRB.GameGetFirstSelectedPC(), stat, base)

def sps(stat, value, pcf=1):
	GemRB.SetPlayerStat(GemRB.GameGetFirstSelectedPC(), stat, value, pcf)
