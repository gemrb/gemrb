import GemRB

def OnLoad():
	print "Testing basic guiscript functions"
	print "1. Testing SetToken"
	GemRB.SetToken("CHARname","Avenger")
	print "2. Testing GetToken"
	print "Avenger == ",GemRB.GetToken("charNAME")
	print "3. Testing SetVariable & GetVariable"
	GemRB.SetVariable("Strref On",1)
	print "Strref On was turned to ",GemRB.GetVariable("Strref On")


	print "99. quit"
	GemRB.Quit()
