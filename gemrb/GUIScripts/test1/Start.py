import GemRB

def OnLoad():
	print "Testing basic guiscript functions"
	#case insensitive on token key, but case sensitive on value
	print "1. Testing Set/GetToken"
	GemRB.SetToken("CHARname","Avenger")
	print "Avenger == ",GemRB.GetToken("charNAME")
	#case insensitive on key
	print "2. Testing SetVar & GetVar"
	GemRB.SetVar("Strref On",1)
	print "Strref On was turned to ",GemRB.GetVar("STRREF ON")
	#actually this breaks the script atm
	#print "3. Testing PlaySound - non-existent resource"
	#GemRB.PlaySound("nothere")
	print "4. Testing PlaySound, uncompressed wav"
	GemRB.PlaySound("alivad12")
	print "5. Testing PlaySound, compressed wav"
	GemRB.PlaySound("compress")
	print "6. Enabling cheatkeys"
	GemRB.EnableCheatKeys(1)
	#should quit without error :)
	print "99. quit"
	GemRB.Quit()
