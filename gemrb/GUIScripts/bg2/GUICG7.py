#character generation, mage spells (GUICG7)
import GemRB

MageSpellsWindow = 0
TextAreaControl = 0
DoneButton = 0
MageSpellsTable = 0

def OnLoad():
	global MageSpellsWindow, TextAreaControl, DoneButton
	global MageSpellsTable
	
        ClassTable = GemRB.LoadTable("classes")
        ClassRow = GemRB.GetVar("Class")-1
        Class = GemRB.GetTableValue(ClassTable, ClassRow, 5)
        TmpTable = GemRB.LoadTable("clskills")
        TableName = GemRB.GetTableValue(TmpTable, Class, 2)
        if TableName == "*":
                GemRB.SetNextScript("GUICG6")
                return

	GemRB.LoadWindowPack("GUICG")
	MageSpellsWindow = GemRB.LoadWindow(7)
	MageSpellsTable = GemRB.LoadTable("MAGESP")
	MageSpellsCount = GemRB.GetTableRowCount(MageSpellsTable)
	GemRB.SetVar("MageSpellBook", 0)
	GemRB.SetVar("SpellMask", 0)

	MageSpellsSelectPointsLeft = 2
	PointsLeftLabel = GemRB.GetControl(MageSpellsWindow, 0x1000001b)
	GemRB.SetLabelUseRGB(MageSpellsWindow, PointsLeftLabel, 1)
	GemRB.SetText(MageSpellsWindow, PointsLeftLabel, str(MageSpellsSelectPointsLeft))
	for i in range (0, 24):
		SpellButton = GemRB.GetControl(MageSpellsWindow, i + 2)
		GemRB.SetButtonFlags(MageSpellsWindow, SpellButton, IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_CHECKBOX, OP_OR)
		if i < MageSpellsCount:
			# Color is no good yet :-(
			GemRB.SetButtonBAM(MageSpellsWindow, SpellButton, GemRB.GetTableValue(MageSpellsTable, i, 0), 0, 0, 63)
			GemRB.SetButtonState(MageSpellsWindow, SpellButton, IE_GUI_BUTTON_ENABLED)
			GemRB.SetEvent(MageSpellsWindow, SpellButton, IE_GUI_BUTTON_ON_PRESS, "MageSpellsSelectPress")
			GemRB.SetVarAssoc(MageSpellsWindow, SpellButton, "SpellMask", 1 << i)
		else:
			GemRB.SetButtonState(MageSpellsWindow, SpellButton, IE_GUI_BUTTON_DISABLED)

	GemRB.SetToken("number", str(MageSpellsSelectPointsLeft))
	MageSpellsTextArea = GemRB.GetControl(MageSpellsWindow, 27)
	GemRB.SetText(MageSpellsWindow, MageSpellsTextArea, 17250)

	MageSpellsDoneButton = GemRB.GetControl(MageSpellsWindow, 0)
	GemRB.SetButtonState(MageSpellsWindow, MageSpellsDoneButton, IE_GUI_BUTTON_DISABLED)
	GemRB.SetEvent(MageSpellsWindow, MageSpellsDoneButton, IE_GUI_BUTTON_ON_PRESS, "MageSpellsDonePress")
	GemRB.SetText(MageSpellsWindow, MageSpellsDoneButton, 11973)
	GemRB.SetButtonFlags(MageSpellsWindow, MageSpellsDoneButton, IE_GUI_BUTTON_DEFAULT, OP_OR)
	MageSpellsCancelButton = GemRB.GetControl(MageSpellsWindow, 29)
	GemRB.SetButtonState(MageSpellsWindow, MageSpellsCancelButton, IE_GUI_BUTTON_ENABLED)
	GemRB.SetEvent(MageSpellsWindow, MageSpellsCancelButton, IE_GUI_BUTTON_ON_PRESS, "MageSpellsCancelPress")
	GemRB.SetText(MageSpellsWindow, MageSpellsCancelButton, 13727)
	MageSpellsCancelButton = GemRB.GetControl(MageSpellsWindow, 30)
	GemRB.SetButtonState(MageSpellsWindow, MageSpellsCancelButton, IE_GUI_BUTTON_ENABLED)
	GemRB.SetEvent(MageSpellsWindow, MageSpellsCancelButton, IE_GUI_BUTTON_ON_PRESS, "MageSpellsDonePress")
	GemRB.SetText(MageSpellsWindow, MageSpellsCancelButton, 9372)

	GemRB.SetVisible(MageSpellsWindow,1)
	return

def MageSpellsSelectPress():
	global MageSpellsWindow, MageSpellsTable, MageSpellsTextArea, MageSpellsDoneButton, MageSpellsSelectPointsLeft
	MageSpellsCount = GemRB.GetTableRowCount(MageSpellsTable)
	MageSpellBook = GemRB.GetVar("MageSpellBook")
	SpellMask = GemRB.GetVar("SpellMask")
	Spell = abs(MageSpellBook - SpellMask)

	i = -1
	while (Spell > 0):
		i = i + 1
		Spell = Spell >> 1
	GemRB.SetText(MageSpellsWindow, MageSpellsTextArea, GemRB.GetTableValue(MageSpellsTable, i, 1))
	if SpellMask < MageSpellBook:
		MageSpellsSelectPointsLeft = MageSpellsSelectPointsLeft + 1
		for i in range (0, MageSpellsCount):
			SpellButton = GemRB.GetControl(MageSpellsWindow, i + 2)
			if (((1 << i) & SpellMask) == 0):
				GemRB.SetButtonState(MageSpellsWindow, SpellButton, IE_GUI_BUTTON_ENABLED)
		GemRB.SetButtonState(MageSpellsWindow, MageSpellsDoneButton, IE_GUI_BUTTON_DISABLED)
	else:
		MageSpellsSelectPointsLeft = MageSpellsSelectPointsLeft - 1
		if MageSpellsSelectPointsLeft == 0:
			for i in range (0, MageSpellsCount):
				SpellButton = GemRB.GetControl(MageSpellsWindow, i + 2)
				if ((1 << i) & SpellMask) == 0:
					GemRB.SetButtonState(MageSpellsWindow, SpellButton, IE_GUI_BUTTON_DISABLED)
			GemRB.SetButtonState(MageSpellsWindow, MageSpellsDoneButton, IE_GUI_BUTTON_ENABLED)

	PointsLeftLabel = GemRB.GetControl(MageSpellsWindow, 0x1000001b)
	GemRB.SetText(MageSpellsWindow, PointsLeftLabel, str(MageSpellsSelectPointsLeft))
	GemRB.SetVar("MageSpellBook", SpellMask)
	return

def MageSpellsCancelPress():
	print "done"
	GemRB.UnloadWindow(MageSpellsWindow)
	GemRB.SetNextScript("CharGen6") #haterace
	return

def MageSpellsDonePress():
	GemRB.UnloadWindow(MageSpellsWindow)
	GemRB.SetNextScript("GUICG6") #abilities
	return
