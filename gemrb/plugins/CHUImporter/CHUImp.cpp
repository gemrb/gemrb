/***************************************************************************
                          CHUImp.cpp  -  description
                             -------------------
    begin                : dom ott 12 2003
    copyright            : (C) 2003 by GemRB Developement Team
    email                : Balrog994@yahoo.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "CHUImp.h"
#include "../Core/Interface.h"
#include "../Core/Button.h"
#include "../Core/Label.h"
#include "../Core/Slider.h"
#include "../Core/ScrollBar.h"
#include "../Core/AnimationMgr.h"
#include "../../includes/RGBAColor.h"

CHUImp::CHUImp(){
	str = NULL;
	autoFree = false;
}
CHUImp::~CHUImp(){
	if(autoFree && str)
		delete(str);
}
/** This function loads all available windows from the 'stream' parameter. */
bool CHUImp::Open(DataStream * stream, bool autoFree)
{
	if(stream == NULL)
		return false;
	if(this->autoFree && str)
		delete(str);
	str = stream;
	this->autoFree = autoFree;
	char Signature[8];
	str->Read(Signature, 8);
	if(strncmp(Signature, "CHUIV1  ", 8) != 0) {
		printf("[CHUImporter]: Not a Vaild CHU File\n");
		return false;
	}
	str->Read(&WindowCount, 4);
	str->Read(&CTOffset, 4);
	str->Read(&WEOffset, 4);
	return true;
}
/** Returns the i-th window in the Previously Loaded Stream */
Window * CHUImp::GetWindow(unsigned long i)
{
	if(i >= WindowCount)
		return NULL;
	str->Seek(WEOffset+(0x1c*i), GEM_STREAM_START);
	unsigned short WindowID, XPos, YPos, Width, Height, BackGround, ControlsCount, FirstControl;
	str->Read(&WindowID, 2);
	str->Seek(2, GEM_CURRENT_POS);
	str->Read(&XPos, 2);
	str->Read(&YPos, 2);
	str->Read(&Width, 2);
	str->Read(&Height, 2);
	str->Read(&BackGround, 2);
	str->Read(&ControlsCount, 2);
	Window * win = new Window(WindowID, XPos, YPos, Width, Height);
	if(BackGround == 1) {
		char MosFile[9];
		str->Read(MosFile, 8);
		MosFile[8] = 0;
		if(core->IsAvailable(IE_MOS_CLASS_ID)) {
			DataStream * bkgr = core->GetResourceMgr()->GetResource(MosFile, IE_MOS_CLASS_ID);
			if(bkgr != NULL) {
				ImageMgr * mos = (ImageMgr*)core->GetInterface(IE_MOS_CLASS_ID);
				mos->Open(bkgr, true);
				win->SetBackGround(mos->GetImage(), true);
				core->FreeInterface(mos);
			}
			else
				printf("[CHUImporter]: Cannot Load BackGround, skipping\n");
		}
		else
			printf("[CHUImporter]: No MOS Importer Available, skipping background\n");
	}
	else
		str->Seek(8, GEM_CURRENT_POS);
	str->Read(&FirstControl, 2);
	if(!core->IsAvailable(IE_BAM_CLASS_ID)) {
		printf("[CHUImporter]: No BAM Importer Available, skipping controls\n");
		return win;
	}
	for(int i = 0;i < ControlsCount; i++) {
		str->Seek(CTOffset+((FirstControl+i)*8), GEM_STREAM_START);
		unsigned long COffset, CLength;
		unsigned short ControlID, BufferLength, XPos, YPos, Width, Height;
		unsigned char ControlType, temp;
		str->Read(&COffset, 4);
		str->Read(&CLength, 4);
		str->Seek(COffset, GEM_STREAM_START);
		str->Read(&ControlID, 2);
		str->Read(&BufferLength, 2);
		str->Read(&XPos, 2);
		str->Read(&YPos, 2);
		str->Read(&Width, 2);
		str->Read(&Height, 2);
		str->Read(&ControlType, 1);
		str->Read(&temp, 1);
		switch(ControlType) {
			case 0: {
				Button * btn = new Button(false);
				btn->ControlID = ControlID;
				btn->BufferLength = BufferLength;
				btn->XPos = XPos;
				btn->YPos = YPos;
				btn->Width = Width;
				btn->Height = Height;
				btn->ControlType = ControlType;
				char BAMFile[8];
				unsigned short Cycle, UnpressedIndex, PressedIndex, SelectedIndex, DisabledIndex;
				str->Read(BAMFile, 8);
				str->Read(&Cycle, 2);
				str->Read(&UnpressedIndex, 2);
				str->Read(&PressedIndex, 2);
				str->Read(&SelectedIndex, 2);
				str->Read(&DisabledIndex, 2);
				if(strncmp(BAMFile, "GUICTRL\0", 8) == 0) {
					if(UnpressedIndex == 0) {
						printf("Special Button Control, Skipping Image Loading\n");
						win->AddControl(btn);
						break;
					}
				}
				AnimationFactory * bam = (AnimationFactory*)core->GetResourceMgr()->GetFactoryResource(BAMFile, IE_BAM_CLASS_ID);
				if(bam == NULL) {
					printf("[CHUImporter]: Cannot Load Button Images, skipping control\n");
					delete(btn);
					continue;
					}
				Animation * ani = bam->GetCycle(Cycle);
				Sprite2D * tspr = ani->GetFrame(UnpressedIndex);
				btn->SetImage(IE_GUI_BUTTON_UNPRESSED, tspr);
				tspr = ani->GetFrame(PressedIndex);
				btn->SetImage(IE_GUI_BUTTON_PRESSED, tspr);
				tspr = ani->GetFrame(SelectedIndex);
				btn->SetImage(IE_GUI_BUTTON_SELECTED, tspr);
				tspr = ani->GetFrame(DisabledIndex);
				btn->SetImage(IE_GUI_BUTTON_DISABLED, tspr);
				ani->free = false;
				delete(ani);
				win->AddControl(btn);
				}
			break;

			case 2: {
				char MOSFile[8], BAMFile[8];
				unsigned short Cycle, Knob, GrabbedKnob;
				short KnobXPos, KnobYPos, KnobStep, KnobStepsCount;
				str->Read(MOSFile, 8);
				str->Read(BAMFile, 8);
				str->Read(&Cycle, 2);
				str->Read(&Knob, 2);
				str->Read(&GrabbedKnob, 2);
				str->Read(&KnobXPos, 2);
				str->Read(&KnobYPos, 2);
				str->Read(&KnobStep, 2);
				str->Read(&KnobStepsCount, 2);
				Slider * sldr = new Slider(KnobXPos, KnobYPos, KnobStep, KnobStepsCount, false);
				sldr->XPos = XPos;
				sldr->YPos = YPos;
				sldr->ControlID = ControlID;
				sldr->BufferLength = BufferLength;
				sldr->ControlType = ControlType;
				sldr->Width = Width;
				sldr->Height = Height;
				ImageMgr * mos = (ImageMgr*)core->GetInterface(IE_MOS_CLASS_ID);
				DataStream * s = core->GetResourceMgr()->GetResource(MOSFile, IE_MOS_CLASS_ID);
				mos->Open(s, true);
				Sprite2D * img = mos->GetImage();
				sldr->SetImage(IE_GUI_SLIDER_BACKGROUND, img);
				core->FreeInterface(mos);
				AnimationFactory * anim = (AnimationFactory*)core->GetResourceMgr()->GetFactoryResource(BAMFile, IE_BAM_CLASS_ID);
				img = anim->GetFrame(Knob);
				sldr->SetImage(IE_GUI_SLIDER_KNOB, img);
				img = anim->GetFrame(GrabbedKnob);
				sldr->SetImage(IE_GUI_SLIDER_GRABBEDKNOB, img);
				win->AddControl(sldr);
			}

			case 6: {
				char FontResRef[8];
				unsigned long StrRef;
				RevColor fore, back;
				unsigned short alignment;
				str->Read(&StrRef, 4);
				str->Read(FontResRef, 8);
				Font * fnt = core->GetFont(FontResRef);
				str->Read(&fore, 4);
				str->Read(&back, 4);
				str->Read(&alignment, 2);
				Label * lab = new Label(BufferLength, fnt);
				lab->ControlID = ControlID;
				lab->XPos = XPos;
				lab->YPos = YPos;
				lab->Width = Width;
				lab->Height = Height;
				lab->ControlType = ControlType;
				char * str = core->GetString(StrRef);
				lab->SetText(str);
				Color f;
				f.r = fore.r;
				f.g = fore.g;
				f.b = fore.b;
				lab->SetColor(f);
				free(str);
				win->AddControl(lab);
			}
			break;

			case 7: {
				char BAMResRef[9];
				unsigned short Cycle, UpUnPressed, UpPressed, DownUnPressed, DownPressed, Trough, Slider;
				str->Read(BAMResRef, 8);
				BAMResRef[8] = 0;
				str->Read(&Cycle, 2);
				str->Read(&UpUnPressed, 2);
				str->Read(&UpPressed, 2);
				str->Read(&DownUnPressed, 2);
				str->Read(&DownPressed, 2);
				str->Read(&Trough, 2);
				str->Read(&Slider, 2);
				ScrollBar * sbar = new ScrollBar();
				sbar->ControlID = ControlID;
				sbar->XPos = XPos;
				sbar->YPos = YPos;
				sbar->Width = Width;
				sbar->Height = Height;
				sbar->ControlType = ControlType;
				AnimationFactory * anim = (AnimationFactory*)core->GetResourceMgr()->GetFactoryResource(BAMResRef, IE_BAM_CLASS_ID);
				Animation * an = anim->GetCycle(Cycle);
				sbar->SetImage(IE_GUI_SCROLLBAR_UP_UNPRESSED, an->GetFrame(UpUnPressed));
				sbar->SetImage(IE_GUI_SCROLLBAR_UP_PRESSED, an->GetFrame(UpPressed));
				sbar->SetImage(IE_GUI_SCROLLBAR_DOWN_UNPRESSED, an->GetFrame(DownUnPressed));
				sbar->SetImage(IE_GUI_SCROLLBAR_DOWN_PRESSED, an->GetFrame(DownPressed));
				sbar->SetImage(IE_GUI_SCROLLBAR_TROUGH, an->GetFrame(Trough));
				sbar->SetImage(IE_GUI_SCROLLBAR_SLIDER, an->GetFrame(Slider));
				an->free = false;
				delete(an);
				win->AddControl(sbar);
			}
			break;

			default:
				printf("[CHUImporter]: Control Not Supported\n");
		}
	}
	return win;
}
/** Returns the number of available windows */
unsigned long CHUImp::GetWindowsCount()
{
	return WindowCount;
}
