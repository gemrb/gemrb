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

MessageWindowLogger::MessageWindowLogger()
{}

MessageWindowLogger::~MessageWindowLogger()
{
	assert(mwl == this);
	mwl = NULL;
}

void MessageWindowLogger::log(log_level level, const char* owner, const char* message, log_color color)
{
	if (displaymsg) {
		GameControl* gc = core->GetGameControl();
		if (level < MESSAGE && gc && !(gc->GetDialogueFlags()&DF_IN_DIALOG)) {
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
				"[color=CDCDCD]"		// LIGHT_WHITE / GREY
			};
			static log_color log_level_color[] = {
				RED,
				RED,
				YELLOW,
				LIGHT_WHITE,
				GREEN,
				BLUE
			};

			const char* fmt = "%s%s: [/color]%s%s[/color]";
			char* msg = (char*)malloc(strlen(message) + strlen(owner) + 45);
			sprintf(msg, fmt, colors[color], owner, colors[log_level_color[level]], message);
			displaymsg->DisplayString(msg);
			free(msg);
		}
	}
}

// this MUST be a singleton. we only have one message window and multiple messages to that window are useless
// additionally GUIScript doesnt check its existance and simply calls this function whenever the command is given
Logger* createMessageWindowLogger()
{
	if (!mwl) {
		mwl = new MessageWindowLogger();
	}
	return mwl;
}

Logger* getMessageWindowLogger()
{
	return mwl;
}

}
