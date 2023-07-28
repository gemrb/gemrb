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
#include "Interface.h"
#include "PluginMgr.h"
#include "ScriptEngine.h"
#include "TableMgr.h"
#include "Video/Video.h"
#include "GameScript/GameScript.h"
#include "GameScript/GSUtils.h"
#include "GUI/GameControl.h"
#include "GUI/TextArea.h"

namespace GemRB {

//translate section values (journal, quests, solved, user)
static const ieByte* sectionMap;
static const ieByte bg2Sections[4] = { 4, 1, 2, 0 };
static const ieByte noSections[4] = { 0, 0, 0, 0 };

// FIXME: arbitrary guess value
#define DIALOG_MOVE_SPEED 75

DialogHandler::DialogHandler(void)
{
	if (core->HasFeature(GFFlags::JOURNAL_HAS_SECTIONS)) {
		sectionMap = bg2Sections;
	} else {
		sectionMap = noSections;
	}
}

DialogHandler::~DialogHandler(void)
{
	delete dlg;
}

void DialogHandler::UpdateJournalForTransition(const DialogTransition* tr) const
{
	if (!tr || !(tr->Flags&IE_DLG_TR_JOURNAL)) return;

	int Section = 0;
	if (tr->Flags&IE_DLG_UNSOLVED) {
		Section |= 1; // quests
	}
	if (tr->Flags&IE_DLG_SOLVED) {
		Section |= 2; // completed
	}

	if (core->GetGame()->AddJournalEntry(tr->journalStrRef, sectionMap[Section], ieByte(tr->Flags >> 16))) {
		String msg(u"\n[color=bcefbc]");
		ieStrRef strJournalChange = DisplayMessage::GetStringReference(HCStrings::JournalChange);
		msg += core->GetString(strJournalChange);
		String str = core->GetString(tr->journalStrRef);
		if (!str.empty()) {
			//cutting off the strings at the first crlf
			size_t newlinePos = str.find_first_of(L'\n');
			if (newlinePos != String::npos) {
				str.resize( newlinePos );
			}
			msg += u" - [/color][p][color=ffd4a9]" + str + u"[/color][/p]";
		} else {
			msg += u"[/color]\n";
		}
		if (core->HasFeedback(FT_MISC)) {
			if (core->HasFeature(GFFlags::ONSCREEN_TEXT)) {
				core->GetGameControl()->SetDisplayText(HCStrings::JournalChange, 30);
			} else {
				displaymsg->DisplayMarkupString(msg);
			}
		}
		// pst also has a sound attached to the base string, so play it manually
		// NOTE: this doesn't display the string anywhere
		DisplayStringCore(core->GetGame(), strJournalChange, 0);
	}
}

//Try to start dialogue between two actors (one of them could be inanimate)
bool DialogHandler::InitDialog(Scriptable* spk, Scriptable* tgt, const ResRef& dialogRef, ieDword si)
{
	delete dlg;
	dlg = nullptr;

	if (dialogRef.IsEmpty() || IsStar(dialogRef)) {
		return false;
	}

	PluginHolder<DialogMgr> dm = GetImporter<DialogMgr>(IE_DLG_CLASS_ID);
	dm->Open(gamedata->GetResourceStream(dialogRef, IE_DLG_CLASS_ID));
	dlg = dm->GetDialog();

	if (!dlg) {
		Log(ERROR, "DialogHandler", "Cannot start dialog ({}): {} with {}", dialogRef, fmt::WideToChar{spk->GetName()}, fmt::WideToChar{tgt->GetName()});
		return false;
	}

	dlg->resRef = dialogRef; //this isn't handled by GetDialog???

	//target is here because it could be changed when a dialog runs onto
	//and external link, we need to find the new target (whose dialog was
	//linked to)

	Actor *oldTarget = GetLocalActorByGlobalID(targetID);
	speakerID = spk->GetGlobalID();
	targetID = tgt->GetGlobalID();
	if (!originalTargetID) originalTargetID = targetID;
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

	// iwd2 ignores conditions when following external references and
	// also just goes directly for the referenced state
	// look at 41cmolb1 and 41cmolb2 for an example
	// Actually bg2 is the same, misca vs imoenj (freeing minsc)
	if (initialState == -1) {
		initialState = dlg->FindFirstState(tgt);
	} else {
		if (originalTargetID == targetID) {
			initialState = dlg->FindFirstState(tgt);
			if (initialState < 0) {
				initialState = si;
			}
		} else {
			initialState = si;
		}
	}
	if (initialState < 0) {
		Log(DEBUG, "DialogHandler", "Could not find a proper state");
		return false;
	}

	core->ToggleViewsEnabled(false, "NOT_DLG");
	prevViewPortLoc = gc->Viewport().origin;
	gc->MoveViewportTo(tgt->Pos, true, DIALOG_MOVE_SPEED);

	//there are 3 bits, if they are all unset, the dialog freezes scripts
	// NOTE: besides marking pause/not pause, they determine what happens if
	// hostile actions are taken against the speaker from a EA < GOODCUTOFF creature:
	//Bit 0: Enemy()
	//Bit 1: EscapeArea()
	//Bit 2: nothing (but since the action was hostile, it behaves similar to bit 0)
	unsigned int flags = DF_IN_DIALOG;
	if (!(dlg->Flags&7) ) {
		flags |= DF_FREEZE_SCRIPTS;
	}
	gc->SetDialogueFlags(flags, BitOp::OR);
	return true;
}

/*try to break will only try to break it, false means unconditional stop*/
void DialogHandler::EndDialog(bool try_to_break)
{
	if (dlg == nullptr) {
		return; // no dialog, nothing to do.
	}

	// FIXME: is this useful for anything concrete? Currently never true, since nothing sets DF_UNBREAKABLE (unused since it was introduced)
	if (try_to_break && core->GetGameControl()->GetDialogueFlags() & DF_UNBREAKABLE) {
		return;
	}

	TextArea* ta = core->GetMessageTextArea();
	if (ta) {
		// reset the TA
		ta->SetSpeakerPicture(nullptr);
		ta->ClearSelectOptions();
	}

	Actor *tmp = GetSpeaker();
	Actor *target = Scriptable::As<Actor>(GetTarget());
	speakerID = 0;
	targetID = 0;
	originalTargetID = 0;

	if (tmp) {
		tmp->LeftDialog();
	}
	if (target) {
		target->LeftDialog();
		target->SetCircleSize();
	}
	ds = nullptr;
	delete dlg;
	dlg = nullptr;

	core->ToggleViewsEnabled(true, "NOT_DLG");
	// FIXME: it's not so nice having this here, but things call EndDialog directly :(
	core->GetGUIScriptEngine()->RunFunction( "GUIWORLD", "DialogEnded" );
	//restoring original size
	core->GetGame()->SetControlStatus(CS_DIALOG, BitOp::NAND);
	GameControl* gc = core->GetGameControl();
	gc->SetDialogueFlags(0, BitOp::SET);
	gc->MoveViewportTo(prevViewPortLoc, false, DIALOG_MOVE_SPEED);
	core->SetEventFlag(EF_PORTRAIT);
}

// TODO: work out if this should go somewhere more central (such
// as GetActorByDialog), or if there's a less awful way to do this
// (we could cache the entries, for example)
static Actor* FindBanter(const Scriptable* target, const ResRef& dialog)
{
	AutoTable pdtable = gamedata->LoadTable("interdia");
	if (!pdtable) return nullptr;

	TableMgr::index_t col;
	if (core->GetGame()->Expansion == GAME_TOB) {
		col = pdtable->GetColumnIndex("25FILE");
	} else {
		col = pdtable->GetColumnIndex("FILE");
	}
	TableMgr::index_t row = pdtable->FindTableValue(col, dialog);
	return target->GetCurrentArea()->GetActorByScriptName(pdtable->GetRowName(row));
}

static int GetDialogOptions(const DialogState *ds, std::vector<SelectOption>& options, Scriptable* target)
{
	int idx = 0;
	// first looking for a 'continue' opportunity, the order is descending (a la IE)
	for (int x = ds->transitionsCount - 1; x >= 0; x--) {
		if (ds->transitions[x]->Flags & IE_DLG_TR_TRIGGER) {
			if (ds->transitions[x]->condition && !ds->transitions[x]->condition->Evaluate(target)) {
				continue;
			}
		}

		idx++;
		if (ds->transitions[x]->textStrRef == ieStrRef::INVALID) {
			// dialogchoose should be set to x
			// it isn't important which END option was chosen, as it ends
			core->GetDictionary()["DialogOption"] = x;
			if (ds->transitions[x]->Flags & IE_DLG_TR_FINAL) {
				core->GetGameControl()->SetDialogueFlags(DF_OPENENDWINDOW, BitOp::OR);
				break;
			} else if (ds->transitions[x]->Flags & IE_DLG_TR_TRIGGER) {
				if (ds->transitions[x]->condition && !ds->transitions[x]->condition->Evaluate(target)) {
					continue;
				}
			}
			core->GetGameControl()->SetDialogueFlags(DF_OPENCONTINUEWINDOW, BitOp::OR);
			break;
		} else {
			options.emplace_back(x, core->GetString(ds->transitions[x]->textStrRef));
		}
	}

	std::reverse(options.begin(), options.end());
	return idx;
}

bool DialogHandler::DialogChoose(unsigned int choose)
{
	TextArea* ta = core->GetMessageTextArea();
	if (!ta) {
		Log(ERROR, "DialogHandler", "Dialog aborted???");
		EndDialog();
		return false;
	}

	Actor *speaker = GetSpeaker();
	if (!speaker) {
		Log(ERROR, "DialogHandler", "Speaker gone???");
		EndDialog();
		return false;
	}

	Scriptable *target = GetTarget();
	if (!target) {
		Log(ERROR, "DialogHandler", "Target gone???");
		EndDialog();
		return false;
	}
	Scriptable *tgt = nullptr;
	Actor *tgta = nullptr;
	if (target->Type == ST_ACTOR) {
		tgta = (Actor *)target;
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

		if (tgta) {
			if (gc->GetDialogueFlags()&DF_TALKCOUNT) {
				gc->SetDialogueFlags(DF_TALKCOUNT, BitOp::NAND);
				tgta->TalkCount++;
			} else if (gc->GetDialogueFlags()&DF_INTERACT) {
				gc->SetDialogueFlags(DF_INTERACT, BitOp::NAND);
				tgta->InteractCount++;
			}
		}
		// does this belong here? we must clear actions somewhere before
		// we start executing them (otherwise queued actions interfere)
		// executing actions directly does not work, because dialog
		// needs to end before final actions are executed due to
		// actions making new dialogs!
		if (!(target->GetInternalFlag() & IF_NOINT)) {
			target->Stop();
		}
	} else {
		if (!ds || ds->transitionsCount <= choose) {
			return false;
		}

		DialogTransition* tr = ds->transitions[choose];
		UpdateJournalForTransition(tr);
		if (tr->textStrRef != ieStrRef::INVALID) {
			//allow_zero is for PST (deionarra's text)
			ta->AppendText(u"\n");
			displaymsg->DisplayStringName( tr->textStrRef, GUIColors::DIALOGPARTY, speaker, STRING_FLAGS::SOUND | STRING_FLAGS::SPEECH | STRING_FLAGS::ALLOW_ZERO);
		}
		target->ImmediateEvent();
		target->ProcessActions(); //run the action queue now

		if (!tr->actions.empty()) {
			if (!(target->GetInternalFlag() & IF_NOINT)) {
				target->ReleaseCurrentAction();
			}

			// do not interrupt during dialog actions (needed for aerie.d polymorph block)
			target->AddAction( GenerateAction( "SetInterrupt(FALSE)" ) );
			// delay all other actions until the next cycle (needed for the machine of Lum the Mad (gorlum2.dlg))
			// FIXME: figure out if pst needs something similar (action missing)
			//        (not conditional on GenerateAction to prevent console spam)
			// iwd2 41nate.d breaks if this is included, since the original delayed execution in a different manner
			if (!core->HasFeature(GFFlags::AREA_OVERRIDE) && !core->HasFeature(GFFlags::RULES_3ED) && !(tr->Flags & IE_DLG_IMMEDIATE)) {
				target->AddAction(GenerateAction("BreakInstants()"));
			}
			for (unsigned int i = 0; i < tr->actions.size(); i++) {
				if (i == tr->actions.size() - 1) tr->actions[i]->flags |= ACF_REALLOW_SCRIPTS;
				target->AddAction(tr->actions[i]);
			}
			target->AddAction( GenerateAction( "SetInterrupt(TRUE)" ) );
		}

		if (tr->Flags & IE_DLG_TR_FINAL) {
			if (!tr->actions.empty()) gc->SetDialogueFlags(DF_POSTPONE_SCRIPTS, BitOp::OR);
			EndDialog();
			ta->AppendText(u"\n");
			return false;
		}

		// avoid problems when dhjollde.dlg tries starting a cutscene in the middle of a dialog
		// (it seems harmless doing it in non-HoW too, since other versions would just break in such a situation)
		core->SetCutSceneMode( false );

		//displaying dialog for selected option
		si = tr->stateIndex;
		//follow external linkage, if required
		if (!tr->Dialog.IsEmpty() && tr->Dialog != dlg->resRef) {
			//target should be recalculated!
			target->LeftDialog();
			tgt = nullptr;
			tgta = nullptr;
			if (originalTargetID) {
				// always try original target first (sometimes there are multiple
				// actors with the same dialog in an area, we want to pick the one
				// we were talking to)
				tgta = GetLocalActorByGlobalID(originalTargetID);
				if (tgta && tgta->GetDialog(GD_NORMAL) != tr->Dialog) {
					tgta = nullptr;
				} else {
					tgt = tgta;
				}
			}
			if (!tgt) {
				// then just search the current area for an actor with the dialog
				tgt = target->GetCurrentArea()->GetScriptableByDialog(tr->Dialog);
				if (tgt && tgt->Type == ST_ACTOR) {
					tgta = (Actor *) tgt;
				}
			}
			if (!tgt) {
				// try searching for banter dialogue: the original engine seems to
				// happily let you randomly switch between normal and banter dialogs
				tgta = FindBanter(target, tr->Dialog);
				tgt = tgta;
			}
			// pst: check if we're carrying any items with the needed dialog (eg. mertwyn's head)
			if (!tgt && core->HasFeature(GFFlags::AREA_OVERRIDE)) {
				tgta = target->GetCurrentArea()->GetItemByDialog(tr->Dialog);
				tgt = tgta;
			}

			if (!tgt) {
				Log(WARNING, "DialogHandler", "Can't redirect dialog");
				EndDialog();
				return false;
			}
			Actor *oldTarget = GetLocalActorByGlobalID(targetID);
			targetID = tgt->GetGlobalID();
			if (tgta) tgta->SetCircleSize();
			if (oldTarget) oldTarget->SetCircleSize();
			target = tgt;

			// we have to make a backup, tr->Dialog is freed
			ResRef tmpresref = tr->Dialog;
			if (!InitDialog(speaker, target, tmpresref, si)) {
				// error was displayed by InitDialog
				EndDialog();
				return false;
			}
		}
	}

	ds = dlg->GetState( si );
	if (!ds) {
		Log(WARNING, "DialogHandler", "Can't find next dialog");
		EndDialog();
		return false;
	}

	if (tgta) {
		// displaying npc text and portrait
		Holder<Sprite2D> portrait = tgta->CopyPortrait(1);
		ta->SetSpeakerPicture(std::move(portrait));
		ta->AppendText(u"\n");
		displaymsg->DisplayStringName( ds->StrRef, GUIColors::DIALOG, target, STRING_FLAGS::SOUND | STRING_FLAGS::SPEECH);
	}

	std::vector<SelectOption> dialogOptions;
	int idx = GetDialogOptions(ds, dialogOptions, target);
	ta->SetSelectOptions(dialogOptions, true);
	ControlEventHandler handler = [this](const Control* c) {
		DialogChoose(c->GetValue());
	};
	ta->SetAction(std::move(handler), TextArea::Action::Select);

	// this happens if a trigger isn't implemented or the dialog is wrong
	if (!idx) {
		Log(WARNING, "DialogHandler", "There were no valid dialog options!");
		gc->SetDialogueFlags(DF_OPENENDWINDOW, BitOp::OR);
	}

	return true;
}

Actor *DialogHandler::GetLocalActorByGlobalID(ieDword ID)
{
	if (!ID) return nullptr;

	const Game* game = core->GetGame();
	if (!game) return nullptr;

	const Map* area = game->GetCurrentArea();
	if (!area) return nullptr;

	return area->GetActorByGlobalID(ID);
}

Scriptable *DialogHandler::GetTarget() const
{
	const Game *game = core->GetGame();
	if (!game) return nullptr;

	Map *area = game->GetCurrentArea();
	if (!area) return nullptr;

	return area->GetScriptableByGlobalID(targetID);
}

Actor *DialogHandler::GetSpeaker()
{
	return GetLocalActorByGlobalID(speakerID);
}

}
