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
#include "Identify.h"

extern "C" {
	#include "misc.h"
}

//#define IDENT_DEBUG
//#define DEVCERTS


s32 ID_Sysmenu( void )
{
	u64 titleID;
	ES_GetTitleID( &titleID);

	if( titleID == 0x0000000100000002LL )
		return 0;

	static u32 tempKeyID ATTRIBUTE_ALIGN(32)=0;
#ifdef DEVCERTS
	s32 r = ES_Identify((signed_blob*)devcerts_bin, 0xA00, (signed_blob*)sys_tmd, sys_tmd_size, (signed_blob*)sys_tik, sys_tik_size, &tempKeyID);
#else
	s32 r = ES_Identify((signed_blob*)certs_bin, 0xA00, (signed_blob*)sys_tmd, sys_tmd_size, (signed_blob*)sys_tik, sys_tik_size, &tempKeyID);
#endif
	gprintf("Identify:ID_Sysmenu->ES_Identify():%d\n", r );

	//fatUnmount("sd");
	ISFS_Deinitialize();
	ISFS_Initialize();
	//SDCard_Init();

	return r;
}
s32 ID_Root( void )
{
	static u32 tempKeyID ATTRIBUTE_ALIGN(32)=0;

#ifdef DEVCERTS
	s32 r = ES_Identify((signed_blob*)devcerts_bin, 0xA00, (signed_blob*)su_tmd, su_tmd_size, (signed_blob*)su_tik, su_tik_size, &tempKeyID);
#else
	s32 r = ES_Identify((signed_blob*)certs_bin, 0xA00, (signed_blob*)su_tmd, su_tmd_size, (signed_blob*)su_tik, su_tik_size, &tempKeyID);
#endif
	gprintf("Identify:ID_Root->ES_Identify():%d\n", r );

	//fatUnmount("sd");
	ISFS_Deinitialize();
	ISFS_Initialize();
	//SDCard_Init();

	return r;
}
s32 ID_Title( u64 TitleID )
{
	u64 titleID;
	s32 r=0;

	ES_GetTitleID( &titleID );
	if( titleID != 0x0000000100000002LL )
	{
		r = ID_Sysmenu();
		if( r < 0)
			return r;
	}

	r = ES_SetUID(TitleID);
	gprintf("Identify:ID_Title->ES_SetUID(%x-%x):%d\n", (u32)(TitleID>>32), (u32)(TitleID&0xFFFFFFFF), r );

	//fatUnmount("sd");
	ISFS_Deinitialize();
	ISFS_Initialize();
	//SDCard_Init();

	return r;
}
s32 ID_Channel( u64 TitleID )
{
	static u32 tmd_size ATTRIBUTE_ALIGN(32);

	s32 r = ID_Root();
	if( r < 0 )
		return r;

	//read ticket from FS
	char *file=(char*)memalign(32, 1024);
	sprintf( file, "/ticket/%08x/%08x.tik", (u32)(TitleID>>32), (u32)(TitleID&0xFFFFFFFF) );
	s32 fd = IOS_Open( file, 1 );
#ifdef IDENT_DEBUG
	gprintf("Identify:ID_Channel->IOS_Open(%s):%d\n", file, r );
#endif
	if( fd < 0 )
	{
		return fd;
	}

	//get size
	fstats * tstatus = (fstats*)memalign( 32, sizeof( fstats ) );
	r = ISFS_GetFileStats( fd, tstatus );
#ifdef IDENT_DEBUG
	gprintf("Identify:ID_Channel->ISFS_GetFileStats():%d\n", r );
#endif
	if( r < 0 )
	{
		IOS_Close( fd );
		return -1;
	}

	//create buffer
	char * TicketData = (char*)memalign( 32, (tstatus->file_length+31)&(~31) );
	if( TicketData == NULL )
	{
		IOS_Close( fd );
		return -1;
	}
	memset(TicketData, 0, (tstatus->file_length+31)&(~31) );

	//read file
	r = IOS_Read( fd, TicketData, tstatus->file_length );
#ifdef IDENT_DEBUG
	gprintf("Identify:ID_Channel->IOS_Read():%d\n", r );
#endif
	if( r < 0 )
	{
		free( TicketData );
		IOS_Close( fd );
		return r;
	}

	IOS_Close( fd );


	//read TMD from FS
	sprintf( file, "/title/%08x/%08x/content/title.tmd", (u32)(TitleID>>32), (u32)(TitleID&0xFFFFFFFF) );
	fd = IOS_Open( file, 1 );
#ifdef IDENT_DEBUG
	//gprintf("Identify:ID_Channel->IOS_Open(%s):%d\n", file, fd );
#endif
	if( fd < 0 )
	{
		return fd;
	}

	//get size
	tstatus = (fstats*)memalign( 32, sizeof( fstats ) );
	r = ISFS_GetFileStats( fd, tstatus );
#ifdef IDENT_DEBUG
	gprintf("Identify:ID_Channel->ISFS_GetFileStats():%d\n", r );
#endif
	if( r < 0 )
	{
		IOS_Close( fd );
		return -1;
	}

	//create buffer
	char * TMDData = (char*)memalign( 32, (tstatus->file_length+31)&(~31) );
	if( TMDData == NULL )
	{
		IOS_Close( fd );
		return -1;
	}
	memset(TMDData, 0, (tstatus->file_length+31)&(~31) );

	//read file
	r = IOS_Read( fd, TMDData, tstatus->file_length );
#ifdef IDENT_DEBUG
	gprintf("Identify:ID_Channel->IOS_Read():%d\n", r );
#endif
	if( r < 0 )
	{
		free( TMDData );
		IOS_Close( fd );
		return r;
	}

	IOS_Close( fd );

	static u32 tempKeyID ATTRIBUTE_ALIGN(32)=0;
#ifdef DEVCERTS
	r = ES_Identify((signed_blob*)devcerts_bin, 0xA00, (signed_blob*)TMDData, tstatus->file_length, (signed_blob*)TicketData, 0x2A4, &tempKeyID);
#else
	r = ES_Identify((signed_blob*)certs_bin, 0xA00, (signed_blob*)TMDData, tstatus->file_length, (signed_blob*)TicketData, 0x2A4, &tempKeyID);
#endif
	gprintf("Identify:ID_Channel->ES_Identify():%d\n", r );

	//fatUnmount("sd");
	ISFS_Deinitialize();
	ISFS_Initialize();
	//SDCard_Init();


	free( TMDData );
	free( TicketData );

	return r;

}
