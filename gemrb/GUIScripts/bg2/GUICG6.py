#character generation, skills (GUICG6)
import GemRB

SkillWindow = 0
TextAreaControl = 0
DoneButton = 0
SkillTable = 0
TopIndex = 0

def ScrollBarPress():
	#redraw skill list
	return

def OnLoad():
	global SkillWindow, TextAreaControl, DoneButton, TopIndex
	global SkillTable
	
	Kit = GemRB.GetVar("Class Kit")
	if Kit == 0:
		ClassTable = GemRB.LoadTable("classes")
		Class = GemRB.GetVar("Class")-1
		KitName = GemRB.GetTableRowName(ClassTable, Class)
	else:
		KitList = GemRB.LoadTable("kitlist")
		KitName = GemRB.GetTableValue(KitList, Kit, 0) #rowname is just a number

        SkillTable = GemRB.LoadTable("skills")
	RowCount = GemRB.GetTableRowCount(SkillTable)
	Ok=0
	for i in range(0,RowCount):
		SkillName = GemRB.GetTableRowName(SkillTable,i)
		if GemRB.GetTableValue(SkillTable,SkillName, KitName)==1:
			Ok=1
			break

	if Ok==0:  #skipping 
		GemRB.SetNextScript("GUICG9")
		return
		
	Level = 1
	Sum = Level * 15
	GemRB.LoadWindowPack("GUICG")
        SkillTable = GemRB.LoadTable("skills")
	SkillWindow = GemRB.LoadWindow(6)

	BackButton = GemRB.GetControl(SkillWindow,25)
	GemRB.SetText(SkillWindow,BackButton,15416)
	DoneButton = GemRB.GetControl(SkillWindow,0)
	GemRB.SetText(SkillWindow,DoneButton,11973)

	TextAreaControl = GemRB.GetControl(SkillWindow, 19)
	GemRB.SetText(SkillWindow,TextAreaControl,17248)

	GemRB.SetEvent(SkillWindow,DoneButton,IE_GUI_BUTTON_ON_PRESS,"NextPress")
	GemRB.SetEvent(SkillWindow,BackButton,IE_GUI_BUTTON_ON_PRESS,"BackPress")
	GemRB.SetButtonState(SkillWindow,DoneButton,IE_GUI_BUTTON_DISABLED)
	GemRB.SetVisible(SkillWindow,1)
	return


def SkillPress():
	Skill = GemRB.GetVar("Skill")
	GemRB.SetText(SkillWindow, TextAreaControl, GemRB.GetTableValue(SkillTable,Skill,1) )
	GemRB.SetButtonState(SkillWindow, DoneButton, IE_GUI_BUTTON_ENABLED)
        GemRB.SetVar("Skill",GemRB.GetTableValue(SkillTable,Skill,3) )
	return

def BackPress():
	GemRB.UnloadWindow(SkillWindow)
	GemRB.SetNextScript("CharGen6")
	#scrap skills
	return

def NextPress():
        GemRB.UnloadWindow(SkillWindow)
	GemRB.SetNextScript("GUICG9") #weapon proficiencies
	return
