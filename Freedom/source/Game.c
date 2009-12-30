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
#include <sys/dir.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <malloc.h>
#include <stdarg.h>
#include <ctype.h>
#include <unistd.h>
#include <wiiuse/wpad.h>
#include <sys/errno.h>
#include <ogc/lwp_watchdog.h>

#include <sys/types.h>
#include <sys/errno.h>
#include <fcntl.h>

#include "Patches.h"
#include "DI.h"
#include "Game.h"
#include "misc.h"
#include "dol.h"
#include "fat.h"

vs32 CoverGame=-1;
vs32 GameReady=0;
vs32 DiscError=0;
s32 GameUpdate=0;
u32 DiscType=0;
u32 GamePartitionOffset=0xDEADBEEF;
char *GameTitle=NULL;

bool GameStatus ( void )
{
	return GameUpdate;
}
void *LoadGame( void *ptr )
{
	GameReady=0;
	GameUpdate=1;

	if( GameTitle == NULL )
	{
		GameTitle = (char*)memalign(32, 0x40 );
		memset( GameTitle, 0, 0x40);
	}

	s32 r = DVDInit();
	if( r < 0 )
	{
		u32 error;
		DVDLowRequestError( &error );
		gprintf("DiscThread:DI_Init():%d ERROR:%X\n", r, error );
		GameReady=-4;

		GameUpdate=1;
		while(1)sleep(1);
	}

	r = DVDLowClosePartition();
	if( r != 1 )
	{
		u32 error;
		DVDLowRequestError( &error );
		gprintf("DiscThread:DVDLowClosePartition():%d ERROR:%X\n", r, error );
		GameReady=-5;

		GameUpdate=1;
		while(1)sleep(1);
	}

	r = DVDLowStopMotor();
	if( r != 1 )
	{
		u32 error;
		DVDLowRequestError( &error );
		gprintf("DiscThread:DVDLowStopMotor():%d ERROR:%X\n", r, error );
		GameReady=-6;

		GameUpdate=1;
		while(1)sleep(1);
	}

	u32 status;

	do {
		DVDLowGetCoverStatus( &status );

		if( status == 1 )
		{
			GameReady=-2;
			GameUpdate=1;
			sleep(3);
		}

	} while( status == 1 );


	GameReady=0;
	GameUpdate=1;

	r = DVDLowReset();
	if( r != 1 )
	{
		u32 error;
		DVDLowRequestError( &error );
		gprintf("DiscThread:DI_Reset():%d ERROR:%X\n", r, error );
		GameReady=-7;

		GameUpdate=1;
		while(1)sleep(1);
	}

	memset( (void*)0x80000000, 0, 0x20);

	r = DVDLowReadDiskID( (void*)0x80000000 );
	if( r != 1 )
	{
		u32 error;
		DVDLowRequestError( &error );
		gprintf("DiscThread:DI_ReadDiscID():%d ERROR:%X\n", r, error );
		GameReady=-8;

		GameUpdate=1;
		while(1)sleep(1);
	}

	CoverGame=0;

	r = DVDLowUnencryptedRead( (void*)0x80000000, 0x20, 0);
	if( r != 1 )
	{
		u32 error;
		DVDLowRequestError( &error );
		gprintf("DiscThread:DVDLowUnencryptedRead():%d ERROR:%X\n", r, error );
		GameReady=-9;

		GameUpdate=1;
		while(1)sleep(1);
	}

	if( (*(vu32*)0x80000018) == 0x5D1C9EA3 )
	{
		DiscType = DISC_RVL;
		gprintf("DiscThread:RVL-Disc\n" );
	} else {
		if( (*(vu32*)0x8000001C) == 0xC2339F3D )
		{
			DiscType = DISC_DOL;
			gprintf("DiscThread:DOL-Disc\n" );
		} else {

			gprintf("DiscThread:Invalid Disc (magic fail)!\n");

			GameReady=-1;
			GameUpdate=1;
			while(1)sleep(1);
		}
	}

	u8 *buffer = memalign( 32, 0x20 );

	if( DiscType == DISC_RVL )
	{
		r = DVDLowUnencryptedRead( buffer, 0x20, 0x40000>>2);
		if( r != 1 )
		{
			gprintf("DiscThread:DVDLowUnencryptedRead():%d\n", r );
			GameReady=-9;
			GameUpdate=1;
			while(1)sleep(1);
		}

		DCFlushRange(buffer, 0x20);

		int partitions = ((u32*)buffer)[0];
		int partition_info_offset = ((u32*)buffer)[1] << 2;

		r = DVDLowUnencryptedRead( buffer, 0x20, partition_info_offset>>2 );
		if( r != 1 )
		{
			gprintf("DiscThread:DVDLowUnencryptedRead():%d\n", r );
			GameReady=-9;
			GameUpdate=1;
			while(1)sleep(1);
		}

		int i;
		for( i=0; i < partitions; i++)
		{
			if( ((u32*)buffer)[i*2+1] == 0 )
				GamePartitionOffset = ((u32*)buffer)[i*2]<<2;

			if( i*2 > 0x20 )
				break;
		}

		if( GamePartitionOffset == 0xDEADBEEF )
		{
			gprintf("DiscThread:Could not find game partition\n");
			GameReady=-10;
			GameUpdate=1;
			while(1)sleep(1);
		}

		gprintf("DiscThread:Game Partition:%08X\n", GamePartitionOffset );

	}

	memset( GameTitle, 0, 0x40 );
	r = DVDLowUnencryptedRead( GameTitle, 0x40, 0x20>>2 );
	if( r != 1 )
	{
		gprintf("DiscThread:DVDLowUnencryptedRead():%d\n", r );
		GameReady=-9;
		GameUpdate=1;
		while(1)sleep(1);
	}

	GameReady=1;
	GameUpdate=1;

	free( buffer );

	while(1)sleep(1);
}
void RunGame( u32 VideoMode )
{
	gprintf("Game:RunGame->%s\n",  *(vu8*)0x80000020 );

	if( DiscType == DISC_DOL )
	{
		//Use country code for the VI mode
		switch( *(vu8*)0x80000003 )
		{
			case 'E':
				gprintf("Game:RunGame->VideoMode(USA)\n");
				*(vu32*)0x800000CC = 0x00000000;
			break;
			case 'J':
				gprintf("Game:RunGame->VideoMode(JAP)\n");
				*(vu32*)0x800000CC = 0x00000000;
			break;
			case 'P':
				gprintf("Game:RunGame->VideoMode(EUR)\n");
				*(vu32*)0x800000CC = 0x00000005;
			break;
			default:
				gprintf("Game:RunGame->VideoMode(0x%x)\n", *(vu8*)0x80000003 );
				*(vu32*)0x800000CC = 0x00000000;
			break;
		}

		DVDClose();

		WPAD_Shutdown();

		*(vu32 *)0xCC003024 |= 7;

		u64 TitleID = 0x0000000100000100LL;

		u32 cnt ATTRIBUTE_ALIGN(32);

		s32 DiscError = ES_GetNumTicketViews(TitleID, &cnt);
		gprintf("Game:RunGame->ES_GetNumTicketViews(%d):%d\n", cnt, DiscError );

		tikview *views = (tikview *)memalign( 32, sizeof(tikview)*cnt );

		DiscError = ES_GetTicketViews(TitleID, views, cnt);
		gprintf("Game:RunGame->ES_GetTicketViews():%d\n", DiscError );

		gprintf("Game:RunGame->Starting DOL disc!\n");
		DiscError = ES_LaunchTitle(TitleID, &views[0]);	
		gprintf("Game:RunGame->ES_LaunchTitle():%d\n", DiscError );

		GameReady=-4;
		GameUpdate=1;
		return;
	}

	void	(*app_init)(void (*report)(const char *fmt, ...));
	int		(*app_main)(void **dst, int *size, int *offset);
	void	(*app_entry)(void(**init)(const char *fmt, ...), int (**main)(), void *(**final)());
	void	(*entrypoint)();
	void *	(*app_final)();

	//DVDClose();

	//WPAD_Shutdown();

	//unsigned int sysver = *(u32*)(((u8*)_TMD)+0x188);

	//free( _TMD );
	//memcpy( (u32*)0x80003000, lowmem_bin+0x3000, 0x200 );

	//r = __IOS_LaunchNewIOS(248);
	//gprintf("Game:RunGame->IOS_ReloadIOS(%d):%d\n", sysver, r );
	//if( r < 0 )
	//{
	//	GameReady=-4;
	//	GameUpdate=1;
	//	return;
	//}

	//r = DI_Init();
	//gprintf("Game:RunGame->DI_Init():%d\n", r );
	//if( r < 0 )
	//{
	//	GameReady=-4;
	//	GameUpdate=1;
	//	return;
	//}

	//r = DVDLowClosePartition();
	//gprintf("Game:RunGame->DVDLowClosePartition():%d\n", r );
	//if( r < 0 )
	//{
	//	GameReady=-4;
	//	GameUpdate=1;
	//	return;
	//}

	//r = DVDLowStopMotor();
	//gprintf("Game:RunGame->DVDLowStopMotor():%d\n", r );
	//if( r < 0 )
	//{
	//	GameReady=-4;
	//	GameUpdate=1;
	//	return;
	//}

	//r = DI_Reset();
	//gprintf("Game:RunGame->DI_Reset():%d\n", r );
	//if( r != 0 )
	//{
	//	GameReady=-4;
	//	GameUpdate=1;
	//	return ;
	//}

	//r = DI_ReadDiscID( (void*)0x80000000 );
	//gprintf("Game:RunGame->DI_ReadDiscID():%d\n", r );
	//if( r != 0 )
	//{
	//	GameReady=-4;
	//	GameUpdate=1;
	//	return ;
	//}

	//s32 r = DI_OpenPartition( GamePartitionOffset>>2 );
	//gprintf("Game:RunGame->DI_OpenPartition(%08x):%d\n", GamePartitionOffset>>2, r );
	//if( r != 0 )
	//{
	//	GameReady=-4;
	//	GameUpdate=1;
	//	return;
	//}


	tmd *_TMD = (tmd*)memalign( 32, 0x49E4 );

	DiscError = DVDLowOpenPartition( GamePartitionOffset, (u8*)_TMD );

	gprintf("Game:RunGame->DVDLowOpenPartition():%d\n", DiscError );
	if( DiscError != 1 )
	{
		u32 error;
		DVDLowRequestError( &error );
		gprintf("DiscThread:DVDLowOpenPartition():%d ERROR:%X\n", DiscError, error );

		GameReady=-11;
		GameUpdate=1;
		return;
	}

	free( _TMD );

	ISFS_Deinitialize();
	ISFS_Initialize();

	u8 *buffer = (u8*)memalign( 32, 0x20 );

	DiscError = DVDLowRead( buffer, 0x20, 0x2440>>2);
	gprintf("Game:RunGame->DVDLowRead( %08x, %08x, %08x):%d\n", buffer, 0x20, 0x2440>>2, DiscError );
	if( DiscError != 1 )
	{
		GameReady=-12;
		GameUpdate=1;
		return;
	}

	gprintf("Game:RunGame->DVDLowRead( %08x, %08x, %08x)\n", (void*)0x81200000, *(unsigned long*)(buffer+0x14)+*(unsigned long*)(buffer+0x18), 0x2460>>2 );
	DiscError=DVDLowRead( (void*)0x81200000, *(unsigned long*)(buffer+0x14)+*(unsigned long*)(buffer+0x18), 0x2460>>2 );
	if( DiscError != 1 )
	{
		GameReady=-12;
		GameUpdate=1;
		return;
	}

	free( buffer );

	*(vu32*)0x8000002C = 0x00000023;				// Console type
	*(vu32*)0x800000F8 = 0x0E7BE2C0;				// Bus Clock Speed
	*(vu32*)0x800000FC = 0x2B73A840;				// CPU Clock Speed

	app_entry = *(unsigned long*)(buffer+0x10);
	gprintf("Apploader Entry:%x\n", app_entry );

	app_entry(&app_init, &app_main, &app_final);
	gprintf("Apploader Init: %08x\n", app_init);
	gprintf("Apploader Main: %08x\n", app_main);
	gprintf("Apploader Final:%08x\n", app_final);

	app_init(gprintf);

	void *dst=0;
	int lenn, offset;

	while (1)
	{
		lenn=0;
		offset=0;
		dst=0;

		if( app_main(&dst, &lenn, &offset) != 1 )
			break;

		gprintf("Game:RunGame->DVDLowRead( %x, %x, %x):", dst, lenn, offset, DiscError );
		DiscError = DVDLowRead(dst, lenn, offset );
		gprintf("%d\n", DiscError );

		if( DiscError != 1 )
		{
			GameReady=-12;
			GameUpdate=1;
			return;
		}

		if(ApplyPatch( (void*)dst, lenn, PATTERN_FWRITE, PATCH_FWRITE ))
			gprintf("Game:RunGame->Found __fwrite\n");

		DCFlushRange(dst, lenn);
	}

	//Use country code for the VI mode
	switch( *(vu8*)0x80000003 )
	{
		case 'E':
			gprintf("Game:RunGame->VideoMode(USA)\n");
			*(vu32*)0x800000CC = 0x00000000;
		break;
		case 'J':
			gprintf("Game:RunGame->VideoMode(JAP)\n");
			*(vu32*)0x800000CC = 0x00000000;
		break;
		case 'P':
			gprintf("Game:RunGame->VideoMode(EUR)\n");
			*(vu32*)0x800000CC = 0x00000005;
		break;
		default:
			gprintf("Game:RunGame->VideoMode(0x%x)\n", *(vu8*)0x80000003 );
			*(vu32*)0x800000CC = 0x00000000;
		break;
	}

	*(vu32*)0x80003188 = *(vu32*)0x80003140;				// 002 fix

	settime(secs_to_ticks(time(NULL) - 946684800));

	DVDClose();

	__ES_Close();

	SYS_ResetSystem(SYS_SHUTDOWN,0,0);
	__IOS_ShutdownSubsystems();

	entrypoint = app_final();

	gprintf("Game:RunGame->entrypoint(%x)\n", entrypoint );
	entrypoint();

	gprintf("Game:RunGame->Error game returned!!\n" );

	return;
}
