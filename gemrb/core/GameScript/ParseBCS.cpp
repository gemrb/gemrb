/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2024 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

// Parser of encoded B(C)S scripts

#include "GSUtils.h"
#include "GameScript.h"

#include "Strings/String.h"

#include <cctype>

namespace GemRB {

static int ParseInt(const char*& src)
{
	char number[33];

	char* tmp = number;
	int i = 1;
	while (isdigit(*src) || *src == '-') {
		if (i == 33) {
			Log(ERROR, "GameScript", "Truncating too big integer!");
			break;
		}
		*tmp = *src;
		tmp++;
		src++;
		i++;
	}
	*tmp = 0;
	if (*src) src++;
	return atoi(number);
}

static void ParseString(const char*& src, char* tmp)
{
	while (*src != '"' && *src) {
		*tmp = *src;
		tmp++;
		src++;
	}
	*tmp = 0;
	if (*src) src++;
}

static Object* DecodeObject(const std::string& line)
{
	const char* cursor = line.c_str();

	Object* oB = new Object();
	for (int i = 0; i < ObjectFieldsCount; i++) {
		oB->objectFields[i] = ParseInt(cursor);
	}
	for (int i = 0; i < MaxObjectNesting; i++) {
		oB->objectFilters[i] = ParseInt(cursor);
	}

	// iwd tolerates the missing rectangle, so we do so too
	if (HasAdditionalRect && (*cursor == '[')) {
		cursor++; // skip [
		int tmp[4];
		for (int& i : tmp) {
			i = ParseInt(cursor);
		}
		oB->objectRect = Region(tmp[0], tmp[1], tmp[2] - tmp[0], tmp[3] - tmp[1]);
		if (*cursor == ' ')
			cursor++; // skip ] (not really... it skips a ' ' since the ] was skipped by the ParseInt function
	}

	if (*cursor == '"') {
		cursor++; // skip "
	}

	ParseString(cursor, oB->objectName.begin());
	// HACK for iwd2 AddExperiencePartyCR
	if (oB->objectName == "0.0.0.0 ") {
		oB->objectName.Reset();
		Log(DEBUG, "GameScript", "overriding: +{}+", oB->objectName);
	}

	if (*cursor == '"')
		cursor++; // skip " (the same as above)
	// this seems to be needed too
	if (ExtraParametersCount && *cursor) {
		cursor++;
	}

	for (int i = 0; i < ExtraParametersCount; i++) {
		oB->objectFields[i + ObjectFieldsCount] = ParseInt(cursor);
	}
	if (*cursor != 'O' || *(cursor + 1) != 'B') {
		Log(WARNING, "GameScript", "Got confused parsing object line: {}", line);
	}
	// let the object realize it has no future (in case of null objects)
	if (oB->isNull()) {
		oB->Release();
		return nullptr;
	}
	return oB;
}

static Trigger* ReadTrigger(DataStream* stream)
{
	std::string line;
	stream->ReadLine(line);
	if (line.compare(0, 2, "TR") != 0) {
		return nullptr;
	}

	stream->ReadLine(line);
	Trigger* tR = new Trigger();
	// this exists only in PST?
	if (HasTriggerPoint) {
		sscanf(line.data(), R"(%hu %d %d %d %d [%d,%d] "%[^"]" "%[^"]" OB)",
		       &tR->triggerID, &tR->int0Parameter, &tR->flags,
		       &tR->int1Parameter, &tR->int2Parameter, &tR->pointParameter.x,
		       &tR->pointParameter.y, tR->string0Parameter.begin(), tR->string1Parameter.begin());
	} else {
		sscanf(line.data(), R"(%hu %d %d %d %d "%[^"]" "%[^"]" OB)",
		       &tR->triggerID, &tR->int0Parameter, &tR->flags,
		       &tR->int1Parameter, &tR->int2Parameter, tR->string0Parameter.begin(),
		       tR->string1Parameter.begin());
	}
	StringToLower(tR->string0Parameter);
	StringToLower(tR->string1Parameter);
	tR->triggerID &= 0x3fff;

	stream->ReadLine(line);
	tR->objectParameter = DecodeObject(line);
	if (triggerflags[tR->triggerID] & TF_HAS_OBJECT && !tR->objectParameter) tR->flags |= TF_MISSING_OBJECT;
	tR->flags |= TF_PRECOMPILED;

	stream->ReadLine(line);
	// discard invalid triggers, so they won't cause a crash
	if (tR->triggerID >= MAX_TRIGGERS) {
		delete tR;
		return nullptr;
	}
	return tR;
}

static Condition* ReadCondition(DataStream* stream)
{
	std::string line;
	stream->ReadLine(line, 10);
	if (line.compare(0, 2, "CO") != 0) {
		return nullptr;
	}

	Condition* cO = new Condition();
	Object* triggerer = nullptr;
	while (true) {
		Trigger* tR = ReadTrigger(stream);
		if (!tR) {
			if (triggerer) delete triggerer;
			break;
		}

		// handle NextTriggerObject
		/* Defines the object that the next trigger will be evaluated in reference to. This trigger
		 * does not evaluate and does not count as a trigger in an OR() block. This trigger ignores
		 * the Eval() trigger when finding the next trigger to evaluate the object for. If the object
		 * cannot be found, the next trigger will evaluate to false.
		 */
		if (triggerer) {
			delete tR->objectParameter; // not using Release, so we don't have to check if it's null
			tR->objectParameter = triggerer;
			triggerer = nullptr;
		} else if (tR->triggerID == NextTriggerObjectID) {
			triggerer = tR->objectParameter;
			tR->objectParameter = nullptr;
			delete tR;
			continue;
		}

		cO->triggers.push_back(tR);
	}
	return cO;
}

ResponseBlock* GameScript::ReadResponseBlock(DataStream* stream)
{
	std::string line;
	stream->ReadLine(line, 10);
	if (line.compare(0, 2, "CR") != 0) {
		return nullptr;
	}

	ResponseBlock* rB = new ResponseBlock();
	rB->condition = ReadCondition(stream);
	rB->responseSet = ReadResponseSet(stream);
	return rB;
}

ResponseSet* GameScript::ReadResponseSet(DataStream* stream)
{
	std::string line;
	stream->ReadLine(line, 10);
	if (line.compare(0, 2, "RS") != 0) {
		return nullptr;
	}

	ResponseSet* rS = new ResponseSet();
	while (true) {
		Response* rE = ReadResponse(stream);
		if (!rE) break;
		rS->responses.push_back(rE);
	}
	return rS;
}

// this is the border of the GameScript object (all subsequent functions are library functions)
// we can't make this a library function, because scriptlevel is set here
Response* GameScript::ReadResponse(DataStream* stream)
{
	std::string line;
	stream->ReadLine(line);
	if (line.compare(0, 2, "RE") != 0) {
		return nullptr;
	}

	Response* rE = new Response();
	rE->weight = 0;
	stream->ReadLine(line, 1024);
	char* poi;
	rE->weight = strtounsigned<uint8_t>(line.c_str(), &poi, 10);
	if (strncmp(poi, "AC", 2) != 0) {
		return rE;
	}

	while (true) {
		// not autofreed, because it is referenced by the Script
		Action* aC = new Action(false);
		stream->ReadLine(line, 1024);
		aC->actionID = strtounsigned<uint16_t>(line.c_str(), nullptr, 10);
		for (int i = 0; i < 3; i++) {
			stream->ReadLine(line, 1024);
			Object* oB = DecodeObject(line);
			aC->objects[i] = oB;
			if (i != 2) {
				stream->ReadLine(line, 1024);
			}
		}

		stream->ReadLine(line);
		sscanf(line.data(), R"(%d %d %d %d %d"%[^"]" "%[^"]" AC)",
		       &aC->int0Parameter, &aC->pointParameter.x, &aC->pointParameter.y,
		       &aC->int1Parameter, &aC->int2Parameter, aC->string0Parameter.begin(),
		       aC->string1Parameter.begin());
		StringToLower(aC->string0Parameter);
		StringToLower(aC->string1Parameter);
		if (aC->actionID >= MAX_ACTIONS) {
			aC->actionID = 0;
			Log(ERROR, "GameScript", "Invalid script action ID!");
		} else {
			if (actionflags[aC->actionID] & AF_SCRIPTLEVEL) {
				// can't set this here, because the same script may be loaded
				// into different slots. Overwriting it with an invalid value
				// just to find bugs faster
				aC->int0Parameter = -1;
			}
		}
		if (actionflags[aC->actionID] & AF_HAS_OBJECT && !aC->objects[0] && !aC->objects[1]) {
			aC->flags |= ACF_MISSING_OBJECT;
		}
		aC->flags |= ACF_PRECOMPILED;

		rE->actions.push_back(aC);

		stream->ReadLine(line);
		if (line.compare(0, 2, "RE") == 0) {
			break;
		}
	}
	return rE;
}

}
