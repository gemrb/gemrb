/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2006 The GemRB Project
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
 *
 *
 */

#include "PROImporter.h"

#include "win32def.h"

#include "EffectMgr.h"
#include "Interface.h"

using namespace GemRB;

PROImporter::PROImporter(void)
{
	str = NULL;
}

PROImporter::~PROImporter(void)
{
	delete str;
	str = NULL;
}

bool PROImporter::Open(DataStream* stream)
{
	if (stream == NULL) {
		return false;
	}
	delete str;
	str = stream;
	char Signature[8];
	str->Read( Signature, 8 );
	if (strncmp( Signature, "PRO V1.0", 8 ) == 0) {
		version = 10;
	} else {
		print("[PROImporter]: This file is not a valid PRO File");
		return false;
	}

	return true;
}

Projectile* PROImporter::GetProjectile(Projectile *s)
{
	if( !s) {
		return NULL;
	}
	ieWord AreaExtension;

	str->ReadWord( &AreaExtension );
	str->ReadWord( &s->Speed );
	str->ReadDword( &s->SFlags ); //spark, ignore center, looping sound etc
	str->ReadResRef( s->SoundRes1 );
	str->ReadResRef( s->SoundRes2 );
	str->ReadResRef( s->TravelVVC ); //no original game data uses this feature
	str->ReadDword( &s->SparkColor );//enabled by PSF_SPARK
	str->ReadDword( &s->ExtFlags ) ; //gemrb extension flags
	str->ReadDword( &s->StrRef );    //gemrb extension strref
	str->ReadDword( &s->RGB );       //gemrb extension rgb pulse
	str->ReadWord( &s->ColorSpeed ); //gemrb extension rgb speed
	str->ReadWord( &s->Shake );      //gemrb extension screen shake
	str->ReadWord( &s->IDSValue);    //gemrb extension IDS targeting
	str->ReadWord( &s->IDSType);     //gemrb extension IDS targeting
	str->ReadWord( &s->IDSValue2);   //gemrb extension IDS targeting
	str->ReadWord( &s->IDSType2);    //gemrb extension IDS targeting
	str->ReadResRef( s->FailSpell);  //gemrb extension fail effect
	str->ReadResRef( s->SuccSpell);  //gemrb extension implicit effect
	str->Seek(172, GEM_CURRENT_POS); //skipping unused (unknown) bytes
	//we should stand at offset 0x100 now
	str->ReadDword( &s->TFlags ); //other projectile flags
	str->ReadResRef( s->BAMRes1 );
	str->ReadResRef( s->BAMRes2 );
	str->Read( &s->Seq1,1 );
	str->Read( &s->Seq2,1 );
	str->ReadWord( &s->LightZ );
	str->ReadWord( &s->LightX );
	str->ReadWord( &s->LightY );
	str->ReadResRef( s->PaletteRes );
	str->Read( s->Gradients, 7);
	str->Read( &s->SmokeSpeed, 1);
	str->Read( s->SmokeGrad, 7);
	str->Read( &s->Aim, 1);
	str->ReadWord( &s->SmokeAnimID);
	str->ReadResRef( s->TrailBAM[0] );
	str->ReadResRef( s->TrailBAM[1] );
	str->ReadResRef( s->TrailBAM[2] );
	str->ReadWord( &s->TrailSpeed[0] );
	str->ReadWord( &s->TrailSpeed[1] );
	str->ReadWord( &s->TrailSpeed[2] );
	str->Seek(172, GEM_CURRENT_POS);
	if (AreaExtension!=3) {
		return s;
	}
	s->InitExtension();
	GetAreaExtension(s->Extension);
	return s;
}

void PROImporter::GetAreaExtension(ProjectileExtension *e)
{
	ieWord tmp;

	str->ReadDword( &e->AFlags );
	str->ReadWord( &e->TriggerRadius );
	str->ReadWord( &e->ExplosionRadius );
	str->ReadResRef( e->SoundRes ); //explosion sound
	str->ReadWord( &e->Delay );
	str->ReadWord( &e->FragAnimID );
	//this projectile index shouldn't be adjusted like the others!!!
	str->ReadWord( &e->FragProjIdx );
	str->Read( &e->ExplosionCount,1 );
	//the area puff type (flames, puffs, clouds) fireball.ids
	//gemrb uses areapro.2da for this
	//It overrides Spread, VVCRes, Secondary, SoundRes, AreaSound, APFlags
	str->Read( &e->ExplType,1 );
	str->ReadWord( &e->ExplColor );
	//this index is off by one in the .pro files, consolidating it here
	str->ReadWord( &tmp);
	if (tmp) {
		tmp--;
	}
	e->ExplProjIdx = tmp;

	str->ReadResRef( e->VVCRes );
	str->ReadWord( &tmp);
	//limit the cone width to 359 degrees (for full compatibility)
	if (tmp>359) {
		tmp=359;
	}
	e->ConeWidth = tmp;
	str->ReadWord( &tmp);

	//These are GemRB specific, not used in the original engine
	str->ReadResRef( e->Spread );
	str->ReadResRef( e->Secondary );
	str->ReadResRef( e->AreaSound );
	str->ReadDword( &e->APFlags );
	str->ReadWord( &e->DiceCount );
	str->ReadWord( &e->DiceSize );
	str->ReadWord( &e->TileX );
	str->ReadWord( &e->TileY );

	if (!e->TileX) {
		e->TileX=64;
	}
	if (!e->TileY) {
		e->TileY=64;
	}

	//we skip the rest
	str->Seek(180, GEM_CURRENT_POS);
}

#include "plugindef.h"

GEMRB_PLUGIN(0xCAD2D64, "PRO File Importer")
PLUGIN_CLASS(IE_PRO_CLASS_ID, PROImporter)
END_PLUGIN()
