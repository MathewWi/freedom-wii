/*

Freedom - A system menu replacement for Nintendo Wii

Copyright (C) 2008-2009  crediar

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation version 2.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/

#include "tremor\ivorbiscodec.h"
#include "tremor\ivorbisfile.h"
#include "asndlib.h"


enum LoadType { LOAD_SOUND_FILE, LOAD_SOUND_BUFFER };
enum Tags { TITLE, ARTIST, ALBUM, TRACKNUMBER, GERNE };

void SoundInit( void );
void SoundDeInit( void );
void SoundPause( void );
void SoundResume( void );
void SoundStop( void );
void SoundPauseResume( void );

s32 SoundPlayBuffer( void *Buffer, u32 size );
void *SoundOggPlayBuf( void *Buffer );

s32 SoundPlayFile( FILE *f );
void *SoundOggPlay( void *f );
s32 SoundOggReadSample( u8 *buffer, u32 size );

char *SoundFileGetTagInfo( FILE *f, u32 Tag );
char *SoundGetOggTag(  FILE *f, u32 Tag );
