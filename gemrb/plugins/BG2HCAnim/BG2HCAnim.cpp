#include "../../includes/win32def.h"
#include "BG2HCAnim.h"
#include "../Core/Interface.h"
#include "../Core/AnimationMgr.h"

BG2HCAnim::BG2HCAnim(void)
{
}

BG2HCAnim::~BG2HCAnim(void)
{
}

void BG2HCAnim::GetCharAnimations(Actor * actor)
{
	switch(actor->Animation) 
		{
		case 0x4000: // STATIC_NOBLE_MAN_CHAIR
		case 0x4002: // STATIC_NOBLE_MAN_MATTE
		case 0x4010: // STATIC_NOBLE_WOMAN_CHAIR
		case 0x4012: // STATIC_NOBLE_WOMAN_MATTE
		case 0x4100: // STATIC_PEASANT_MAN_CHAIR
		case 0x4101: // STATIC_PEASANT_MAN_STOOL
		case 0x4102: // STATIC_PEASANT_MAN_MATTE
		case 0x4110: // STATIC_PEASANT_WOMAN_CHAIR
		case 0x4112: // STATIC_PEASANT_WOMAN_MATTE
		case 0x4200: // STATIC_HUMANCLERIC_MAN_CHAIR
		case 0x4300: // STATIC_SPIDER_WOMAN
			{
			ParseStaticAnims(actor);	
			}
		break;

		case 0x4400: // SLEEPING_MAN_HUMAN
		case 0x4410: // SLEEPING_WOMAN_HUMAN
		case 0x4500: // SLEEPING_FAT_MAN_HUMAN
		case 0x4600: // SLEEPING_DWARF
		case 0x4700: // SLEEPING_MAN_ELF
		case 0x4710: // SLEEPING_WOMAN_ELF
		case 0x4800: // SLEEPING_MAN_HALFLING
			{
			ParseSleeping(actor);
			}
		break;

		case 0x5000: // CLERIC_MALE_HUMAN_LOW
		case 0x5001: // CLERIC_MALE_ELF_LOW
		case 0x5002: // CLERIC_MALE_DWARF_LOW
		case 0x5003: // CLERIC_MALE_HALFLING_LOW
		case 0x5010: // CLERIC_FEMALE_HUMAN_LOW
		case 0x5011: // CLERIC_FEMALE_ELF_LOW
		case 0x5012: // CLERIC_FEMALE_DWARF_LOW
		case 0x5013: // CLERIC_FEMALE_HALFLING_LOW
		case 0x5100: // FIGHTER_MALE_HUMAN_LOW
		case 0x5101: // FIGHTER_MALE_ELF_LOW
		case 0x5102: // FIGHTER_MALE_DWARF_LOW
		case 0x5103: // FIGHTER_MALE_HALFLING_LOW
		case 0x5110: // FIGHTER_FEMALE_HUMAN_LOW
		case 0x5111: // FIGHTER_FEMALE_ELF_LOW
		case 0x5112: // FIGHTER_FEMALE_DWARF_LOW
		case 0x5113: // FIGHTER_FEMALE_HALFLING_LOW
		case 0x5200: // MAGE_MALE_HUMAN_LOW
		case 0x5201: // MAGE_MALE_ELF_LOW
		case 0x5202: // MAGE_MALE_DWARF_LOW
		case 0x5210: // MAGE_FEMALE_HUMAN_LOW
		case 0x5211: // MAGE_FEMALE_ELF_LOW
		case 0x5212: // MAGE_FEMALE_DWARF_LOW
		case 0x5300: // THIEF_MALE_HUMAN_LOW
		case 0x5301: // THIEF_MALE_ELF_LOW
		case 0x5302: // THIEF_MALE_DWARF_LOW
		case 0x5303: // THIEF_MALE_HALFLING_LOW
		case 0x5310: // THIEF_FEMALE_HUMAN_LOW
		case 0x5311: // THIEF_FEMALE_ELF_LOW
		case 0x5312: // THIEF_FEMALE_DWARF_LOW
		case 0x5313: // THIEF_FEMALE_HALFLING_LOW
			{
			ParseFullAnimLow(actor);
			}
		break;

		case 0x2000: // SIRINE
			LoadMonsterMidRes("MSIR", actor);
		break;

		case 0x2300: // DEATH_KNIGHT
			LoadMonsterMidRes("MDKN", actor);
		break;

		case 0x6102: // FIGHTER_MALE_DWARF
			LoadMonsterHiRes("CDMB1", actor);
		break;

		case 0x6300: // THIEF_MALE_HUMAN
			LoadMonsterHiRes("CHMB1", actor);
		break;

		case 0x6403: // SKELETON
			LoadMonsterMidRes("MSKL1", actor);
		break;

		case 0x6405: // DOOM_GUARD
			LoadMonsterMidRes("MDGU1", actor);
		break;

		case 0x7F03: // IMP
			LoadMonsterHiRes("MIMP", actor);
		break;

		case 0x7F05: // DJINNI
			LoadMonsterHiRes("MDJI", actor);
		break;

		case 0x7F07: // GOLEM_CLAY
			LoadMonsterHiRes("MGLC", actor);
		break;

		case 0x7F08: // OTYUGH
			LoadMonsterHiRes("MOTY", actor);
		break;

		case 0xE400: // IC_GOBLIN_AXE
			LoadCritter("MGO1", actor, false);
		break;

		case 0xE430: // IC_GOBLINELITE_BOW
			LoadCritter("MGO4", actor, true);
		break;

		default:
			{
			printf("[BG2HCAnim]: Unsupported Animation (0x%04hX)\n", actor->Animation);
			}
		break;
		}
}
void BG2HCAnim::LoadCritter(const char * ResRef, Actor * actor, bool bow)
{
	char tResRef[9] = {0};
	//Walking
	strcpy(tResRef, ResRef);
	strcat(tResRef, "WK");
	AnimationFactory * a  = (AnimationFactory*)core->GetResourceMgr()->GetFactoryResource(tResRef, IE_BAM_CLASS_ID);
	strcat(tResRef, "E");
	AnimationFactory * ae = (AnimationFactory*)core->GetResourceMgr()->GetFactoryResource(tResRef, IE_BAM_CLASS_ID);
	for(int i = 0; i < 5; i++) {
		actor->anims->Walk.push_back(a->GetCycle(i));
	}
	for(int i = 5; i < 8; i++) {
		actor->anims->Walk.push_back(ae->GetCycle(i));
	}
	//Standing
	strcpy(tResRef, ResRef);
	strcat(tResRef, "SD");
	a  = (AnimationFactory*)core->GetResourceMgr()->GetFactoryResource(tResRef, IE_BAM_CLASS_ID);
	strcat(tResRef, "E");
	ae = (AnimationFactory*)core->GetResourceMgr()->GetFactoryResource(tResRef, IE_BAM_CLASS_ID);
	for(int i = 0; i < 5; i++) {
		actor->anims->Stands.push_back(a->GetCycle(i));
	}
	for(int i = 5; i < 8; i++) {
		actor->anims->Stands.push_back(ae->GetCycle(i));
	}
	//Unconscious
	strcpy(tResRef, ResRef);
	strcat(tResRef, "TW");
	a  = (AnimationFactory*)core->GetResourceMgr()->GetFactoryResource(tResRef, IE_BAM_CLASS_ID);
	strcat(tResRef, "E");
	ae = (AnimationFactory*)core->GetResourceMgr()->GetFactoryResource(tResRef, IE_BAM_CLASS_ID);
	for(int i = 0; i < 5; i++) {
		actor->anims->Stands.push_back(a->GetCycle(i));
	}
	for(int i = 5; i < 8; i++) {
		actor->anims->Stands.push_back(ae->GetCycle(i));
	}
	//Sleep
	strcpy(tResRef, ResRef);
	strcat(tResRef, "SL");
	a  = (AnimationFactory*)core->GetResourceMgr()->GetFactoryResource(tResRef, IE_BAM_CLASS_ID);
	strcat(tResRef, "E");
	ae = (AnimationFactory*)core->GetResourceMgr()->GetFactoryResource(tResRef, IE_BAM_CLASS_ID);
	for(int i = 0; i < 5; i++) {
		actor->anims->Stands.push_back(a->GetCycle(i));
	}
	for(int i = 5; i < 8; i++) {
		actor->anims->Stands.push_back(ae->GetCycle(i));
	}
	//Spell Cast
	strcpy(tResRef, ResRef);
	strcat(tResRef, "SC");
	a  = (AnimationFactory*)core->GetResourceMgr()->GetFactoryResource(tResRef, IE_BAM_CLASS_ID);
	strcat(tResRef, "E");
	ae = (AnimationFactory*)core->GetResourceMgr()->GetFactoryResource(tResRef, IE_BAM_CLASS_ID);
	for(int i = 0; i < 5; i++) {
		actor->anims->SpellCasts.push_back(a->GetCycle(i));
	}
	for(int i = 5; i < 8; i++) {
		actor->anims->SpellCasts.push_back(ae->GetCycle(i));
	}
	//Get Up
	strcpy(tResRef, ResRef);
	strcat(tResRef, "GU");
	a  = (AnimationFactory*)core->GetResourceMgr()->GetFactoryResource(tResRef, IE_BAM_CLASS_ID);
	strcat(tResRef, "E");
	ae = (AnimationFactory*)core->GetResourceMgr()->GetFactoryResource(tResRef, IE_BAM_CLASS_ID);
	for(int i = 0; i < 5; i++) {
		actor->anims->Stands.push_back(a->GetCycle(i));
	}
	for(int i = 5; i < 8; i++) {
		actor->anims->Stands.push_back(ae->GetCycle(i));
	}
	//Get Hit
	strcpy(tResRef, ResRef);
	strcat(tResRef, "GH");
	a  = (AnimationFactory*)core->GetResourceMgr()->GetFactoryResource(tResRef, IE_BAM_CLASS_ID);
	strcat(tResRef, "E");
	ae = (AnimationFactory*)core->GetResourceMgr()->GetFactoryResource(tResRef, IE_BAM_CLASS_ID);
	for(int i = 0; i < 5; i++) {
		actor->anims->Stands.push_back(a->GetCycle(i));
	}
	for(int i = 5; i < 8; i++) {
		actor->anims->Stands.push_back(ae->GetCycle(i));
	}
	//Death
	strcpy(tResRef, ResRef);
	strcat(tResRef, "DE");
	a  = (AnimationFactory*)core->GetResourceMgr()->GetFactoryResource(tResRef, IE_BAM_CLASS_ID);
	strcat(tResRef, "E");
	ae = (AnimationFactory*)core->GetResourceMgr()->GetFactoryResource(tResRef, IE_BAM_CLASS_ID);
	for(int i = 0; i < 5; i++) {
		actor->anims->Stands.push_back(a->GetCycle(i));
	}
	for(int i = 5; i < 8; i++) {
		actor->anims->Stands.push_back(ae->GetCycle(i));
	}
	//Attack 1
	strcpy(tResRef, ResRef);
	strcat(tResRef, "A1");
	a  = (AnimationFactory*)core->GetResourceMgr()->GetFactoryResource(tResRef, IE_BAM_CLASS_ID);
	strcat(tResRef, "E");
	ae = (AnimationFactory*)core->GetResourceMgr()->GetFactoryResource(tResRef, IE_BAM_CLASS_ID);
	for(int i = 0; i < 5; i++) {
		actor->anims->a1HSattack.push_back(a->GetCycle(i));
	}
	for(int i = 5; i < 8; i++) {
		actor->anims->a1HSattack.push_back(ae->GetCycle(i));
	}
	//Attack 2
	strcpy(tResRef, ResRef);
	if(!bow)
		strcat(tResRef, "A2");
	else
		strcat(tResRef, "A4");
	a  = (AnimationFactory*)core->GetResourceMgr()->GetFactoryResource(tResRef, IE_BAM_CLASS_ID);
	strcat(tResRef, "E");
	ae = (AnimationFactory*)core->GetResourceMgr()->GetFactoryResource(tResRef, IE_BAM_CLASS_ID);
	for(int i = 0; i < 5; i++) {
		actor->anims->a1HSattack.push_back(a->GetCycle(i));
	}
	for(int i = 5; i < 8; i++) {
		actor->anims->a1HSattack.push_back(ae->GetCycle(i));
	}
}

void BG2HCAnim::LoadMonsterMidRes(const char * ResRef, Actor * actor)
{
	char tResRef[9] = {0};
	//Walking
	strcpy(tResRef, ResRef);
	strcat(tResRef, "G1");
	AnimationFactory * a  = (AnimationFactory*)core->GetResourceMgr()->GetFactoryResource(tResRef, IE_BAM_CLASS_ID);
	strcat(tResRef, "E");
	AnimationFactory * ae = (AnimationFactory*)core->GetResourceMgr()->GetFactoryResource(tResRef, IE_BAM_CLASS_ID);
	for(int i = 0; i < 5; i++) {
		actor->anims->Walk.push_back(a->GetCycle(i));
	}
	for(int i = 5; i < 8; i++) {
		actor->anims->Walk.push_back(ae->GetCycle(i));
	}
	//Standing
	for(int i = 8; i < 13; i++) {
		actor->anims->Stands.push_back(a->GetCycle(i));
	}
	for(int i = 13; i < 16; i++) {
		actor->anims->Stands.push_back(ae->GetCycle(i));
	}
	//Standing (Looking)
	for(int i = 16; i < 21; i++) {
		actor->anims->Stands.push_back(a->GetCycle(i));
	}
	for(int i = 21; i < 24; i++) {
		actor->anims->Stands.push_back(ae->GetCycle(i));
	}
	//Armed Standing
	for(int i = 24; i < 29; i++) {
		actor->anims->Stands.push_back(a->GetCycle(i));
	}
	for(int i = 29; i < 32; i++) {
		actor->anims->Stands.push_back(ae->GetCycle(i));
	}
	//Taken Hit
	//TODO: Try to understand others
}

void BG2HCAnim::LoadMonsterHiRes(const char * ResRef, Actor * actor)
{
	//AnimationMgr * a = (AnimationMgr*)core->GetInterface(IE_BAM_CLASS_ID);
	char tResRef[9] = {0};
	strcpy(tResRef, ResRef);
	strcat(tResRef, "G1");
	AnimationFactory * a = (AnimationFactory*)core->GetResourceMgr()->GetFactoryResource(tResRef, IE_BAM_CLASS_ID);
	for(int i = 9; i < 19; i++) {
		actor->anims->Stands.push_back(a->GetCycle(i));
	}
	strcat(tResRef, "1");
	a = (AnimationFactory*)core->GetResourceMgr()->GetFactoryResource(tResRef, IE_BAM_CLASS_ID);
	for(int i = 0; i < 10; i++) {
		actor->anims->Walk.push_back(a->GetCycle(i));
	}
	strcpy(tResRef, ResRef);
	strcat(tResRef, "G12");
	a = (AnimationFactory*)core->GetResourceMgr()->GetFactoryResource(tResRef, IE_BAM_CLASS_ID);
	for(int i = 18; i < 28; i++) {
		actor->anims->Stands.push_back(a->GetCycle(i)); // TODO: Check This, I'm not sure it is a stand position, maybe an attack
	}
	//core->FreeInterface(a);
	//TODO: Add more imports
}

void BG2HCAnim::LoadStaticAnims(const char * ResRef, Actor * actor)
{
	AnimationMgr * a = (AnimationMgr*)core->GetInterface(IE_BAM_CLASS_ID);
	a->Open(core->GetResourceMgr()->GetResource(ResRef, IE_BAM_CLASS_ID), true);
	for(int i = 0; i < 16; i++) {
		actor->anims->Stands.push_back(a->GetAnimation(i, 0, 0));
	}
	core->FreeInterface(a);
}

void BG2HCAnim::LoadFullAnimLow(const char * ResRef, Actor * actor)
{
	char ResRefE[9];
	strcpy(ResRefE, ResRef);
	strcat(ResRefE, "E");
	AnimationMgr * a = (AnimationMgr*)core->GetInterface(IE_BAM_CLASS_ID);
	AnimationMgr * a1 = (AnimationMgr*)core->GetInterface(IE_BAM_CLASS_ID);
	a->Open(core->GetResourceMgr()->GetResource(ResRef, IE_BAM_CLASS_ID), true);
	a1->Open(core->GetResourceMgr()->GetResource(ResRefE, IE_BAM_CLASS_ID), true);
	for(int i = 0; i < 5; i++) {
		actor->anims->Stands.push_back(a->GetAnimation(i, 0, 0)); //Walk Ovest
	}
	for(int i = 5; i < 9; i++) {
		actor->anims->Stands.push_back(a1->GetAnimation(i, 0, 0)); //Walk Est
	}
	for(int i = 9; i < 13; i++) {
		actor->anims->Stands.push_back(a->GetAnimation(i, 0, 0)); //Stand Long Ovest
	}
	for(int i = 13; i < 16; i++) {
		actor->anims->Stands.push_back(a1->GetAnimation(i, 0, 0)); //Stand Long Est
	}
	for(int i = 16; i < 21; i++) {
		actor->anims->Stands.push_back(a->GetAnimation(i, 0, 0)); //Stand Short Ovest
	}
	for(int i = 21; i < 24; i++) {
		actor->anims->Stands.push_back(a1->GetAnimation(i, 0, 0)); //Stand Short Est
	}
	for(int i = 24; i < 29; i++) {
		actor->anims->Stands.push_back(a->GetAnimation(i, 0, 0)); //Get Hit Ovest
	}
	for(int i = 29; i < 32; i++) {
		actor->anims->Stands.push_back(a1->GetAnimation(i, 0, 0)); //Get Hit Est
	}
	for(int i = 32; i < 37; i++) {
		actor->anims->Stands.push_back(a->GetAnimation(i, 0, 0)); //Falling Ovest
	}
	for(int i = 37; i < 40; i++) {
		actor->anims->Stands.push_back(a1->GetAnimation(i, 0, 0)); //Falling Est
	}
	for(int i = 40; i < 45; i++) {
		actor->anims->Stands.push_back(a->GetAnimation(i, 0, 0)); //Dead Ovest
	}
	for(int i = 45; i < 48; i++) {
		actor->anims->Stands.push_back(a1->GetAnimation(i, 0, 0)); //Dead Long Est
	}
	core->FreeInterface(a);
	core->FreeInterface(a1);
}

void BG2HCAnim::ParseStaticAnims(Actor* actor)
{
	switch(actor->Animation) 
		{
		case 0x4000:
			LoadStaticAnims("SNOMC\0\0\0", actor);
		break;
									
		case 0x4002:
			LoadStaticAnims("SNOMM\0\0\0", actor);
		break;

		case 0x4010:
			LoadStaticAnims("SNOWC\0\0\0", actor);
		break;
									
		case 0x4012:
			LoadStaticAnims("SNOWM\0\0\0", actor);
		break;
		
		case 0x4100:
			LoadStaticAnims("SSIMC\0\0\0", actor);
		break;
									
		case 0x4102:
			LoadStaticAnims("SSIMM\0\0\0", actor);
		break;

		case 0x4110:
			LoadStaticAnims("SSIWC\0\0\0", actor);
		break;
									
		case 0x4112:
			LoadStaticAnims("SSIWM\0\0\0", actor);
		break;
		
		case 0x4200:
			LoadStaticAnims("SHMCM\0\0\0", actor);
		break;

		case 0x4300:
		break;
	}
}

void BG2HCAnim::ParseSleeping(Actor* actor)
{
	switch(actor->Animation) {
		case 0x4400: // SLEEPING_MAN_HUMAN
			LoadStaticAnims("LHMC\0\0\0\0", actor);
		break;

		case 0x4410: // SLEEPING_WOMAN_HUMAN
			LoadStaticAnims("LHFC\0\0\0\0", actor);
		break;

		case 0x4500: // SLEEPING_FAT_MAN_HUMAN
			LoadStaticAnims("LFAM\0\0\0\0", actor);
		break;

		case 0x4600: // SLEEPING_DWARF
			LoadStaticAnims("LDMF\0\0\0\0", actor);
		break;

		case 0x4700: // SLEEPING_MAN_ELF
			LoadStaticAnims("LEMF\0\0\0\0", actor);
		break;

		case 0x4710: // SLEEPING_WOMAN_ELF
			LoadStaticAnims("LEFF\0\0\0\0", actor);
		break;

		case 0x4800: // SLEEPING_MAN_HALFLING
			LoadStaticAnims("LIMC\0\0\0\0", actor);
		break;
	}
}

void BG2HCAnim::ParseFullAnimLow(Actor* actor)
{
	switch(actor->Animation) {
		case 0x5000: // CLERIC_MALE_HUMAN_LOW
		case 0x5001: // CLERIC_MALE_ELF_LOW
		case 0x5002: // CLERIC_MALE_DWARF_LOW
		case 0x5003: // CLERIC_MALE_HALFLING_LOW
		case 0x5010: // CLERIC_FEMALE_HUMAN_LOW
		case 0x5011: // CLERIC_FEMALE_ELF_LOW
		case 0x5012: // CLERIC_FEMALE_DWARF_LOW
		case 0x5013: // CLERIC_FEMALE_HALFLING_LOW
		case 0x5100: // FIGHTER_MALE_HUMAN_LOW
		case 0x5101: // FIGHTER_MALE_ELF_LOW
		case 0x5102: // FIGHTER_MALE_DWARF_LOW
		case 0x5103: // FIGHTER_MALE_HALFLING_LOW
		case 0x5110: // FIGHTER_FEMALE_HUMAN_LOW
		case 0x5111: // FIGHTER_FEMALE_ELF_LOW
		case 0x5112: // FIGHTER_FEMALE_DWARF_LOW
		case 0x5113: // FIGHTER_FEMALE_HALFLING_LOW
		case 0x5200: // MAGE_MALE_HUMAN_LOW
		case 0x5201: // MAGE_MALE_ELF_LOW
		case 0x5202: // MAGE_MALE_DWARF_LOW
		case 0x5210: // MAGE_FEMALE_HUMAN_LOW
		case 0x5211: // MAGE_FEMALE_ELF_LOW
		case 0x5212: // MAGE_FEMALE_DWARF_LOW
		case 0x5300: // THIEF_MALE_HUMAN_LOW
		case 0x5301: // THIEF_MALE_ELF_LOW
		case 0x5302: // THIEF_MALE_DWARF_LOW
		case 0x5303: // THIEF_MALE_HALFLING_LOW
		case 0x5310: // THIEF_FEMALE_HUMAN_LOW
		case 0x5311: // THIEF_FEMALE_ELF_LOW
		case 0x5312: // THIEF_FEMALE_DWARF_LOW
		case 0x5313: // THIEF_FEMALE_HALFLING_LOW
			printf("[BG2HCAnims]: Low Quality Character Animation not supported (0x%04hX)\n", actor->Animation);
		break;
	}
}
