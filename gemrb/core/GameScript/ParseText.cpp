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

// Parser of plain text scripts as found in dialogs or used internally

#include "globals.h"

#include "GSUtils.h"
#include "GameScript.h"
#include "Interface.h"

namespace GemRB {


// we need this because some special characters like _ or * are also accepted
inline bool IsMySymbol(const char letter)
{
	if (letter == ']') return false;
	if (letter == '[') return false;
	if (letter == ')') return false;
	if (letter == '(') return false;
	if (letter == '.') return false;
	if (letter == ',') return false;
	return true;
}

// this function returns a value, symbol could be a numeric string or
// a symbol from idsname
static int GetIdsValue(const char*& symbol, const ResRef& idsName)
{
	char* newsymbol;
	int value = strtosigned<int>(symbol, &newsymbol);
	if (symbol != newsymbol) {
		symbol = newsymbol;
		return value;
	}

	int idsFile = core->LoadSymbol(idsName);
	auto valHook = core->GetSymbol(idsFile);
	if (!valHook) {
		Log(ERROR, "GameScript", "Missing IDS file {} for symbol {}!", idsName, symbol);
		return -1;
	}

	std::string symbolName(64, '\0');
	for (int x = 0; IsMySymbol(*symbol) && x < 63; x++) {
		symbolName[x] = *symbol;
		symbol++;
	}
	if (symbolName.substr(0, 6) == "anyone") return 0;
	return valHook->GetValue(symbolName);
}

static int ParseIntParam(const char*& src, const char*& str)
{
	// going to the variable name
	while (*str != '*' && *str != ',' && *str != ')') {
		str++;
	}
	if (*str == '*') { // there may be an IDS table
		str++;
		ResRef idsTabName;
		char* cur = idsTabName.begin();
		const char* end = idsTabName.bufend();
		while (cur != end && *str != ',' && *str != ')') {
			*cur = *str;
			++cur;
			++str;
		}

		if (idsTabName[0]) {
			return GetIdsValue(src, idsTabName);
		}
	}
	// no IDS table
	return strtosigned<int>(src, const_cast<char**>(&src));
}

static void ParseIdsTarget(const char*& src, Object*& object)
{
	for (int i = 0; i < DialogObjectIDSCount; i++) {
		int fieldIndex = DialogObjectIDSOrder[i];
		object->objectFields[fieldIndex] = GetIdsValue(src, DialogObjectIDSTableNames[i]);
		if (*src != '.') {
			break;
		}
		src++;
	}
	src++; // skipping ]
}

// this will skip to the next element in the prototype of an action/trigger
#define SKIP_ARGUMENT() \
	while (*str && (*str != ',') && (*str != ')')) str++

static void ParseObject(const char*& str, const char*& src, Object*& object)
{
	SKIP_ARGUMENT();
	object = new Object();
	switch (*src) {
		case ')':
			// missing parameter
			// (for example, StartDialogueNoSet() in aerie.d)
			Log(WARNING, "GSUtils", "ParseObject expected an object when parsing dialog");
			// replace with Myself
			object->objectFilters[0] = 1;
			break;
		case '"':
			// Scriptable name
			src++;
			uint8_t i;
			for (i = 0; i < static_cast<uint8_t>(sizeof(object->objectName) - 1) && *src && *src != '"'; i++) {
				object->objectName[i] = *src;
				src++;
			}
			object->objectName[i] = 0;
			src++;
			break;
		case '[':
			src++; // skipping [
			ParseIdsTarget(src, object);
			break;
		default: // nested object filters
			int Nesting = 0;

			while (Nesting < MaxObjectNesting) {
				memmove(object->objectFilters + 1, object->objectFilters, (int) sizeof(int) * (MaxObjectNesting - 1));
				object->objectFilters[0] = GetIdsValue(src, "object");
				if (*src != '(') {
					break;
				}
				src++; // skipping (
				if (*src == ')') {
					src++;
					break;
				}
				Nesting++;
			}
			if (*src == '[') {
				ParseIdsTarget(src, object);
			}
			src += Nesting; // skipping )
	}
}

// some iwd2 dialogs use # instead of " for delimiting parameters (11phaen, 30gobpon, 11oswald)
// BUT at the same time, some bg2 mod prefixes use it too (eg. Tashia)
inline bool IsParamDelimiter(const char* src)
{
	if (*src == '"') return true;

	if (*src != '#') return false;
	return *(src - 1) == '(' || *(src - 1) == ',' || *(src + 1) == ')';
}

// this function was lifted from GenerateAction, to make it clearer
Action* GenerateActionCore(const char* src, const char* str, unsigned short actionID)
{
	Action* newAction = new Action(true);
	newAction->actionID = actionID;
	// this flag tells us to merge 2 consecutive strings together to get
	// a variable (context+variablename)
	int mergeStrings = actionflags[newAction->actionID] & AF_MERGESTRINGS;
	int objectCount = (newAction->actionID == 1) ? 0 : 1; // only object 2 and 3 are used by actions, 1 being reserved for ActionOverride
	int stringsCount = 0;
	int intCount = 0;
	if (actionflags[newAction->actionID] & AF_DIRECT) {
		Object* tmp = new Object();
		tmp->objectFields[0] = -1;
		newAction->objects[objectCount++] = tmp;
	}
	// here is the Action; now we need to evaluate the parameters, if any
	if (*str != ')')
		while (*str) {
			if (*(str + 1) != ':') {
				Log(WARNING, "GSUtils", "parser was sidetracked: {}", str);
			}
			switch (*str) {
				default:
					Log(WARNING, "GSUtils", "Invalid type: {}", str);
					delete newAction;
					return nullptr;

				case 'p': // Point
					SKIP_ARGUMENT();
					src++; // skip [
					newAction->pointParameter.x = strtosigned<int>(src, const_cast<char**>(&src), 10);
					src++; // skip .
					newAction->pointParameter.y = strtosigned<int>(src, const_cast<char**>(&src), 10);
					src++; // skip ]
					break;

				case 'i': // Integer
					{
						int value = ParseIntParam(src, str);
						if (intCount == 0) {
							newAction->int0Parameter = value;
						} else if (intCount == 1) {
							newAction->int1Parameter = value;
						} else {
							newAction->int2Parameter = value;
						}
						intCount++;
					}
					break;

				case 'a':
					// Action - only ActionOverride takes such a parameter
					{
						SKIP_ARGUMENT();
						std::string action(257, '\0');
						int i = 0;
						int openParenthesisCount = 0;
						while (true) {
							if (*src == ')') {
								if (!openParenthesisCount) {
									break;
								}
								openParenthesisCount--;
							} else {
								if (*src == '(') {
									openParenthesisCount++;
								} else {
									if ((*src == ',') && !openParenthesisCount) {
										break;
									}
								}
							}
							action[i] = *src;
							i++;
							src++;
						}
						Action* act = GenerateAction(std::move(action));
						if (!act) {
							delete newAction;
							return nullptr;
						}
						act->objects[0] = newAction->objects[0];
						newAction->objects[0] = nullptr; // avoid freeing of object
						delete newAction; // freeing action
						newAction = act;
					}
					break;

				case 'o': // Object
					if (objectCount == 3) {
						Log(ERROR, "GSUtils", "Invalid object count!");
						delete newAction;
						return nullptr;
					}
					ParseObject(str, src, newAction->objects[objectCount++]);
					break;

				case 's': // String
					{
						SKIP_ARGUMENT();
						src++;
						int i;
						char* dst;
						if (!stringsCount) {
							dst = newAction->string0Parameter.begin();
						} else {
							dst = newAction->string1Parameter.begin();
						}
						// if there are 3 strings, the first 2 will be merged,
						// the last one will be left alone
						if (*str == ')') {
							mergeStrings = 0;
						}
						// skipping the context part, which
						// is to be readded later
						if (mergeStrings) {
							for (i = 0; i < 6; i++) {
								*dst++ = '*';
							}
						} else {
							i = 0;
						}
						// breaking on ',' in case of a monkey attack
						// fixes bg1:melicamp.dlg, bg1:sharte.dlg, bg2:udvith.dlg
						// NOTE: if strings ever need a ',' inside, this is will need to change
						while (*src != ',' && !IsParamDelimiter(src)) {
							if (*src == 0) {
								delete newAction;
								return nullptr;
							}
							// sizeof(context+name) = 40
							if (i < 40) {
								*dst++ = (char) tolower(*src);
								i++;
							}
							src++;
						}
						if (*src == '"' || *src == '#') {
							src++;
						}
						*dst = 0;
						// reading the context part
						if (mergeStrings) {
							str++;
							if (*str != 's') {
								Log(ERROR, "GSUtils", "Invalid mergestrings: {}", str);
								delete newAction;
								return nullptr;
							}
							SKIP_ARGUMENT();
							if (!stringsCount) {
								dst = newAction->string0Parameter.begin();
							} else {
								dst = newAction->string1Parameter.begin();
							}

							// this works only if there are no spaces
							if (*src++ != ',' || *src++ != '"') {
								break;
							}
							// reading the context string
							i = 0;
							while (*src != '"') {
								if (*src == 0) {
									delete newAction;
									return nullptr;
								}
								if (i++ < 6) {
									*dst++ = (char) tolower(*src);
								}
								src++;
							}
							src++; // skipping "
						}
						stringsCount++;
					}
					break;
			}
			str++;
			if (*src == ',' || *src == ')') {
				src++;
			}
		}
	return newAction;
}


Trigger* GenerateTriggerCore(const char* src, const char* str, int trIndex, int negate)
{
	Trigger* newTrigger = new Trigger();
	newTrigger->triggerID = (unsigned short) triggersTable->GetValueIndex(trIndex) & 0x3fff;
	newTrigger->flags = (unsigned short) negate;
	int mergeStrings = triggerflags[newTrigger->triggerID] & TF_MERGESTRINGS;
	int stringsCount = 0;
	int intCount = 0;
	// here is the Trigger; now we need to evaluate the parameters
	if (*str != ')')
		while (*str) {
			if (*(str + 1) != ':') {
				Log(WARNING, "GSUtils", "parser was sidetracked: {}", str);
			}
			switch (*str) {
				default:
					Log(ERROR, "GSUtils", "Invalid type: {}", str);
					delete newTrigger;
					return nullptr;

				case 'p': // Point
					SKIP_ARGUMENT();
					src++; // skip [
					newTrigger->pointParameter.x = strtosigned<int>(src, const_cast<char**>(&src), 10);
					src++; // skip .
					newTrigger->pointParameter.y = strtosigned<int>(src, const_cast<char**>(&src), 10);
					src++; // skip ]
					break;

				case 'i': // Integer
					{
						int value = ParseIntParam(src, str);
						if (intCount == 0) {
							newTrigger->int0Parameter = value;
						} else if (intCount == 1) {
							newTrigger->int1Parameter = value;
						} else {
							newTrigger->int2Parameter = value;
						}
						intCount++;
					}
					break;

				case 'o': // Object
					ParseObject(str, src, newTrigger->objectParameter);
					break;

				case 's': // String
					{
						SKIP_ARGUMENT();
						src++;
						int i;
						char* dst;
						if (!stringsCount) {
							dst = newTrigger->string0Parameter.begin();
						} else {
							dst = newTrigger->string1Parameter.begin();
						}
						// skipping the context part, which
						// is to be readded later
						if (mergeStrings) {
							for (i = 0; i < 6; i++) {
								*dst++ = '*';
							}
						} else {
							i = 0;
						}
						// some iwd2 dialogs use # instead of " for delimiting parameters (11phaen)
						// BUT at the same time, some bg2 mod prefixes use it too (eg. Tashia)
						while (!IsParamDelimiter(src)) {
							if (*src == 0) {
								delete newTrigger;
								return nullptr;
							}

							// sizeof(context+name) = 40
							if (i < 40) {
								*dst++ = (char) tolower(*src);
								i++;
							}
							src++;
						}
						*dst = 0;
						// reading the context part
						if (mergeStrings) {
							str++;
							if (*str != 's') {
								Log(ERROR, "GSUtils", "Invalid mergestrings 2: {}", str);
								delete newTrigger;
								return nullptr;
							}
							SKIP_ARGUMENT();
							if (!stringsCount) {
								dst = newTrigger->string0Parameter.begin();
							} else {
								dst = newTrigger->string1Parameter.begin();
							}

							// this works only if there are no spaces
							if (*src++ != '"' || *src++ != ',' || *src++ != '"') {
								break;
							}
							// reading the context string
							i = 0;
							while (*src != '"' && (*src != '#' || (*(src - 1) != '(' && *(src - 1) != ','))) {
								if (*src == 0) {
									delete newTrigger;
									return nullptr;
								}

								if (i++ < 6) {
									*dst++ = (char) tolower(*src);
								}
								src++;
							}
						}
						src++; // skipping "
						stringsCount++;
					}
					break;
			}
			str++;
			if (*src == ',' || *src == ')') {
				src++;
			}
		}
	return newTrigger;
}

Trigger* GenerateTrigger(std::string string)
{
	StringToLower(string);
	ScriptDebugLog(DebugMode::TRIGGERS, "Compiling: '{}'", string);

	int negate = 0;
	strpos_t start = 0;
	if (string[start] == '!') {
		++start;
		negate = TF_NEGATE;
	}
	strpos_t len = string.find_first_of('(', start) + 1 - start; // including (
	int i = triggersTable->FindString(StringView(string.c_str() + start, len));
	if (i < 0) {
		Log(ERROR, "GameScript", "Invalid scripting trigger: '{}'", string);
		return nullptr;
	}
	const char* src = string.c_str() + start + len;
	const char* str = triggersTable->GetStringIndex(i).c_str() + len;
	Trigger* trigger = GenerateTriggerCore(src, str, i, negate);
	if (!trigger) {
		Log(ERROR, "GameScript", "Malformed scripting trigger: '{}'", string);
		return nullptr;
	}
	if (triggerflags[trigger->triggerID] & TF_HAS_OBJECT && !trigger->objectParameter) trigger->flags |= TF_MISSING_OBJECT;
	return trigger;
}

Action* GenerateAction(std::string actionString)
{
	Action* action = nullptr;

	StringToLower(actionString);
	ScriptDebugLog(DebugMode::ACTIONS, "Compiling: '{}'", actionString);

	auto len = actionString.find_first_of('(') + 1; //including (
	assert(len != std::string::npos);
	const char* src = &actionString[len];
	int i = -1;
	const char* str;
	unsigned short actionID;
	StringView key(actionString.data(), len);
	if (overrideActionsTable) {
		i = overrideActionsTable->FindString(key);
		if (i >= 0) {
			str = overrideActionsTable->GetStringIndex(i).c_str() + len;
			actionID = static_cast<unsigned short>(overrideActionsTable->GetValueIndex(i));
		}
	}
	if (i < 0) {
		i = actionsTable->FindString(key);
		if (i < 0) {
			Log(ERROR, "GameScript", "Invalid scripting action: '{}'", actionString);
			return action;
		}
		str = actionsTable->GetStringIndex(i).c_str() + len;
		actionID = static_cast<unsigned short>(actionsTable->GetValueIndex(i));
	}
	action = GenerateActionCore(src, str, actionID);
	if (!action) {
		Log(ERROR, "GameScript", "Malformed scripting action: '{}'", actionString);
		return nullptr;
	}
	if (actionflags[action->actionID] & AF_HAS_OBJECT && !action->objects[0] && !action->objects[1]) {
		action->flags |= ACF_MISSING_OBJECT;
	}

	return action;
}

Action* GenerateActionDirect(std::string string, const Scriptable* object)
{
	Action* action = GenerateAction(std::move(string));
	if (!action) return nullptr;
	Object* tmp = action->objects[1];
	if (tmp && tmp->objectFields[0] == -1) {
		tmp->objectFields[1] = object->GetGlobalID();
	}
	action->pointParameter.Invalidate();
	return action;
}

}
