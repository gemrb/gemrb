/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2020 The GemRB Project
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
 *
 */

#include "CodepageToIconv.h"

/**
 * This list is taken from win-iconv, available under public domain.
 *
 * It is sorted and stripped for uniqueness and regular Iconv support.
 */
namespace GemRB {

const CodepageIconvMap codepageIconvMap = {{
	{437, "IBM437"}, /* OEM United States */
	{737, "ibm737"}, /* OEM Greek (formerly 437G); Greek (DOS) */
	{775, "ibm775"}, /* OEM Baltic; Baltic (DOS) */
	{850, "ibm850"}, /* OEM Multilingual Latin 1; Western European (DOS) */
	{852, "ibm852"}, /* OEM Latin 2; Central European (DOS) */
	{855, "IBM855"}, /* OEM Cyrillic (primarily Russian) */
	{857, "ibm857"}, /* OEM Turkish; Turkish (DOS) */
	{858, "IBM00858"}, /* OEM Multilingual Latin 1 + Euro symbol */
	{860, "IBM860"}, /* OEM Portuguese; Portuguese (DOS) */
	{861, "ibm861"}, /* OEM Icelandic; Icelandic (DOS) */
	{862, "DOS-862"}, /* OEM Hebrew; Hebrew (DOS) */
	{863, "IBM863"}, /* OEM French Canadian; French Canadian (DOS) */
	{864, "IBM864"}, /* OEM Arabic; Arabic (864) */
	{865, "IBM865"}, /* OEM Nordic; Nordic (DOS) */
	{866, "cp866"}, /* OEM Russian; Cyrillic (DOS) */
	{869, "ibm869"}, /* OEM Modern Greek; Greek, Modern (DOS) */
	{874, "windows-874"}, /* ANSI/OEM Thai (same as 28605, ISO 8859-15); Thai (Windows) */
	{932, "shift_jis"}, /* ANSI/OEM Japanese; Japanese (Shift-JIS) */
	{936, "gb2312"}, /* ANSI/OEM Simplified Chinese (PRC, Singapore); Chinese Simplified (GB2312) */
	{949, "ks_c_5601-1987"}, /* ANSI/OEM Korean (Unified Hangul Code) */
	{950, "big5"}, /* ANSI/OEM Traditional Chinese (Taiwan; Hong Kong SAR, PRC); Chinese Traditional (Big5) */
	{1250, "windows-1250"}, /* ANSI Central European; Central European (Windows) */
	{1251, "windows-1251"}, /* ANSI Cyrillic; Cyrillic (Windows) */
	{1252, "windows-1252"}, /* ANSI Latin 1; Western European (Windows) */
	{1253, "windows-1253"}, /* ANSI Greek; Greek (Windows) */
	{1254, "windows-1254"}, /* ANSI Turkish; Turkish (Windows) */
	{1255, "windows-1255"}, /* ANSI Hebrew; Hebrew (Windows) */
	{1256, "windows-1256"}, /* ANSI Arabic; Arabic (Windows) */
	{1257, "windows-1257"}, /* ANSI Baltic; Baltic (Windows) */
	{1258, "windows-1258"}, /* ANSI/OEM Vietnamese; Vietnamese (Windows) */
	{10000, "macintosh"}, /* MAC Roman; Western European (Mac) */
	{20127, "us-ascii"}, /* US-ASCII (7-bit) */
	{20866, "koi8-r"}, /* Russian (KOI8-R); Cyrillic (KOI8-R) */
	{20932, "EUC-JP"}, /* Japanese (JIS 0208-1990 and 0121-1990) */
	{21025, "cp1025"}, /* IBM EBCDIC Cyrillic Serbian-Bulgarian */
	{21866, "koi8-u"}, /* Ukrainian (KOI8-U); Cyrillic (KOI8-U) */
	{28591, "iso-8859-1"}, /* ISO 8859-1 Latin 1; Western European (ISO) */
	{28592, "iso-8859-2"}, /* ISO 8859-2 Central European; Central European (ISO) */
	{28593, "iso-8859-3"}, /* ISO 8859-3 Latin 3 */
	{28594, "iso-8859-4"}, /* ISO 8859-4 Baltic */
	{28595, "iso-8859-5"}, /* ISO 8859-5 Cyrillic */
	{28596, "iso-8859-6"}, /* ISO 8859-6 Arabic */
	{28597, "iso-8859-7"}, /* ISO 8859-7 Greek */
	{28598, "iso-8859-8"}, /* ISO 8859-8 Hebrew; Hebrew (ISO-Visual) */
	{28599, "iso-8859-9"}, /* ISO 8859-9 Turkish */
	{28603, "iso-8859-13"}, /* ISO 8859-13 Estonian */
	{28605, "iso-8859-15"}, /* ISO 8859-15 Latin 9 */
	{38598, "iso-8859-8-i"}, /* ISO 8859-8 Hebrew; Hebrew (ISO-Logical) */
	{50220, "iso-2022-jp"}, /* ISO 2022 Japanese with no halfwidth Katakana; Japanese (JIS) */
	{50221, "csISO2022JP"}, /* ISO 2022 Japanese with halfwidth Katakana; Japanese (JIS-Allow 1 byte Kana) */
	{50222, "iso-2022-jp"}, /* ISO 2022 Japanese JIS X 0201-1989; Japanese (JIS-Allow 1 byte Kana - SO/SI) */
	{50225, "iso-2022-kr"}, /* ISO 2022 Korean */
	{51932, "euc-jp"}, /* EUC Japanese */
	{51936, "EUC-CN"}, /* EUC Simplified Chinese; Chinese Simplified (EUC) */
	{51949, "euc-kr"}, /* EUC Korean */
	{54936, "GB18030"}, /* Windows XP and later: GB18030 Simplified Chinese (4 byte); Chinese Simplified (GB18030) */
}};

}
