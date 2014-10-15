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
		Log(ERROR, "DialogHandler", "Cannot start dialog: %s", dlgref);
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
	//allow mouse selection from dialog (even though screen is locked)
	video->SetMouseEnabled(true);
	gc->MoveViewportTo(tgt->Pos.x, tgt->Pos.y, true);

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


void DialogHandler::DialogChoose(unsigned int choose)
{
	TextArea* ta = core->GetMessageTextArea();
	if (!ta) {
		Log(ERROR, "GameControl", "Dialog aborted???");
		EndDialog();
		return;
	}

	Actor *speaker = GetSpeaker();
	if (!speaker) {
		Log(ERROR, "GameControl", "Speaker gone???");
		EndDialog();
		return;
	}

	Scriptable *target = GetTarget();
	if (!target) {
		Log(ERROR, "GameControl", "Target gone???");
		EndDialog();
		return;
	}
	Actor *tgt = NULL;
	if (target->Type == ST_ACTOR) {
		tgt = (Actor *)target;
	}

	int si;
	if (choose == (unsigned int) -1) {
		//increasing talkcount after top level condition was determined

		si = initialState;
		if (si<0) {
			EndDialog();
			return;
		}

		if (tgt) {
			if (core->GetGameControl()->GetDialogueFlags()&DF_TALKCOUNT) {
				core->GetGameControl()->SetDialogueFlags(DF_TALKCOUNT, BM_NAND);
				tgt->TalkCount++;
			} else if (core->GetGameControl()->GetDialogueFlags()&DF_INTERACT) {
				core->GetGameControl()->SetDialogueFlags(DF_INTERACT, BM_NAND);
				tgt->InteractCount++;
			}
		}
		// does this belong here? we must clear actions somewhere before
		// we start executing them (otherwise queued actions interfere)
		// executing actions directly does not work, because dialog
		// needs to end before final actions are executed due to
		// actions making new dialogs!
		target->Stop();
	} else {
		if (!ds || ds->transitionsCount <= choose) {
			return;
		}

		DialogTransition* tr = ds->transitions[choose];

		ta->PopMinRow();

		if (tr->Flags&IE_DLG_TR_JOURNAL) {
			int Section = 0;
			if (tr->Flags&IE_DLG_UNSOLVED) {
				Section |= 1;
			}
			if (tr->Flags&IE_DLG_SOLVED) {
				Section |= 2;
			}
			if (core->GetGame()->AddJournalEntry(tr->journalStrRef, sectionMap[Section], tr->Flags>>16) ) {
				displaymsg->DisplayConstantString(STR_JOURNALCHANGE, DMC_BG2XPGREEN);
				char *string = core->GetString( tr->journalStrRef );
				//cutting off the strings at the first crlf
				char *poi = strchr(string,'\n');
				if (poi) {
					*poi='\0';
				}
				displaymsg->DisplayString( string );
				free( string );
			}
		}

		if (tr->textStrRef != 0xffffffff) {
			//allow_zero is for PST (deionarra's text)
			displaymsg->DisplayStringName( (int) (tr->textStrRef), DMC_DIALOGPARTY, speaker, IE_STR_SOUND|IE_STR_SPEECH|IE_STR_ALLOW_ZERO);
			if (core->HasFeature( GF_DIALOGUE_SCROLLS )) {
				ta->AppendText( "", -1 );
			}
		}
		target->ImmediateEvent();
		target->ProcessActions(); //run the action queue now

		if (tr->actions.size()) {
			if (!(target->GetInternalFlag() & IF_NOINT)) {
				target->ReleaseCurrentAction();
			}

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
			ta->SetMinRow( false );
			EndDialog();
		}

		if (final_dialog) {
			return;
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
			if (!tgt) {
				Log(WARNING, "Dialog", "Can't redirect dialog");
				ta->SetMinRow( false );
				EndDialog();
				return;
			}
			Actor *oldTarget = GetActorByGlobalID(targetID);
			targetID = tgt->GetGlobalID();
			tgt->SetCircleSize();
			if (oldTarget) oldTarget->SetCircleSize();
			if (target != tgt) {
				// switching target; clear actions
				target = tgt;
				target->Stop();
			}
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
				ta->SetMinRow( false );
				EndDialog();
				return;
			}
		}
	}

	ds = dlg->GetState( si );
	if (!ds) {
		Log(WARNING, "Dialog", "Can't find next dialog");
		ta->SetMinRow( false );
		EndDialog();
		return;
	}

	//displaying npc text
	displaymsg->DisplayStringName( ds->StrRef, DMC_DIALOG, target, IE_STR_SOUND|IE_STR_SPEECH);
	//adding a gap between options and npc text
	ta->AppendText("",-1);
	int i;
	int idx = 0;
	ta->SetMinRow( true );
	//first looking for a 'continue' opportunity, the order is descending (a la IE)
	unsigned int x = ds->transitionsCount;
	while(x--) {
		if (ds->transitions[x]->Flags & IE_DLG_TR_FINAL) {
			continue;
		}
		if (ds->transitions[x]->textStrRef != 0xffffffff) {
			continue;
		}
		if (ds->transitions[x]->Flags & IE_DLG_TR_TRIGGER) {
			if (ds->transitions[x]->condition &&
				!ds->transitions[x]->condition->Evaluate(target)) {
				continue;
			}
		}
		core->GetDictionary()->SetAt("DialogOption",x);
		core->GetGameControl()->SetDialogueFlags(DF_OPENCONTINUEWINDOW, BM_OR);
		goto end_of_choose;
	}
	for (x = 0; x < ds->transitionsCount; x++) {
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
			core->GetGameControl()->SetDialogueFlags(DF_OPENENDWINDOW, BM_OR);
		} else {
			char *string = ( char * ) malloc( 40 );
			sprintf( string, "[s=%d,ffffff,ff0000]%d - [p]", x, idx );
			i = ta->AppendText( string, -1 );
			free( string );
			string = core->GetString( ds->transitions[x]->textStrRef );
			ta->AppendText( string, i );
			free( string );
			ta->AppendText( "[/p][/s]", i );
		}
	}
	// this happens if a trigger isn't implemented or the dialog is wrong
	if (!idx) {
		Log(WARNING, "Dialog", "There were no valid dialog options!");
		core->GetGameControl()->SetDialogueFlags(DF_OPENENDWINDOW, BM_OR);
	}
end_of_choose:
	//padding the rows so our text will be at the top
	if (core->HasFeature( GF_DIALOGUE_SCROLLS )) {
		ta->AppendText( "", -1 );
	}
	else {
		ta->PadMinRow();
	}
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
	Game *game = core->GetGame();
	if (!game) return NULL;

	Map *area = game->GetCurrentArea();
	if (!area) return NULL;

	return area->GetScriptableByGlobalID(targetID);
}

Actor *DialogHandler::GetSpeaker()
{
	return GetActorByGlobalID(speakerID);
}


}
