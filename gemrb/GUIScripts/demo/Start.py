import GemRB

def OnLoad():
	GemRB.LoadGame(None)

	# this is needed, so the game loop runs and the load happens
	# before other code (eg. CreatePlayer) depending on it is run
	GemRB.SetNextScript("SetupGame")

