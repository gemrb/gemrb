/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2012 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "MessageWindowLogger.h"

#include "DisplayMessage.h"
#include "GUI/GameControl.h"
#include "Interface.h"

namespace GemRB {

MessageWindowLogger* mwl = NULL;

MessageWindowLogger::MessageWindowLogger( log_level level)
	: Logger(level)
{
	PrintStatus(true);
}

MessageWindowLogger::~MessageWindowLogger()
{
	PrintStatus(false);
	assert(mwl == this);
	mwl = NULL;
}

void MessageWindowLogger::LogInternal(LogMessage&& msg)
{
	const GameControl* gc = core->GetGameControl();
	if (displaymsg && gc && !(gc->GetDialogueFlags()&DF_IN_DIALOG)) {
		// FIXME: we check DF_IN_DIALOG here to avoid recurssion in the MessageWindowLogger, but what happens when an error happens during dialog?
		// I'm not super sure about how to avoid that. for now the logger will not log anything in dialog mode.
		static const char* colors[] = {
			"[color=FFFFFF]",	// DEFAULT
			"[color=000000]",	// BLACK
			"[color=FF0000]",	// RED
			"[color=00FF00]",	// GREEN
			"[color=603311]",	// BROWN
			"[color=0000FF]",	// BLUE
			"[color=8B008B]",	// MAGENTA
			"[color=00CDCD]",	// CYAN
			"[color=FFFFFF]",	// WHITE
			"[color=CD5555]",	// LIGHT_RED
			"[color=90EE90]",	// LIGHT_GREEN
			"[color=FFFF00]",	// YELLOW
			"[color=BFEFFF]",	// LIGHT_BLUE
			"[color=FF00FF]",	// LIGHT_MAGENTA
			"[color=B4CDCD]",	// LIGHT_CYAN
			"[color=CDCDCD]"		// LIGHT_WHITE
		};
		static const log_color log_level_color[] = {
			RED,
			RED,
			YELLOW,
			LIGHT_WHITE,
			GREEN,
			BLUE
		};

		const wchar_t* fmt = L"%s%s: [/color]%s%s[/color]";
		size_t len = msg.message.length() + msg.owner.length() + wcslen(fmt) + 28; // 28 is for sizeof(colors[x]) * 2
		wchar_t* text = (wchar_t*)malloc(len * sizeof(wchar_t));
		swprintf(text, len, fmt, colors[msg.color], msg.owner.c_str(), colors[log_level_color[msg.level]], msg.message.c_str());
		displaymsg->DisplayMarkupString(text);
		free(text);
	}
}

void MessageWindowLogger::PrintStatus(bool toggle)
{
	if (toggle) {
		log(INTERNAL, "Logger", "MessageWindow logging active.", LIGHT_GREEN);
	} else {
		log(INTERNAL, "Logger", "MessageWindow logging disabled.", LIGHT_RED);
	}
}

// this MUST be a singleton. we only have one message window and multiple messages to that window are useless
// additionally GUIScript doesnt check its existance and simply calls this function whenever the command is given
Logger* getMessageWindowLogger( bool create )
{
	if (create && !mwl) {
		mwl = new MessageWindowLogger();
		AddLogger(mwl);
	}
	return mwl;
}

}
