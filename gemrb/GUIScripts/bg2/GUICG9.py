#character generation, proficiencies (GUICG9)
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

        SkillOk = GemRB.LoadTable("ALIGNMNT")

	GemRB.LoadWindowPack("GUICG")
        SkillTable = GemRB.LoadTable("weapprof")
	SkillWindow = GemRB.LoadWindow(9)

	BackButton = GemRB.GetControl(SkillWindow,25)
	GemRB.SetText(SkillWindow,BackButton,15416)
	DoneButton = GemRB.GetControl(SkillWindow,0)
	GemRB.SetText(SkillWindow,DoneButton,11973)

	TextAreaControl = GemRB.GetControl(SkillWindow, 19)
	GemRB.SetText(SkillWindow,TextAreaControl,9602)

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
	GemRB.SetNextScript("CharGen7") #appearance
	return
