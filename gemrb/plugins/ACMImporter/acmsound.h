/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/ACMImporter/Attic/acmsound.h,v 1.3 2003/11/25 13:48:04 balrog994 Exp $
 *
 */

#if !defined(AFX_ACMSOUND_H__C975A46B_7D3C_433C_8F35_9582A94DEF15__INCLUDED_)
#define AFX_ACMSOUND_H__C975A46B_7D3C_433C_8F35_9582A94DEF15__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

int ConvertAcmWav(int fhandle, long maxlen, unsigned char *&memory, long &samples_written, int forcestereo);
int ConvertWavAcm(int fh, long maxlen, FILE *foutp, bool wavc_or_acm);

#endif // !defined(AFX_ACMSOUND_H__C975A46B_7D3C_433C_8F35_9582A94DEF15__INCLUDED_)
