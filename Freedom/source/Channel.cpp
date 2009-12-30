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
#include <stdio.h>
#include <stdlib.h>
#include <gccore.h>
#include <wiiuse/wpad.h>
#include <fat.h>
#include <sys/dir.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <malloc.h>
#include <vector>
#include <stdarg.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/errno.h>
#include <ogc/lwp_watchdog.h>

#include <sys/types.h>
#include <sys/errno.h>
#include <fcntl.h>

#include "Channel.h"
#include "dol.h"
#include "processor.h"
#include "Identify.h"
#include "WAD.h"
#include "Config.h"


extern "C"
{
	#include "Patches.h"
	#include "misc.h"
	#include "lz77.h"
	#include "Loader_bin.h"
	#include "kenobiwii_bin.h"
	extern void _unstub_start(void);
	extern void patchhook(u32 address, u32 len);
}


std::vector<char*> *ChannelNames=NULL;

ChannelType CT[TYPECOUNT] =
{
	{"Commodore 64", 'C'},
	{"NeoGeo", 'E'},
	{"MSX", 'X'},
	{"Nintendo 64", 'N'},
	{"NES", 'F'},
	{"Sega Master System", 'L'},
	{"Sega Megadrive", 'M'},
	{"SNES", 'J'},
	{"TurboGraFX", 'P'},
	{"TurboGraFX CD", 'Q'},
	{"Virtual Console Arcade", 'D'},
	{"WiiWare", 'W'},
	{"System", 'H'},
	{"Unknown", '_'}
};
s32	Channel::GetStatus ( void )
{
	return Status;
}
s32	Channel::Update ( void )
{
	return update;
}

	Channel::Channel( void )
{
	Status=0;
	update=0;

	s32 r = ES_GetNumTitles( &TitleCount );
	if( r < 0 )
	{
		gprintf("ES_GetNumTitles():%d", r );
		Status=-4;
		update=1;
		return;
	}

	u64 *TitlesIDs = (u64*)memalign( 32, sizeof(u64)*TitleCount );
	memset( TitlesIDs, 0, sizeof(u64)*TitleCount );

	r = ES_GetTitles( TitlesIDs, TitleCount );
	if( r < 0 )
	{
		gprintf("ES_GetTitles():%d", r );
		Status=-4;
		update=1;
		return;
	}

	std::vector<u64> t;

	//Pre sort list
	for(u32 i=0; i < TitleCount; ++i)
	{	
		//Skip system, game and hidden titles
		if( ( (TitlesIDs[i]>>32) == 0x00000001 ) ||  ( (TitlesIDs[i]>>32) == 0x00010000 ) ||  ( (TitlesIDs[i]>>32) == 0x00010008 ) )
			continue;

		//check if the title has a ticket
		u32 cnt ATTRIBUTE_ALIGN(32)=0;
		s32 r=ES_GetNumTicketViews(TitlesIDs[i], &cnt);
		if( cnt == 0 || r < 0 )
			continue;

		t.resize( t.size() + 1);
		t[t.size()-1] = TitlesIDs[i];
	}

	free(TitlesIDs);

	TitleCount = t.size();

	gprintf("Channel:Installed Channels:%d\n", TitleCount );

	ChannelSettings.resize( TitleCount );

	//Load settings from SD
	//r = LoadConfig();
	//gprintf("Channel:LoadConfig():%d\n", r );

	for( u32 i=0; i < TitleCount; ++i )
	{
		//We have to assign the ID first, otherwise iGetName will fail
		ChannelSettings[i].TitleID = t[i];

		char *s=iGetName(i);
		if( s == NULL )
			strncpy( ChannelSettings[i].Name, "fail", 32 );
		else
			strncpy( ChannelSettings[i].Name, s, 32 );
				

		switch( (u8)(ChannelSettings[i].TitleID&0xFF) )
		{
			case 'J':
			case 'E':
				ChannelSettings[i].Region = 0;
				break;
			case 'P':
			case 'D':
				ChannelSettings[i].Region = 1;
				break;
			case 'X':	//HBC
			case 'A':	//Region Free
				ChannelSettings[i].Region = 0;	//TODO: Get real value from system
				break;
			default:
				gprintf("Channel:Channel:Warning unknown Region:%x(%016llx)\n", (u8)(ChannelSettings[i].TitleID&0xFF), ChannelSettings[i].TitleID );
				ChannelSettings[i].Region = 0;
				break;
		}
	}

	t.resize(0);

	Status=1;
	update=1;

	//gprintf("done!\n");

	return;
}
u32 Channel::LoadConfig ( void )
{
	FILE *f = fopen("sd:/freedom/channel.cfg", "rb" );
	if( f == NULL )
		return 0;

	//Get file size and allocate space

	fseek( f, 0, SEEK_END );
	u32 size = ftell( f );
	fseek( f, 0, SEEK_SET );

	ChannelSettingsSD.resize( size/sizeof( ChannelSetting ) );

	s32 r = 0;

	ChannelSetting set;

	for( u32 i=0; i < ChannelSettingsSD.size(); ++i )
	{
		memset( &set, 0, sizeof( ChannelSetting ) );
		
		r += fread( &set, sizeof( ChannelSetting ), 1, f );

		ChannelSettingsSD[i].TitleID = set.TitleID;
		ChannelSettingsSD[i].Region = set.Region;
		strncpy( ChannelSettingsSD[i].Name, set.Name, 32 );
	}

	gprintf("Channel:LoadConfig->Read %d bytes\n", r*sizeof( ChannelSetting ) );
	fclose( f );

	return 1;
}
u32 Channel::SaveConfig( void )
{
	FILE *f = fopen("sd:/freedom/channel.cfg", "wb" );
	if( f == NULL )
		return 0;
	
	s32 r = 0;

	ChannelSetting set;

	for( u32 i=0; i < ChannelSettings.size(); ++i )
	{

		memset( &set, 0, sizeof( ChannelSetting ) );

		set.TitleID = ChannelSettings[i].TitleID;
		set.Region = ChannelSettings[i].Region;
		strncpy( set.Name, ChannelSettings[i].Name, 32 );
		
		r += fwrite( &set, sizeof( ChannelSetting ), 1, f );
	}

	gprintf("Channel:SaveConfig->Wrote %d bytes\n", r*sizeof( ChannelSetting ) );
	fclose( f );

	return 1;

}
u32 Channel::GetRegion( u32 ID )
{
	if( ID > TitleCount )
	{
		gprintf("Channel:GetLocation(%d):Error ID(%d) > TitleCount(%d)\n", ID, ID, TitleCount ); 
		return NULL;
	}
	return ChannelSettings[ID].Region;
}
u32 Channel::SetRegion( u32 ID, u32 Region )
{
	if( ID > TitleCount )
	{
		gprintf("Channel:SetLocation(%d):Error ID(%d) > TitleCount(%d)\n", ID, ID, TitleCount ); 
		return 0;
	}
	if( Region > 1 )
	{
		gprintf("Channel:SetLocation(%d):Error Region(%d) > (%d)\n", Region, Region, 1 ); 
		return 0;
	}

	ChannelSettings[ID].Region = Region;

	return 1;
}

char *Channel::GetName( u32 ID )
{
	if( ID > TitleCount )
	{
		gprintf("Channel:GetName(%d):Error ID(%d) > TitleCount(%d)\n", ID, TitleCount ); 
		return NULL;
	}
	return ChannelSettings[ID].Name;
}
/*
	Internal Function
	Gets the Title name from the content file
*/
char *Channel::iGetName( u32 ID )
{
	if( ID > TitleCount )
	{
#ifdef CHANNEL_DEBUG
		gprintf("blah\n");
#endif
		return NULL;
	}

	static u32 tmd_size ATTRIBUTE_ALIGN(32);
	s32 r=ES_GetStoredTMDSize( ChannelSettings[ID].TitleID, &tmd_size);
#ifdef CHANNEL_DEBUG
	gprintf("Channel:iGetName->ES_GetStoredTMDSize():%d\n", r);
#endif
	if(r<0)
	{
		update=1;
		Status=r;
		return NULL;
	}

	signed_blob *TMD = (signed_blob *)memalign( 32, (tmd_size+31)&(~31) );
	if( TMD == NULL )
	{
		update=1;
		Status=-20;
		return NULL;
	}
	memset(TMD, 0, tmd_size);

	r=ES_GetStoredTMD( ChannelSettings[ID].TitleID, TMD, tmd_size);
#ifdef CHANNEL_DEBUG
	gprintf("Channel:iGetName->ES_GetStoredTMD():%d\n", r);
#endif
	if(r<0)
	{
		update=1;
		Status=r;
		free( TMD );
		return NULL;
	}
	
	tmd *rTMD = (tmd *)(TMD+(0x140/sizeof(tmd *)));

	u8 *File = (u8*)memalign( 32, 256 );
	memset( File, 0, 256);

	u32 a = (u32)(ChannelSettings[ID].TitleID>>32);
	u32 b = (u32)(ChannelSettings[ID].TitleID&0xFFFFFFFF);
	u32 c = rTMD->contents[0].cid;

	sprintf( (char*)File, "/title/%08x/%08x/content/%08x.app", a, b, c );
	//sprintf( (char*)File, "/title/00010001/4a414550/content/00000009.app");
	
	s32 fd = ISFS_Open( (const char*)File, 1 );
#ifdef CHANNEL_DEBUG
	gprintf("Channel:iGetName->ISFS_Open():%d\n", fd);
#endif
	if( fd < 0 )
	{
		update=1;
		Status=-c<<8;
		free(File);
		free( TMD );
		char *s = (char*)memalign(32,32);
		sprintf( s, "%c%c%c%c", b>>24, (b>>16)&0xFF, (b>>8)&0xFF, b&0xFF);
		return s;
	}

	u8 *buf = (u8 *)memalign(32, 512 );
	memset( buf, 0, 512 );
	
	ISFS_Seek( fd, 0, SEEK_SET );
	if( ISFS_Read( fd, buf, 0x200 ) != 0x200 )
	{
#ifdef CHANNEL_DEBUG
	gprintf("Channel:iGetName->ISFS_Read():fail\n");
#endif
		update=1;
		Status=-8;
		free(buf);
		ISFS_Close( fd );
		free(File);
		free( TMD );
		char *s = (char*)memalign(32,32);
		sprintf( s, "%c%c%c%c", b>>24, (b>>16)&0xFF, (b>>8)&0xFF, b&0xFF);
		return s;
	}
	
	char *TitleName = (char*)memalign(32, 0x28);

	int y;
	for(y=0;y<0x28;y++)
		TitleName[y]=buf[0xF0+(y*2)+1];

	ISFS_Close( fd );

	free(buf);
	free(File);
	free( TMD );
	Status=1;
	update=1;

	return TitleName;

}
u64	Channel::GetTitleID( u32 ID )
{
	if( ID > TitleCount )
	{
		gprintf("Channel:GetTitleID(%d):Error ID(%d) > TitleCount(%d)\n", ID, TitleCount ); 
		return 0;
	}
	return ChannelSettings[ID].TitleID;
}
s32 Channel::LaunchByIOS( u32 ID )
{
	u32 cnt ATTRIBUTE_ALIGN(32);
	s32 r=ES_GetNumTicketViews(ChannelSettings[ID].TitleID, &cnt);
	gprintf("Channel:LaunchByIOS->ES_GetNumTicketViews(%x-%x):%d\n", (u32)(ChannelSettings[ID].TitleID>>32), (u32)(ChannelSettings[ID].TitleID&0xFFFFFFFF), r );
	if( r < 0 )
	{
		update=1;
		Status=-1;
		return r;
	}
	tikview *views = (tikview *)memalign( 32, sizeof(tikview)*cnt );

	r=ES_GetTicketViews(ChannelSettings[ID].TitleID, views, cnt);
	gprintf("Channel:LaunchByIOS->ES_GetTicketViews(%x-%x):%d\n", (u32)(ChannelSettings[ID].TitleID>>32), (u32)(ChannelSettings[ID].TitleID&0xFFFFFFFF), r );
	if( r < 0 )
	{
		update=1;
		Status=-2;
		return r;
	}

	WPAD_Shutdown();

	fatUnmount("SD");

	gprintf("Channel:LaunchByIOS->ES_LaunchTitle(%x-%x) ...", (u32)(ChannelSettings[ID].TitleID>>32), (u32)(ChannelSettings[ID].TitleID&0xFFFFFFFF) );
	r = ES_LaunchTitle(ChannelSettings[ID].TitleID, &views[0]);

	// Title installed but no content! replace with some read from file stuff
	if( r == -106 )
	{
		gprintf("\nChannel:LaunchByIOS->No content installed!\n");
	}

	gprintf("Failed: %d\n", r );

	update=1;
	Status=-3;
	return r;
}
s32 Channel::Launch( u32 ID )
{
	//Haxx for certain channels!
	switch( ChannelSettings[ID].TitleID )
	{
		case 0x1000248414241LL:
		case 0x1000148415858LL:
			LaunchByIOS( ID );
			break;
	}

	//First get TMD and boot index
	u32 tmd_size=0;
	s32 r=ES_GetStoredTMDSize( ChannelSettings[ID].TitleID, &tmd_size);
	gprintf("Channel:Launch->ES_GetStoredTMDSize(%llx):%d\n", ChannelSettings[ID].TitleID, r );
	if( r < 0 )
	{
		return -1;
	}

	signed_blob *TMD = (signed_blob *)memalign( 32, (tmd_size+31)&(~31) );
	if( TMD == NULL )
	{
		return -2;
	}
	memset(TMD, 0, tmd_size);

	r=ES_GetStoredTMD( ChannelSettings[ID].TitleID, TMD, tmd_size);
	gprintf("Channel:Launch->ES_GetStoredTMD():%d\n", r );
	if(r<0)
	{
		free( TMD );
		return -3;
	}

	tmd *rTMD = (tmd *)(TMD+(0x140/sizeof(tmd *)));

	//get boot filename
	u32 fileID = 0;
	for(u32 z=0; z < rTMD->num_contents; ++z)
	{
		if( rTMD->contents[z].index == 1 )
		{
			fileID = rTMD->contents[z].cid;
			break;
		}
	}

	gprintf("Channel:Launch->fileID:%d\n", fileID );

	if( fileID == 0 )
	{
		free( TMD );
		return -5;
	}

	char * file = (char*)memalign( 32, 256 );
	if( file == NULL )
	{
		free( TMD );
		return -6;
	}

	memset(file, 0, 256 );

	u32 a = (u32)(ChannelSettings[ID].TitleID>>32);
	u32 b = (u32)(ChannelSettings[ID].TitleID&0xFFFFFFFF);

	sprintf( file, "/title/%08x/%08x/content/%08x.app", a, b, fileID );	
	gprintf("Channel:Launch->File:\"%s\"\n", file );

	s32 fd = ISFS_Open( file, 1 );
	gprintf("Channel:Launch->ISFS_Open():%d\n", fd );
	if( fd < 0 )
	{
		free( TMD );
		free( file );
		return -7;
	}

	void	(*entrypoint)();

	ISFS_Seek( fd, 0, 0 );
	u32 size = ISFS_Seek( fd, 0, SEEK_END );

	gprintf("Channel:Launch->DOL size:%d\n", size );

	u8 *DOLComp = (u8 *)memalign(32, (size+31)&(~31) );
	if( DOLComp == NULL )
	{
		gprintf("Channel:Launch->memalign failed!\n" );
		free( TMD );
		free( file );
		return -8;
	}

	ISFS_Seek( fd, 0, 0);

	r = ISFS_Read( fd, DOLComp, size );

	if( r < 0 || r != size )
	{
		gprintf("Channel:Launch->ISFS_Read() failed:%d\n", r );
		free( TMD );
		free( file );
		return -8;
	}

	dolhdr *hdr = (dolhdr*)DOLComp;
	u8 *DOLDecomp;

	if( isLZ77compressed( (u8*)DOLComp )  )	//Compressed!
	{
		gprintf("Channel:Launch->DOL is compressed!\n");

		u32 Type = (*(vu8*)DOLComp);
		u32 SizeD = __GetDecompressSize( DOLComp, Type);

		gprintf("Channel:Launch->DOL decompressed size:%d Type:%02X\n", SizeD, Type );

		
		switch( Type )
		{
			case 0x10:
				__decompressLZ77_10( DOLComp, size, &DOLDecomp );
			break;
			case 0x11:
				__decompressLZ77_11( DOLComp, size, &DOLDecomp );
			break;
		}

		free( DOLComp );

	} else {
		memcpy( (void*)0x92000000, DOLComp, size );
	}

	hdr = (dolhdr*)(0x92000000);

	u32 ChannelHookType = 2;
	u32 Patched=0;
	u32 HookOffset=0;
	u32 RealOffset=0;
	//apply patches

	memset( (void*)0x80001800, 0, kenobiwii_bin_size );
	memcpy( (void*)0x80001800, kenobiwii_bin, kenobiwii_bin_size ); 

	*(vu32*)(*(vu32*)0x80001808) = 0;
	DCFlushRange( (void*)0x80001800, kenobiwii_bin_size );

	ApplyPatch( (void*)(0x92000000), size, PATTERN_FWRITE, PATCH_FWRITE );

	//We need to know the sections offsets to correctly calculate the jump offset

#ifdef CHANNEL_DEBUG
	gprintf("Text sections\n");
#endif
	for( u32 i=0; i < 6; ++i )
	{
		if( hdr->sizeText[i] == 0 )
			continue;

#ifdef CHANNEL_DEBUG
		gprintf("%d:\t%08X\t\t%08X\t\t%08X\n", i, hdr->addressText[i], hdr->offsetText[i], hdr->sizeText[i] );
#endif
		if( Patched == 0 )
		{
			HookOffset = FindPattern( (void*)(0x92000000+hdr->offsetText[i]), hdr->sizeText[i], PATTERN_VIWII );
			if( HookOffset )
			{
				RealOffset = 0x92000000+HookOffset;
				HookOffset = hdr->addressText[i]+HookOffset-hdr->offsetText[i] ;
				gprintf("Channel:Launch->Found VIWaitForRetrace at %x\n", HookOffset );
				Patched = 1;
			} else {
				HookOffset = FindPattern( (void*)(0x92000000+hdr->offsetText[i]), hdr->sizeText[i], PATTERN_KPADREAD );
				if( HookOffset  )
				{
					RealOffset = 0x92000000+HookOffset;
					HookOffset = hdr->addressText[i]+HookOffset-hdr->offsetText[i] ;
					gprintf("Channel:Launch->Found KPAD_Read at %x\n", HookOffset );
					Patched = 1;
				} else {
					HookOffset = FindPattern( (void*)(0x92000000+hdr->offsetText[i]), hdr->sizeText[i], PATTERN_KPADREADOLD );
					if( HookOffset  )
					{
						RealOffset = 0x92000000+HookOffset;
						HookOffset = hdr->addressText[i]+HookOffset-hdr->offsetText[i] ;
						gprintf("Channel:Launch->Found KPAD_Read(old) at %x\n", HookOffset );
						Patched = 1;
					} else {
						HookOffset = FindPattern( (void*)(0x92000000+hdr->offsetText[i]), hdr->sizeText[i], PATTERN_PADREAD );
						if( HookOffset  )
						{
							RealOffset = 0x92000000+HookOffset;
							HookOffset = hdr->addressText[i]+HookOffset-hdr->offsetText[i] ;
							gprintf("Channel:Launch->Found PAD_Read at %x\n", HookOffset );
							Patched = 1;
						}
					}
				}
			}
		}

		//	if( Patched == 0 )
		//	{
		//		if( memcmp( (void*)(0x92000000+j), viwiihooks, sizeof(viwiihooks))==0)
		//		{
		//			RealOffset = 0x92000000+j;
		//			HookOffset = hdr->addressText[i]+j-hdr->offsetText[i] ;
		//			gprintf("Channel:Launch->Found viwiihooks at %x\n", HookOffset );
		//			Patched = 1;
		//		}
		//		if( memcmp( (void*)(0x92000000+j), kpadhooks, sizeof(kpadhooks))==0)
		//		{
		//			RealOffset = 0x92000000+j;
		//			HookOffset = hdr->addressText[i]+j-hdr->offsetText[i] ;
		//			gprintf("Channel:Launch->Found kpadhooks at %x\n", HookOffset );
		//			Patched = 1;
		//		}
		//		if( memcmp( (void*)(0x92000000+j), kpadoldhooks, sizeof(kpadoldhooks))==0)
		//		{
		//			RealOffset = 0x92000000+j;
		//			HookOffset = hdr->addressText[i]+j-hdr->offsetText[i] ;
		//			gprintf("Channel:Launch->Found kpadoldhooks at %x\n", HookOffset );
		//			Patched = 1;
		//		}
		//		if( memcmp( (void*)(0x92000000+j), joypadhooks, sizeof(joypadhooks))==0)
		//		{
		//			RealOffset = 0x92000000+j;
		//			HookOffset = hdr->addressText[i]+j-hdr->offsetText[i] ;
		//			gprintf("Channel:Launch->Found joypadhooks at %x\n", HookOffset );
		//			Patched = 1;
		//		}
		//	}
		//}
	}

#ifdef CHANNEL_DEBUG
	gprintf("\nData sections\n");
#endif
	for( u32 i=0; i <= 10; ++i )
	{
		if( hdr->sizeData[i] == 0 )
			continue;

#ifdef CHANNEL_DEBUG
		gprintf("%d:\t%08X\t\t%08X\t\t%08X\n", i, hdr->addressData[i], hdr->offsetData[i], hdr->sizeData[i] );
#endif
		if( Patched == 0 )
		{
			HookOffset = FindPattern( (void*)(0x92000000+hdr->offsetData[i]), hdr->sizeData[i], PATTERN_VIWII );
			if( HookOffset )
			{
				RealOffset = 0x92000000+HookOffset;
				HookOffset = hdr->addressText[i]+HookOffset-hdr->offsetText[i];
				gprintf("Channel:Launch->Found VIWaitForRetrace at %x\n", HookOffset );
				Patched = 1;
			} else {
				HookOffset = FindPattern( (void*)(0x92000000+hdr->offsetData[i]), hdr->sizeData[i], PATTERN_KPADREAD );
				if( HookOffset  )
				{
					RealOffset = 0x92000000+HookOffset;
					HookOffset = hdr->addressText[i]+HookOffset-hdr->offsetText[i] ;
					gprintf("Channel:Launch->Found KPAD_Read at %x\n", HookOffset );
					Patched = 1;
				} else {
					HookOffset = FindPattern( (void*)(0x92000000+hdr->offsetData[i]), hdr->sizeData[i], PATTERN_KPADREADOLD );
					if( HookOffset  )
					{
						RealOffset = 0x92000000+HookOffset;
						HookOffset = hdr->addressText[i]+HookOffset-hdr->offsetText[i] ;
						gprintf("Channel:Launch->Found KPAD_Read(old) at %x\n", HookOffset );
						Patched = 1;
					} else {
						HookOffset = FindPattern( (void*)(0x92000000+hdr->offsetData[i]), hdr->sizeData[i], PATTERN_PADREAD );
						if( HookOffset  )
						{
							RealOffset = 0x92000000+HookOffset;
							HookOffset = hdr->addressText[i]+HookOffset-hdr->offsetText[i] ;
							gprintf("Channel:Launch->Found PAD_Read at %x\n", HookOffset );
							Patched = 1;
						}
					}
				}
			}
		}

		//	if( Patched == 0 )
		//	{
		//		if( memcmp( (void*)(0x92000000+j), viwiihooks, sizeof(viwiihooks))==0)
		//		{
		//			RealOffset = 0x92000000+j;
		//			HookOffset = hdr->addressData[i]+j-hdr->offsetData[i] ;
		//			gprintf("Channel:Launch->Found viwiihooks at %x\n", HookOffset );
		//			Patched = 1;
		//		}
		//		if( memcmp( (void*)(0x92000000+j), kpadhooks, sizeof(kpadhooks))==0)
		//		{
		//			RealOffset = 0x92000000+j;
		//			HookOffset = hdr->addressData[i]+j-hdr->offsetData[i] ;
		//			gprintf("Channel:Launch->Found kpadhooks at %x\n", HookOffset );
		//			Patched = 1;
		//		}
		//		if( memcmp( (void*)(0x92000000+j), kpadoldhooks, sizeof(kpadoldhooks))==0)
		//		{
		//			RealOffset = 0x92000000+j;
		//			HookOffset = hdr->addressData[i]+j-hdr->offsetData[i] ;
		//			gprintf("Channel:Launch->Found kpadoldhooks at %x\n", HookOffset );
		//			Patched = 1;
		//		}
		//		if( memcmp( (void*)(0x92000000+j), joypadhooks, sizeof(joypadhooks))==0)
		//		{
		//			RealOffset = 0x92000000+j;
		//			HookOffset = hdr->addressData[i]+j-hdr->offsetData[i] ;
		//			gprintf("Channel:Launch->Found joypadhooks at %x\n", HookOffset );
		//			Patched = 1;
		//		}
		//	}
		//}
	}

	if ( Patched == 1 )
	{
		//apply hook

		//find next blr
		u32 z=0;
		while( *(vu32*)(RealOffset+z) != 0x4E800020 )
			z+=4;

		HookOffset+=z;

		u32 newval = (0x800018A8 - HookOffset);
			newval&= 0x03FFFFFC;
			newval|= 0x48000000;

		*(vu32*)(RealOffset+z) = newval;

		gprintf("Channel:Launch->Wrote %08X to %p(%08x) Dist:%X\n", newval, (vu32*)(RealOffset+z), HookOffset, z );

	}

	entrypoint = (void (*)())(hdr->entrypoint);

	WPAD_Shutdown();

	fatUnmount("SD");

	ISFS_Deinitialize();

	r = IOS_ReloadIOS((u32)(rTMD->sys_version&0xFFFFFFFF));
	gprintf("Channel:Launch->IOS_ReloadIOS(%d):%d\n", (u32)(rTMD->sys_version&0xFFFFFFFF), r );
	if( r != 0 )
	{
		// either not installed or a stub :O, default to IOS36
		r = IOS_ReloadIOS(36);
		gprintf("Channel:Launch->IOS_ReloadIOS(%d):%d\n", 36, r );
	}

	__IOS_InitializeSubsystems();
	__ES_Init();

	ISFS_Initialize();

	ID_Sysmenu();
	ES_SetUID( ChannelSettings[ID].TitleID );

	AUDIO_StopDMA();
	AUDIO_RegisterDMACallback(NULL);

	fatUnmount("SD");

	__IOS_ShutdownSubsystems();

	mtmsr(mfmsr() & ~0x8000);
	mtmsr(mfmsr() | 0x2002);

	ISFS_Deinitialize();
	SYS_ResetSystem(SYS_SHUTDOWN,0,0);

	switch( (u8)(ChannelSettings[ID].TitleID&0xFF) )
	{
		case 'E':
			gprintf("Channel:Launch->VideoMode(USA)\n");
			*(vu32*)0x800000CC = 0x00000000;
		break;
		case 'J':
			gprintf("Channel:Launch->VideoMode(JAP)\n");
			*(vu32*)0x800000CC = 0x00000000;
		break;
		case 'P':
			gprintf("Channel:Launch->VideoMode(EUR)\n");
			*(vu32*)0x800000CC = 0x00000005;
		break;
		default:
			gprintf("Channel:Launch->VideoMode(0x%x)\n", (u8)(ChannelSettings[ID].TitleID&0xFF) );
			*(vu32*)0x800000CC = 0x00000000;
		break;
	}

	*(vu16*)0x8000315E = 0x0108;					// NDEV BOOT PROGRAM Version (>=0x107 expected)
	*(vu32*)0x8000002C = 0x00000023;				// Console type
	*(vu32*)0x800000F8 = 0x0E7BE2C0;				// Bus Clock Speed
	*(vu32*)0x800000FC = 0x2B73A840;				// CPU Clock Speed
	*(vu32*)0xCD00643C = 0x00000000;				// 32Mhz on Bus

	u8 entryholder[4];

	entryholder[0] = hdr->entrypoint >> 24;
	entryholder[1] = hdr->entrypoint >> 16;
	entryholder[2] = hdr->entrypoint >> 8;
	entryholder[3] = hdr->entrypoint;

	memcpy( (void*)0x80001800, (char*)0x80000000, 6);	// For WiiRD
	memcpy( (void*)0x80003180, (char*)0x80000000, 4);	// online check code, seems offline games clear it?
	memcpy( (void*)0x817f0000, entryholder, 4);			// entrypoint

	*(vu32*)0x80003188 = *(vu32*)0x80003140;			// 002 fix

	*(vu32*)0x80000000 = (u8)(ChannelSettings[ID].TitleID&0xFFFFFFFF);

	DCFlushRange((u32 *)(0x80000000), 0x4000);

	gprintf("Channel:Launch->entrypoint(0x%x)\n", entrypoint );

	memcpy( (void*)0x91F00000, Loader_bin, Loader_bin_size );
	DCFlushRange((void*)0x91F00000, Loader_bin_size);

	entrypoint = (void (*)())(0x91F00000);

	gprintf("boing!\n");

	entrypoint();

	return 1;
}
u32 Channel::GetCount( void )
{
	return TitleCount;
}
	Channel::~Channel( )
{
	;
}
