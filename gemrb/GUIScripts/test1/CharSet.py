def OnLoad():
	GemRB.LoadWindowPack ("testing")
	Window = GemRB.LoadWindow (0)
	start = 64
	end = 255
	for i in range(end-start):
		x = i & 15
		y = i >> 4
		GemRB.CreateButton (Window, i, x*40+10, y*40+10, 40, 40)
		Button = GemRB.GetControl (Window, i)
		GemRB.SetText (Window, i, chr(i+start) )
		#GemRB.SetButtonSprites (Window, i, "NORMAL",i+start,0,0,0,0)
		GemRB.SetTooltip (Window, i, str(i+start) )
	GemRB.SetVisible (Window, 1)
