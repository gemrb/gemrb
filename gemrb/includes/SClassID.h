/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
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
 *
 *
 */

/**
 * @file SClassID.h
 * Defines ID numbers identifying the various plugins.
 * Needed for loading the plugins on MS Windows.
 * @author The GemRB Project
 */

#ifndef SCLASS_ID_H
#define SCLASS_ID_H

/** Type of plugin ID numbers */
typedef unsigned long SClass_ID;

#define IE_2DA_CLASS_ID			0x000003F4
#define IE_ACM_CLASS_ID			0x00010000
#define IE_ARE_CLASS_ID			0x000003F2
#define IE_BAM_CLASS_ID			0x000003E8
#define IE_BCS_CLASS_ID			0x000003EF
#define IE_BS_CLASS_ID			0x100003EF
#define IE_BIF_CLASS_ID			0x00020000
#define IE_BIO_CLASS_ID			0x000003FE  //also .res
#define IE_BMP_CLASS_ID			0x00000001
#define IE_PNG_CLASS_ID			0x00000003
#define IE_CHR_CLASS_ID			0x000003FA
#define IE_CHU_CLASS_ID			0x000003EA
#define IE_CRE_CLASS_ID			0x000003F1
#define IE_DLG_CLASS_ID			0x000003F3
#define IE_EFF_CLASS_ID			0x000003F8
#define IE_GAM_CLASS_ID			0x000003F5
#define IE_IDS_CLASS_ID			0x000003F0
#define IE_INI_CLASS_ID			0x00000802
#define IE_ITM_CLASS_ID			0x000003ED
#define IE_MOS_CLASS_ID			0x000003EC
#define IE_MUS_CLASS_ID			0x00040000
#define IE_MVE_CLASS_ID			0x00000002
#define IE_BIK_CLASS_ID			0x00FFFFFF
#define IE_OGG_CLASS_ID			0x00000007
#define IE_PLT_CLASS_ID			0x00000006
#define IE_PRO_CLASS_ID			0x000003FD
#define IE_SAV_CLASS_ID			0x00050000
#define IE_SPL_CLASS_ID			0x000003EE
#define IE_SRC_CLASS_ID			0x00000803
#define IE_STO_CLASS_ID			0x000003F6
#define IE_TIS_CLASS_ID			0x000003EB
#define IE_TLK_CLASS_ID			0x00060000
#define IE_TOH_CLASS_ID			0x00070000
#define IE_TOT_CLASS_ID			0x00080000
#define IE_VAR_CLASS_ID			0x00090000
#define IE_VEF_CLASS_ID			0x000003FC
#define IE_VVC_CLASS_ID			0x000003FB
#define IE_WAV_CLASS_ID			0x00000004
#define IE_WED_CLASS_ID			0x000003E9
#define IE_WFX_CLASS_ID			0x00000005
#define IE_WMP_CLASS_ID			0x000003F7
#define IE_SCRIPT_CLASS_ID		0x000D0000
#define IE_GUI_SCRIPT_CLASS_ID		0x000E0000

typedef unsigned long PluginID;
enum {
	PLUGIN_OPCODES_CORE =		0xABCD0001,
	PLUGIN_OPCODES_ICEWIND,
	PLUGIN_OPCODES_TORMENT,
	PLUGIN_RESOURCE_KEY,
	PLUGIN_RESOURCE_DIRECTORY,
	PLUGIN_RESOURCE_CACHEDDIRECTORY,
	PLUGIN_RESOURCE_NULL,
	PLUGIN_IMAGE_WRITER_BMP,
	PLUGIN_COMPRESSION_ZLIB 
};

#endif
