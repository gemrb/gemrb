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

#include "Logging/Logging.h"

using namespace GemRB;

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
	str->Read(Signature, 8);
	if (strncmp(Signature, "PRO V1.0", 8) == 0) {
		version = 10;
	} else {
		Log(ERROR, "PROImporter", "This file is not a valid PRO File");
		return false;
	}

	return true;
}

Projectile* PROImporter::GetProjectile(Projectile* s)
{
	ieWord AreaExtension;

	str->ReadWord(AreaExtension);
	str->ReadWord(s->Speed);
	str->ReadDword(s->SFlags); //spark, ignore center, looping sound etc
	str->ReadResRef(s->FiringSound);
	str->ReadResRef(s->ArrivalSound);
	str->ReadResRef(s->TravelVVC); //no original game data uses this feature
	str->ReadDword(s->SparkColor); //enabled by PSF_SPARK
	// in the original bg2, there was just a 2 byte padding and 212 byte reserve left
	str->ReadDword(s->ExtFlags); //gemrb extension flags
	str->ReadStrRef(s->StrRef); //gemrb extension strref
	ieDword c;
	str->ReadDword(c); //gemrb extension rgb pulse
	s->RGB = Color::FromABGR(c);
	str->ReadWord(s->ColorSpeed); //gemrb extension rgb speed
	str->ReadWord(s->Shake); //gemrb extension screen shake
	str->ReadWord(s->IDSValue); //gemrb extension IDS targeting
	str->ReadWord(s->IDSType); //gemrb extension IDS targeting
	str->ReadWord(s->IDSValue2); //gemrb extension IDS targeting
	str->ReadWord(s->IDSType2); //gemrb extension IDS targeting
	str->ReadResRef(s->failureSpell); //gemrb extension fail effect
	str->ReadResRef(s->successSpell); //gemrb extension implicit effect
	str->Seek(172, GEM_CURRENT_POS); // skipping unused bytes
	//we should stand at offset 0x100 now
	str->ReadDword(s->TFlags); //other projectile flags
	str->ReadResRef(s->BAMRes1);
	str->ReadResRef(s->BAMRes2);
	str->Read(&s->Seq1, 1);
	str->Read(&s->Seq2, 1);
	str->ReadWord(s->LightZ);
	str->ReadWord(s->LightX);
	str->ReadWord(s->LightY);
	str->ReadResRef(s->PaletteRes);
	str->Read(s->Gradients, 7);
	str->Read(&s->SmokeSpeed, 1);
	str->Read(s->SmokeGrad, 7);
	str->Read(&s->Aim, 1);
	str->ReadWord(s->SmokeAnimID);
	str->ReadResRef(s->TrailBAM[0]);
	str->ReadResRef(s->TrailBAM[1]);
	str->ReadResRef(s->TrailBAM[2]);
	str->ReadWord(s->TrailSpeed[0]);
	str->ReadWord(s->TrailSpeed[1]);
	str->ReadWord(s->TrailSpeed[2]);
	// TODO: check if this was used
	// IESDP: 0 - puff at target, 1 - puff at source
	// original bg2: DWORD  m_dwPuffFlags and then just reserved space
	str->Seek(172, GEM_CURRENT_POS);
	if (AreaExtension != 3) {
		return s;
	}
	s->Extension = GetAreaExtension();
	return s;
}

Holder<ProjectileExtension> PROImporter::GetAreaExtension()
{
	Holder<ProjectileExtension> e = MakeHolder<ProjectileExtension>();

	str->ReadDword(e->AFlags);
	str->ReadWord(e->TriggerRadius);
	str->ReadWord(e->ExplosionRadius);
	str->ReadResRef(e->SoundRes); //explosion sound
	str->ReadWord(e->Delay);
	str->ReadWord(e->FragAnimID);
	//this projectile index shouldn't be adjusted like the others!!!
	str->ReadWord(e->FragProjIdx);
	str->Read(&e->ExplosionCount, 1);
	//the area puff type (flames, puffs, clouds) fireball.ids
	//gemrb uses areapro.2da for this
	//It overrides Spread, VVCRes, Secondary, SoundRes, AreaSound, APFlags
	str->Read(&e->ExplType, 1);
	str->ReadWord(e->ExplColor);
	//this index is off by one in the .pro files, consolidating it here
	str->ReadWord(e->ExplProjIdx);
	if (e->ExplProjIdx) {
		--e->ExplProjIdx;
	}

	str->ReadResRef(e->VVCRes);
	str->ReadWord(e->ConeWidth);
	//limit the cone width to 359 degrees (for full compatibility)
	e->ConeWidth = e->ConeWidth > 359 ? 359 : e->ConeWidth;
	str->Seek(2, GEM_CURRENT_POS); // padding

	//These are GemRB specific, not used in the original engine
	// bg2 has 216 bytes of reserved space here instead
	str->ReadResRef(e->Spread);
	str->ReadResRef(e->Secondary);
	str->ReadResRef(e->AreaSound);
	str->ReadDword(e->APFlags);
	str->ReadWord(e->DiceCount);
	str->ReadWord(e->DiceSize);
	str->ReadPoint(e->tileCoord);

	if (!e->tileCoord.x) {
		e->tileCoord.x = 64;
	}
	if (!e->tileCoord.y) {
		e->tileCoord.y = 64;
	}

	//we skip the rest
	str->Seek(180, GEM_CURRENT_POS);
	return e;
}

#include "plugindef.h"

GEMRB_PLUGIN(0xCAD2D64, "PRO File Importer")
PLUGIN_CLASS(IE_PRO_CLASS_ID, PROImporter)
END_PLUGIN()
