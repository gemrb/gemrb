#include "../../includes/win32def.h"
#include "ScriptedAnimation.h"
#include "AnimationMgr.h"
#include "Interface.h"

void ScriptedAnimation::PrepareBAM(DataStream* /*stream*/, Point &p)
{
  /*
  AnimationFactory* af = ( AnimationFactory* )
	  core->GetResourceMgr()->GetFactoryResource( Anim1ResRef, IE_BAM_CLASS_ID );
	anims[0] = af->GetCycle( 0 );
  */
	XPos += p.x;
	YPos += p.y;
	justCreated = true;
}

ScriptedAnimation::ScriptedAnimation(DataStream* stream, Point &p, bool autoFree)
{
	anims[0] = NULL;
	anims[1] = NULL;
	Transparency = 0;
	SequenceFlags = 0;
	XPos = YPos = ZPos = 0;
	FrameRate = 15;
	FaceTarget = 0;
	Sounds[0][0] = 0;
	Sounds[1][0] = 0;
	if (!stream) {
		return;
	}
	char Signature[8];
	stream->Read( Signature, 8 );
	if (strncmp( Signature, "BAM", 3 ) == 0) {
		PrepareBAM(stream, p);
		return;
	}

	if (strncmp( Signature, "VVC V1.0", 8 ) != 0) {
		printf( "Not a valid VVC File\n" );
		if (autoFree)
			delete( stream );
		return;
	}
	ieResRef Anim1ResRef, Anim2ResRef;
	ieDword seq1, seq2;
	stream->ReadResRef( Anim1ResRef );
	stream->ReadResRef( Anim2ResRef );
	stream->ReadDword( &Transparency );
	stream->Seek( 4, GEM_CURRENT_POS );
	stream->ReadDword( &SequenceFlags );
	stream->Seek( 4, GEM_CURRENT_POS );
	stream->ReadDword( &XPos );
	stream->ReadDword( &YPos );
	stream->Seek( 4, GEM_CURRENT_POS );
	stream->ReadDword( &FrameRate );
	stream->ReadDword( &FaceTarget );
	stream->Seek( 16, GEM_CURRENT_POS );
	stream->ReadDword( &ZPos );
	stream->Seek( 24, GEM_CURRENT_POS );
	stream->ReadDword( &seq1 );
	stream->ReadDword( &seq2 );
	stream->Seek( 8, GEM_CURRENT_POS );
	stream->ReadResRef( Sounds[0] );
	stream->ReadResRef( Sounds[1] );
	AnimationFactory* af = ( AnimationFactory* )
		core->GetResourceMgr()->GetFactoryResource( Anim1ResRef, IE_BAM_CLASS_ID );
	anims[0] = af->GetCycle( ( unsigned char ) seq1 );
	anims[1] = af->GetCycle( ( unsigned char ) seq2 );
	XPos += p.x;
	YPos += p.y;
	if (anims[1]) {
		autoSwitchOnEnd = true;
	} else {
		autoSwitchOnEnd = false;
	}
	if(anims[0]) {
		anims[0]->pos = 0;
	}
	justCreated = true;
	if (autoFree) {
		delete( stream );
	}
}

ScriptedAnimation::~ScriptedAnimation(void)
{
	if (anims[0]) {
		delete( anims[0] );
	}
	if (anims[1]) {
		delete( anims[1] );
	}
}
