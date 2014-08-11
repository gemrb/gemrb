/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include "DialogHandler.h"

#include "strrefs.h"

#include "DialogMgr.h"
#include "DisplayMessage.h"
#include "Game.h"
#include "GameData.h"
#include "GlobalTimer.h"
#include "ImageMgr.h"
#include "PluginMgr.h"
#include "ScriptEngine.h"
#include "TableMgr.h"
#include "Video.h"
#include "GameScript/GameScript.h"
#include "GUI/GameControl.h"
#include "GUI/TextArea.h"

namespace GemRB {

//translate section values (journal, solved, unsolved, user)
static int sectionMap[4]={4,1,2,0};
static const int bg2Sections[4]={4,1,2,0};
static const int noSections[4]={0,0,0,0};

DialogHandler::DialogHandler(void)
{
	dlg = NULL;
	targetID = 0;
	originalTargetID = 0;
	speakerID = 0;
	initialState = -1;
	if (core->HasFeature(GF_JOURNAL_HAS_SECTIONS) ) {
		memcpy(sectionMap, bg2Sections, sizeof(sectionMap) );
	} else {
		memcpy(sectionMap, noSections, sizeof(sectionMap) );
	}
}

DialogHandler::~DialogHandler(void)
{
	if (dlg) {
		delete dlg;
	}
}

//Try to start dialogue between two actors (one of them could be inanimate)
bool DialogHandler::InitDialog(Scriptable* spk, Scriptable* tgt, const char* dlgref)
{
	if (dlg) {
		delete dlg;
		dlg = NULL;
	}

	PluginHolder<DialogMgr> dm(IE_DLG_CLASS_ID);
	dm->Open(gamedata->GetResource(dlgref, IE_DLG_CLASS_ID));
	dlg = dm->GetDialog();

	if (!dlg) {
		Log(ERROR, "GameControl", "Cannot start dialog: %s", dlgref);
		return false;
	}

	strnlwrcpy(dlg->ResRef, dlgref, 8); //this isn't handled by GetDialog???

	//target is here because it could be changed when a dialog runs onto
	//and external link, we need to find the new target (whose dialog was
	//linked to)

	Actor *oldTarget = GetActorByGlobalID(targetID);
	speakerID = spk->GetGlobalID();
	targetID = tgt->GetGlobalID();
	if (!originalTargetID) originalTargetID = tgt->GetGlobalID();
	if (tgt->Type==ST_ACTOR) {
		Actor *tar = (Actor *) tgt;
		// TODO: verify
		spk->LastTalker=targetID;
		tar->LastTalker=speakerID;
		tar->SetCircleSize();
	}
	if (oldTarget) oldTarget->SetCircleSize();

	GameControl *gc = core->GetGameControl();

	if (!gc)
		return false;

	Video *video = core->GetVideoDriver();
	Region vp = video->GetViewport();
	//allow mouse selection from dialog (even though screen is locked)
	video->SetMouseEnabled(true);
	core->timer->SetMoveViewPort( tgt->Pos.x, tgt->Pos.y, 0, true );
	video->MoveViewportTo( tgt->Pos.x-vp.w/2, tgt->Pos.y-vp.h/2 );

	//check if we are already in dialog
	if (gc->GetDialogueFlags()&DF_IN_DIALOG) {
		return true;
	}

	initialState = dlg->FindFirstState( tgt );
	if (initialState < 0) {
		return false;
	}

	//we need GUI for dialogs
	//but the guiscript must be in control here
	//gc->UnhideGUI();

	//no exploring while in dialogue
	gc->SetScreenFlags(/*SF_GUIENABLED|*/SF_DISABLEMOUSE|SF_LOCKSCROLL, BM_OR);
	Log(WARNING, "DialogHandler", "Errors occuring while in dialog mode cannot be logged in the MessageWindow.");
	gc->SetDialogueFlags(DF_IN_DIALOG, BM_OR);

	if (tgt->Type==ST_ACTOR) {
		Actor *tar = (Actor *) tgt;
		tar->DialogInterrupt();
	}

	//there are 3 bits, if they are all unset, the dialog freezes scripts
	if (!(dlg->Flags&7) ) {
		gc->SetDialogueFlags(DF_FREEZE_SCRIPTS, BM_OR);
	}
	//opening control size to maximum, enabling dialog window
	//but the guiscript must be in control here
	//core->GetGame()->SetControlStatus(CS_HIDEGUI, BM_NAND);
	//core->GetGame()->SetControlStatus(CS_DIALOG, BM_OR);
	//core->SetEventFlag(EF_PORTRAIT);
	return true;
}

/*try to break will only try to break it, false means unconditional stop*/
void DialogHandler::EndDialog(bool try_to_break)
{
	if (try_to_break && (core->GetGameControl()->GetDialogueFlags()&DF_UNBREAKABLE) ) {
		return;
	}

	Actor *tmp = GetSpeaker();
	if (tmp) {
		tmp->LeaveDialog();
	}
	speakerID = 0;
	Scriptable *tmp2 = GetTarget();
	if (tmp2 && tmp2->Type == ST_ACTOR) {
		tmp = (Actor *)tmp2;
	} else {
		tmp = NULL;
	}
	if (tmp) {
		tmp->LeaveDialog();
	}
	targetID = 0;
	if (tmp) tmp->SetCircleSize();
	originalTargetID = 0;
	ds = NULL;
	if (dlg) {
		delete dlg;
		dlg = NULL;
	}
	// FIXME: it's not so nice having this here, but things call EndDialog directly :(
	core->GetGUIScriptEngine()->RunFunction( "GUIWORLD", "DialogEnded" );
	//restoring original size
	core->GetGame()->SetControlStatus(CS_DIALOG, BM_NAND);
	GameControl* gc = core->GetGameControl();
	if ( !(gc->GetScreenFlags()&SF_CUTSCENE)) {
		gc->SetScreenFlags(SF_DISABLEMOUSE|SF_LOCKSCROLL, BM_NAND);
	}
	gc->SetDialogueFlags(0, BM_SET);
	core->SetEventFlag(EF_PORTRAIT);
}

bool DialogHandler::DialogChoose(Control* ctl)
{
	return DialogChoose(ctl->Value);
}

bool DialogHandler::DialogChoose(unsigned int choose)
{
	TextArea* ta = core->GetMessageTextArea();
	if (!ta) {
		Log(ERROR, "GameControl", "Dialog aborted???");
		EndDialog();
		return false;
	}
	// reset the TA
	ta->SetAnimPicture(NULL);
	ta->ClearSelectOptions();

	Actor *speaker = GetSpeaker();
	if (!speaker) {
		Log(ERROR, "GameControl", "Speaker gone???");
		EndDialog();
		return false;
	}

	Scriptable *target = GetTarget();
	if (!target) {
		Log(ERROR, "GameControl", "Target gone???");
		EndDialog();
		return false;
	}
	Actor *tgt = NULL;
	if (target->Type == ST_ACTOR) {
		tgt = (Actor *)target;
	}

	int si;
	GameControl* gc = core->GetGameControl();
	if (choose == (unsigned int) -1) {
		//increasing talkcount after top level condition was determined

		si = initialState;
		if (si<0) {
			EndDialog();
			return false;
		}

		if (tgt) {
			if (gc->GetDialogueFlags()&DF_TALKCOUNT) {
				gc->SetDialogueFlags(DF_TALKCOUNT, BM_NAND);
				tgt->TalkCount++;
			} else if (gc->GetDialogueFlags()&DF_INTERACT) {
				gc->SetDialogueFlags(DF_INTERACT, BM_NAND);
				tgt->InteractCount++;
			}
		}
		//ds = dlg->GetState( si );
	} else {
		if (!ds || ds->transitionsCount <= choose) {
			return false;
		}

		DialogTransition* tr = ds->transitions[choose];

		if (tr->Flags&IE_DLG_TR_JOURNAL) {
			int Section = 0;
			if (tr->Flags&IE_DLG_UNSOLVED) {
				Section |= 1;
			}
			if (tr->Flags&IE_DLG_SOLVED) {
				Section |= 2;
			}
			if (core->GetGame()->AddJournalEntry(tr->journalStrRef, sectionMap[Section], tr->Flags>>16) ) {
				 String msg(L"[color=bcefbc]");
				 String* str = core->GetString(displaymsg->GetStringReference(STR_JOURNALCHANGE));
				 msg += *str;
				 delete str;
				 str = core->GetString(tr->journalStrRef);
				 //cutting off the strings at the first crlf
				 str->resize(str->find_first_of(L"\n"));
				 msg += L" - [/color][color=e4c999]" + *str + L"[/color]";
				 delete str;
				 displaymsg->DisplayMarkupString(msg);
			}
		}

		if (tr->textStrRef != 0xffffffff) {
			//allow_zero is for PST (deionarra's text)
			displaymsg->DisplayStringName( (int) (tr->textStrRef), DMC_DIALOGPARTY, speaker, IE_STR_SOUND|IE_STR_SPEECH|IE_STR_ALLOW_ZERO);
			if (core->HasFeature( GF_DIALOGUE_SCROLLS )) {
			}
		}
		target->ImmediateEvent();
		target->ProcessActions(); //run the action queue now

		if (tr->actions.size()) {
			// does this belong here? we must clear actions somewhere before
			// we start executing them (otherwise queued actions interfere)
			// executing actions directly does not work, because dialog
			// needs to end before final actions are executed due to
			// actions making new dialogs!
			target->Stop();

			// do not interrupt during dialog actions (needed for aerie.d polymorph block)
			target->AddAction( GenerateAction( "BreakInstants()" ) );
			target->AddAction( GenerateAction( "SetInterrupt(FALSE)" ) );
			for (unsigned int i = 0; i < tr->actions.size(); i++) {
				target->AddAction(tr->actions[i]);
			}
			target->AddAction( GenerateAction( "SetInterrupt(TRUE)" ) );
		}

		int final_dialog = tr->Flags & IE_DLG_TR_FINAL;

		if (final_dialog) {
			EndDialog();
		}

		if (final_dialog) {
			return false;
		}

		// avoid problems when dhjollde.dlg tries starting a cutscene in the middle of a dialog
		// (it seems harmless doing it in non-HoW too, since other versions would just break in such a situation)
		core->SetCutSceneMode( false );

		//displaying dialog for selected option
		si = tr->stateIndex;
		//follow external linkage, if required
		if (tr->Dialog[0] && strnicmp( tr->Dialog, dlg->ResRef, 8 )) {
			//target should be recalculated!
			tgt = NULL;
			if (originalTargetID) {
				// always try original target first (sometimes there are multiple
				// actors with the same dialog in an area, we want to pick the one
				// we were talking to)
				tgt = GetActorByGlobalID(originalTargetID);
				if (tgt && strnicmp( tgt->GetDialog(GD_NORMAL), tr->Dialog, 8 ) != 0) {
					tgt = NULL;
				}
			}
			if (!tgt) {
				// then just search the current area for an actor with the dialog
				tgt = target->GetCurrentArea()->GetActorByDialog(tr->Dialog);
			}
			if (!tgt) {
				// try searching for banter dialogue: the original engine seems to
				// happily let you randomly switch between normal and banter dialogs

				// TODO: work out if this should go somewhere more central (such
				// as GetActorByDialog), or if there's a less awful way to do this
				// (we could cache the entries, for example)
				AutoTable pdtable("interdia");
				if (pdtable) {
					int col;

					if (core->GetGame()->Expansion==5) {
						col = pdtable->GetColumnIndex("25FILE");
					} else {
						col = pdtable->GetColumnIndex("FILE");
					}
					int row = pdtable->FindTableValue( col, tr->Dialog );
					tgt = target->GetCurrentArea()->GetActorByScriptName(pdtable->GetRowName(row));
				}
			}
			target = tgt;
			if (!target) {
				Log(WARNING, "Dialog", "Can't redirect dialog");
				EndDialog();
				return false;
			}
			Actor *oldTarget = GetActorByGlobalID(targetID);
			targetID = tgt->GetGlobalID();
			tgt->SetCircleSize();
			if (oldTarget) oldTarget->SetCircleSize();
			// we have to make a backup, tr->Dialog is freed
			ieResRef tmpresref;
			strnlwrcpy(tmpresref,tr->Dialog, 8);
			/*if (target->GetInternalFlag()&IF_NOINT) {
				// this whole check moved out of InitDialog by fuzzie, see comments
				// for the IF_NOINT check in BeginDialog
				displaymsg->DisplayConstantString(STR_TARGETBUSY, DMC_RED);
				ta->SetMinRow( false );
				EndDialog();
				return;
			}*/
			if (!InitDialog( speaker, target, tmpresref)) {
				// error was displayed by InitDialog
				EndDialog();
				return false;
			}
		}
	}

	ds = dlg->GetState( si );
	if (!ds) {
		Log(WARNING, "Dialog", "Can't find next dialog");
		EndDialog();
		return false;
	}

	// displaying npc text and portrait
	const char *portrait = NULL;
	if (tgt) {
		portrait = tgt->GetPortrait(1);
	}
	if (portrait) {
		// dialog speaker pic
		ieResRef PortraitResRef;
		strnlwrcpy(PortraitResRef, portrait, 8);
		ResourceHolder<ImageMgr> im(PortraitResRef, true);
		if (im) {
			// we set the anim picture for the speaker to always be on the side during dialogue,
			// but also append the image to the TA so that it remains in the backlog.
			Sprite2D* image = im->GetSprite2D();
			ta->SetAnimPicture(image);

			// TODO: I would like to actually append the image as content to the TA
			// the TA supports this, but unfortunately we destroy the TA at the end of dialog
			// the TA that replaces it is created via ta->QueryText() on the old one so images are lost!
			//ta->AppendContent(new ImageSpan(image));
		}
	}
	ta->AppendText(L"\n");
	displaymsg->DisplayStringName( ds->StrRef, DMC_DIALOG, target, IE_STR_SOUND|IE_STR_SPEECH);

	int idx = 0;
	std::vector<SelectOption> dialogOptions;
	ControlEventHandler handler = NULL;
	//first looking for a 'continue' opportunity, the order is descending (a la IE)
	for (unsigned int x = 0; x < ds->transitionsCount; x++) {
		if (ds->transitions[x]->Flags & IE_DLG_TR_TRIGGER) {
			if (ds->transitions[x]->condition &&
				!ds->transitions[x]->condition->Evaluate(target)) {
				continue;
			}
		}
		idx++;
		if (ds->transitions[x]->textStrRef == 0xffffffff) {
			//dialogchoose should be set to x
			//it isn't important which END option was chosen, as it ends
			core->GetDictionary()->SetAt("DialogOption",x);
			if (ds->transitions[x]->Flags & IE_DLG_TR_TRIGGER
				&& ds->transitions[x]->condition
				&& !ds->transitions[x]->condition->Evaluate(target))
			{
				gc->SetDialogueFlags(DF_OPENENDWINDOW, BM_OR);
			} else {
				gc->SetDialogueFlags(DF_OPENCONTINUEWINDOW, BM_OR);
			}
		} else {
			String* string = core->GetString( ds->transitions[x]->textStrRef );
			dialogOptions.push_back(std::make_pair(x, *string));
			delete string;
		}
	}

	ta->SetSelectOptions(dialogOptions, true, &ColorRed, &ColorWhite, NULL);
	handler = new MethodCallback<DialogHandler, Control*>(this, &DialogHandler::DialogChoose);
	ta->SetEvent(IE_GUI_TEXTAREA_ON_SELECT, handler);

	// this happens if a trigger isn't implemented or the dialog is wrong
	if (!idx) {
		Log(WARNING, "Dialog", "There were no valid dialog options!");
		gc->SetDialogueFlags(DF_OPENENDWINDOW, BM_OR);
	}

	return true;
}

// TODO: duplicate of the one in GameControl
Actor *DialogHandler::GetActorByGlobalID(ieDword ID)
{
	if (!ID)
		return NULL;
	Game* game = core->GetGame();
	if (!game)
		return NULL;

	Map* area = game->GetCurrentArea( );
	if (!area)
		return NULL;
	return area->GetActorByGlobalID(ID);
}

Scriptable *DialogHandler::GetTarget()
{
	// TODO: area GetScriptableByGlobalID?

	if (!targetID) return NULL;

	Game *game = core->GetGame();
	if (!game) return NULL;

	Map *area = game->GetCurrentArea();
	if (!area) return NULL;

	Actor *actor = area->GetActorByGlobalID(targetID);
	if (actor) return actor;

	Door *door = area->GetDoorByGlobalID(targetID);
	if (door) return (Scriptable *)door;
	Container *container = area->GetContainerByGlobalID(targetID);
	if (container) return (Scriptable *)container;
	InfoPoint *ip = area->GetInfoPointByGlobalID(targetID);
	if (ip) return (Scriptable *)ip;

	return NULL;
}

Actor *DialogHandler::GetSpeaker()
{
	return GetActorByGlobalID(speakerID);
}


}
