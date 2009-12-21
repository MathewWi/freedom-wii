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
#include "Config.h"

extern "C"
{
	extern syssramex* __SYS_LockSramEx();
	extern u32 __SYS_UnlockSramEx(u32 write);
	extern u32 __SYS_SyncSram();
	#include "misc.h"
}

char *AreaStr[] = 
{
	"JPN",
	"USA",
	"EUR",
	"KOR",
};

char *VideoStr[] = 
{
	"NTSC",
	"PAL",
	"MPAL",
};

char *GameRegionStr[] = 
{
	"JP",
	"US",
	"EU",
	"KR",
};


u32 ConfigLoaded=0;
u8 *SettingsBuf=NULL;
u8 *SysconfBuf=NULL;
u8 *NetworkBuf=NULL;
u8 *GCSRAM=NULL;
SysConfigHeader *SysConfHdr=NULL;
NetworkConfig *NetworkCfg=NULL;

s32 Config_InitSetting( void )
{
	s32 fd = ISFS_Open("/title/00000001/00000002/data/setting.txt", 1 );
#ifdef CONFIG_DEBUG
	gprintf("Config:Config_Init->ISFS_Open():%d\n", fd );
#endif
	if( fd < 0 )
		return fd;

	SettingsBuf = (u8*)memalign( 32, 256 );
	if( SettingsBuf == NULL )
		return -1;

	s32 r = ISFS_Read( fd, SettingsBuf, 256 );
#ifdef CONFIG_DEBUG
	gprintf("Config:Config_Init->ISFS_Read():%d\n", r );
#endif
	if( r < 0 || r != 256 )
	{
		return -2;
	}
	ISFS_Close( fd );
#ifdef CONFIG_DEBUG
	gprintf("Config:Config_Init->ISFS_Close()\n" );
#endif

	//Decrypt buffer

	u32 magic = 0x73B5DBFA;

	for( u32 i=0; i < 256; ++i )
	{
		SettingsBuf[i] = SettingsBuf[i] ^ (magic&0xFF);
		magic = (magic<<1) | (magic>>31);
	}

#ifdef CONFIG_DEBUG
	gprintf("%s", SettingsBuf );
#endif
	return 1;
}
s32 Config_InitSYSCONF( void )
{
	s32 fd = ISFS_Open("/shared2/sys/SYSCONF", 1 );
#ifdef CONFIG_DEBUG
	gprintf("Config:Config_Init->ISFS_Open():%d\n", fd );
#endif
	if( fd < 0 )
		return fd;

	SysconfBuf = (u8*)memalign( 32, 0x4000 );
	if( SettingsBuf == NULL )
		return -3;

	s32 r = ISFS_Read( fd, SysconfBuf, 0x4000 );
#ifdef CONFIG_DEBUG
	gprintf("Config:Config_Init->ISFS_Read():%d\n", r );
#endif
	if( r < 0 || r != 0x4000 )
	{
		return -4;
	}
	ISFS_Close( fd );
#ifdef CONFIG_DEBUG
	gprintf("Config:Config_Init->ISFS_Close()\n" );
#endif

	SysConfHdr = (SysConfigHeader*)SysconfBuf;

	gprintf("Config:Config_Init->Sysconf Items:%d\n", SysConfHdr->ItemCount );

	return 1;
}
s32 Config_InitNetwork( void )
{
	s32 fd = ISFS_Open("/shared2/sys/net/02/config.dat", 1 );
#ifdef CONFIG_DEBUG
	gprintf("Config:Config_Init->ISFS_Open():%d\n", fd );
#endif
	if( fd < 0 )
	{
		fd = ISFS_Open("/shared2/sys/net/config.dat", 1 );
#ifdef CONFIG_DEBUG
		gprintf("Config:Config_Init->ISFS_Open():%d\n", fd );
#endif
		if( fd < 0 )
			return fd;
	}

	NetworkBuf = (u8*)memalign( 32, 0x2000 );
	if( NetworkBuf == NULL )
		return -3;

	s32 r = ISFS_Read( fd, NetworkBuf, 7004 );
#ifdef CONFIG_DEBUG
	gprintf("Config:Config_Init->ISFS_Read():%d\n", r );
#endif
	if( r < 0 || r != 7004 )
	{
		return -4;
	}
	ISFS_Close( fd );
#ifdef CONFIG_DEBUG
	gprintf("Config:Config_Init->ISFS_Close()\n" );
#endif

	NetworkCfg = (NetworkConfig*)NetworkBuf;

	return 1;
}
s32 Config_InitSRAM( void )
{
	GCSRAM = (u8*)__SYS_LockSramEx();
	__SYS_UnlockSramEx( 0 );

	if( GCSRAM != NULL )
	{
		//hexdump( GCSRAM, 64);
		return 1;
	}
	return 0;
}
s32	Config_Init( void )
{
	if( ConfigLoaded )
	{
		gprintf("Config:Config_Init->Config alread loaded\n" );
		return 1;
	}

	s32 r = Config_InitSetting();
	gprintf("Config_Init:InitSetting():%d\n", r );

	r = Config_InitSYSCONF();
	gprintf("Config_Init:InitSYSCONF():%d\n", r );

	r = Config_InitNetwork();
	gprintf("Config_Init:Config_InitNetwork():%d\n", r );

	r = Config_InitSRAM();
	gprintf("Config_Init:Config_InitSRAM():%d\n", r );

	Config_OSSetEUR60(Config_GetEur60());
	Config_OSSetProgressive(Config_GetProgressive());

	if( Config_GetSystemLanguage() == Japanse )
		Config_OSSetLanguage(0);
	else
		Config_OSSetLanguage(Config_GetSystemLanguage()-1);

	//Config_OSSetEUR60(1);
	//Config_OSSetProgressive(0);
	//Config_OSSetLanguage(English+1);
	//Config_OSSetVideMode( 1 );

	//{
	//	char *path = (char*)memalign( 32, 1024 );

	//	sprintf( path, "/shared2" );
	//	s32 r = ISFS_CreateDir( path, 0, 3, 3, 3 );
	//	sprintf( path, "/shared2/sys" );
	//	r = ISFS_CreateDir( path, 0, 3, 3, 3 );
	//	sprintf( path, "/shared2/sys/net" );
	//	r = ISFS_CreateDir( path, 0, 3, 3, 3 );
	//	sprintf( path, "/shared2/sys/net/02" );
	//	r = ISFS_CreateDir( path, 0, 3, 3, 3 );

	//	gprintf("ISFS_CreateFile():%d\n", r );

	//	sprintf( path, "/shared2/sys/net/02/config.dat" );
	//	r = ISFS_CreateFile( path, 0, 3, 3, 3 );

	//	gprintf("ISFS_CreateFile():%d\n", r );
	//	s32 fd = IOS_Open( path, 2 );
	//	gprintf("IOS_Open():%d\n", fd );

	//	NetworkConfig *ncfg = (NetworkConfig*)memalign( 32, sizeof(NetworkConfig) );
	//	memset( ncfg, 0, sizeof( NetworkConfig ) );

	//	ncfg->header = 0x0000000001070000LL;
	//	ncfg->Connection[0].Selected= 0xA6;
	//	ncfg->Connection[0].IP		= 0xC0A8011F;
	//	ncfg->Connection[0].Netmask	= 0xFFFFFF00;
	//	ncfg->Connection[0].Gateway	= 0xC0A80163;
	//	ncfg->Connection[0].DNS		= 0xC0A80163;

	//	sprintf( ncfg->Connection[0].ssid, "BrinkOfTime");
	//	ncfg->Connection[0].ssid_length = strlen("BrinkOfTime");

	//	ncfg->Connection[0].encryption = 6;

	//	sprintf( ncfg->Connection[0].key, "x2ZAspuyuwed");
	//	ncfg->Connection[0].key_length = strlen("x2ZAspuyuwed");

	//	r = IOS_Write( fd, ncfg, sizeof( NetworkConfig ) );
	//	gprintf("IOS_Write():%d\n", r );

	//	IOS_Close( fd );

	//	free( path );
	//}

	ConfigLoaded=1;

	//for( u32 i=0; i < SysConfHdr->ItemCount; ++i )
	//{
	//	u32 type = SysconfBuf[(SysConfHdr->ItemOffset[i])]>>5;
	//	u32 NameLen = (SysconfBuf[(SysConfHdr->ItemOffset[i])]&0x1F)+1;
	//	u32 value = 0;
	//	switch( type )
	//	{
	//		case 3:
	//			value = *(u8*)(SysconfBuf+SysConfHdr->ItemOffset[i] + 1 + NameLen);
	//			break;
	//		case 4:
	//			value = *(u16*)(SysconfBuf+SysConfHdr->ItemOffset[i] + 1 + NameLen);
	//			break;
	//		case 5:
	//			value = *(u32*)(SysconfBuf+SysConfHdr->ItemOffset[i] + 1 + NameLen);
	//			break;
	//	}

	//	char *s = new char[NameLen+1];
	//	memset( s, 0, NameLen+1 );
	//	memcpy( s, SysconfBuf+(SysConfHdr->ItemOffset[i] + 1), NameLen );

	//	gprintf("%X:%s:%d\n", type, s, value );

	//	free( s );
	//}


	return 1;
}
u32 Config_OSFlush( void )
{
	*(u16*)(GCSRAM) = 0;
	*(u16*)(GCSRAM+2) = 0;

    for( u32 i = 0;i<4;++i) 
    { 
        *(u16*)(GCSRAM)		+=  GCSRAM[0x06 + i]; 
        *(u16*)(GCSRAM+2)	+= (GCSRAM[0x06 + i] ^ 0xFFFF); 
    } 
	return __SYS_UnlockSramEx( 1 );
}
u32	Config_OSGetLanguage( void )
{
	return GCSRAM[0x12];
}
u32 Config_OSSetLanguage( u8 Language )
{
	GCSRAM[0x12] = Language;
	__SYS_UnlockSramEx( 1 );

	return Config_OSFlush();
}
u32	Config_OSGetSoundMode( void )
{
	return (GCSRAM[0x13]>>2)&1;
}
u32 Config_OSSetSoundMode( u8 SoundMode )
{
	GCSRAM[0x13] &= ~(1<<2);
	GCSRAM[0x13] |= (SoundMode&1);
	__SYS_UnlockSramEx( 1 );

	return Config_OSFlush();
}
u32	Config_OSGetEUR60( void )
{
	return (GCSRAM[0x11]>>6)&1;
}
u32 Config_OSSetEUR60( u8 EUR60Mode )
{
	GCSRAM[0x11] &= ~(1<<6);
	GCSRAM[0x11] |= (EUR60Mode&1)<<6;
	__SYS_UnlockSramEx( 1 );

	return Config_OSFlush();
}
u32	Config_OSGetProgressive( void )
{
	return (GCSRAM[0x13]>>7)&1;
}
u32 Config_OSSetProgressive( u8 ProgressiveMode )
{
	GCSRAM[0x13] &= ~((1<<7)|1);
	GCSRAM[0x13] |= (ProgressiveMode&1)<<7 | (ProgressiveMode&1);
	__SYS_UnlockSramEx( 1 );

	return Config_OSFlush();
}
u32 Config_OSSetVideMode( u8 VideMode )
{
	GCSRAM[0x13] &= 0x1F8;

	if( VideMode <= 2 )
		GCSRAM[0x13] |= VideMode;

	__SYS_UnlockSramEx( 1 );

	return Config_OSFlush();
}
u32	Config_OSGetVideMode( void )
{
	return GCSRAM[0x13]&7;
}
u32 Config_NetSlotUsed( u32 SlotID )
{
	if( SlotID > 2 )
		return 0;

	return NetworkCfg->Connection[SlotID].Selected & (1<<5);
}
u32 Config_NetSlotSelected( u32 SlotID )
{
	if( SlotID > 2 )
		return 0;

	return NetworkCfg->Connection[SlotID].Selected & (1<<7);
}
u32 Config_NetGetConnectionType( u32 SlotID )
{
	if( SlotID > 2 )
		return 0;

	return NetworkCfg->Connection[SlotID].Selected & (1<<1);
}
u32 Config_NetGetDHCPStatus( u32 SlotID )
{
	if( SlotID > 2 )
		return 0;

	return NetworkCfg->Connection[SlotID].Selected & (1<<2);
}
ConnectionSetting *Config_NetGetConnectionSetting( u32 SlotID )
{
	if( SlotID > 2 )
		return NULL;

	return &NetworkCfg->Connection[SlotID];
}
u32 Config_NetSaveConnection( u32 SlotID, ConnectionSetting *cs )
{
	if( SlotID > 2 )
		return 0;

	memcpy( &(NetworkCfg->Connection[SlotID]), cs, sizeof( ConnectionSetting ) );

	return 1;
}
u32 Config_NetSaveConfig( void )
{
	s32 fd = ISFS_Open("/shared2/sys/net/02/config.dat", 2 );
#ifdef CONFIG_DEBUG
	gprintf("Config:Config_Init->ISFS_Open():%d\n", fd );
#endif
	if( fd < 0 )
	{
		fd = ISFS_Open("/shared2/sys/net/config.dat", 2 );
#ifdef CONFIG_DEBUG
		gprintf("Config:Config_Init->ISFS_Open():%d\n", fd );
#endif
		if( fd < 0 )
			return fd;
	}

	s32 r = ISFS_Write( fd, NetworkBuf, 7004 );
#ifdef CONFIG_DEBUG
	gprintf("Config:Config_Init->ISFS_Write():%d\n", r );
#endif
	if( r < 0 || r != 7004 )
	{
		gprintf("Config:Config_Init->ISFS_Write():%d\n", r );
		return -4;
	}
	ISFS_Close( fd );
#ifdef CONFIG_DEBUG
	gprintf("Config:Config_Init->ISFS_Close()\n" );
#endif

	gprintf("Config:Config_NetSaveConfig ok\n");

	return 1;
}
u32 Config_UpdateSysconf( void )
{
	if( !ConfigLoaded )
		return -1;

//Load SYSCONF

	s32 fd = ISFS_Open("/shared2/sys/SYSCONF", 2 );
#ifdef CONFIG_DEBUG
	gprintf("Config:Config_Init->ISFS_Open():%d\n", fd );
#endif
	if( fd < 0 )
	{
		gprintf("Config:Config_Init->ISFS_Open():%d\n", fd );
		return fd;
	}
	s32 r = ISFS_Write( fd, SysconfBuf, 0x4000 );
#ifdef CONFIG_DEBUG
	gprintf("Config:Config_Init->ISFS_Write():%d\n", r );
#endif
	if( r < 0 || r != 0x4000 )
	{
		gprintf("Config:Config_Init->ISFS_Write():%d\n", r );
		return -4;
	}
	ISFS_Close( fd );
#ifdef CONFIG_DEBUG
	gprintf("Config:Config_Init->ISFS_Close()\n" );
#endif

	gprintf("Config:UpdateSysconf ok\n");

	return 1;
}
u8	Config_GetAspectRatio( void )
{
	if( !ConfigLoaded )
		return 0;

	for( u32 i=0; i < SysConfHdr->ItemCount; ++i )
	{
		if( memcmp( SysconfBuf+(SysConfHdr->ItemOffset[i] + 1), "IPL.AR", 6 ) == 0 )
		{
			return (u8)(SysconfBuf[SysConfHdr->ItemOffset[i] + 1 + 6 ]);
			break;
		}
	}
	return 0;
}
u32	Config_SetAspectRatio( u8 Value )
{
	if( !ConfigLoaded )
		return 0;

	for( u32 i=0; i < SysConfHdr->ItemCount; ++i )
	{
		if( memcmp( SysconfBuf+(SysConfHdr->ItemOffset[i] + 1), "IPL.AR", 6 ) == 0 )
		{
			*(u8*)(SysconfBuf+SysConfHdr->ItemOffset[i] + 1 + 6) = Value;
			break;
		}
	}
	return 0;
}

u32	Config_GetBTSensitivity( void )
{
	if( !ConfigLoaded )
		return 0;

	for( u32 i=0; i < SysConfHdr->ItemCount; ++i )
	{
		if( memcmp( SysconfBuf+(SysConfHdr->ItemOffset[i] + 1), "BT.SENS", 7 ) == 0 )
		{
			return *(u32*)(SysconfBuf+SysConfHdr->ItemOffset[i] + 1 + 7);
			break;
		}
	}
	return 0;
}
u32	Config_SetBTSensitivity( u32 Value )
{
	if( !ConfigLoaded )
		return 0;

	for( u32 i=0; i < SysConfHdr->ItemCount; ++i )
	{
		if( memcmp( SysconfBuf+(SysConfHdr->ItemOffset[i] + 1), "BT.SENS", 7 ) == 0 )
		{
			*(u32*)(SysconfBuf+SysConfHdr->ItemOffset[i] + 1 + 7) = Value;
			break;
		}
	}
	return 0;
}
u8	Config_GetSensorBarPostion( void )
{
	if( !ConfigLoaded )
		return 0;

	for( u32 i=0; i < SysConfHdr->ItemCount; ++i )
	{
		if( memcmp( SysconfBuf+(SysConfHdr->ItemOffset[i] + 1), "BT.BAR", 6 ) == 0 )
		{
			return *(u8*)(SysconfBuf+SysConfHdr->ItemOffset[i] + 1 + 6);
			break;
		}
	}
	return 0;
}

u32	Config_SetSensorBarPostion( u8 Value )
{
	if( !ConfigLoaded )
		return 0;

	for( u32 i=0; i < SysConfHdr->ItemCount; ++i )
	{
		if( memcmp( SysconfBuf+(SysConfHdr->ItemOffset[i] + 1), "BT.BAR", 6 ) == 0 )
		{
			*(u8*)(SysconfBuf+SysConfHdr->ItemOffset[i] + 1 + 6) = Value;
			break;
		}
	}
	return 0;
}

u8	Config_GetSpeakerVolume( void )
{
	if( !ConfigLoaded )
		return 0;

	for( u32 i=0; i < SysConfHdr->ItemCount; ++i )
	{
		if( memcmp( SysconfBuf+(SysConfHdr->ItemOffset[i] + 1), "BT.SPKV", 7 ) == 0 )
		{
			return *(u8*)(SysconfBuf+SysConfHdr->ItemOffset[i] + 1 + 7);
			break;
		}
	}
	return 0;
}

u32	Config_SetSpeakerVolume( u8 Value )
{
	if( !ConfigLoaded )
		return 0;

	for( u32 i=0; i < SysConfHdr->ItemCount; ++i )
	{
		if( memcmp( SysconfBuf+(SysConfHdr->ItemOffset[i] + 1), "BT.SPKV", 7 ) == 0 )
		{
			*(u8*)(SysconfBuf+SysConfHdr->ItemOffset[i] + 1 + 7) = Value;
			break;
		}
	}
	return 0;
}

u8	Config_GetRumbleMode( void )
{
	if( !ConfigLoaded )
		return 0;

	for( u32 i=0; i < SysConfHdr->ItemCount; ++i )
	{
		if( memcmp( SysconfBuf+(SysConfHdr->ItemOffset[i] + 1), "BT.MOT", 6 ) == 0 )
		{
			return *(u8*)(SysconfBuf+SysConfHdr->ItemOffset[i] + 1 + 6);
			break;
		}
	}
	return 0;
}

u32	Config_SetRumbleMode( u8 Value )
{
	if( !ConfigLoaded )
		return 0;

	for( u32 i=0; i < SysConfHdr->ItemCount; ++i )
	{
		if( memcmp( SysconfBuf+(SysConfHdr->ItemOffset[i] + 1), "BT.MOT", 6 ) == 0 )
		{
			*(u8*)(SysconfBuf+SysConfHdr->ItemOffset[i] + 1 + 6) = Value;
			break;
		}
	}
	return 0;
}

u8	Config_GetSystemLanguage( void )
{
	if( !ConfigLoaded )
		return 0;

	for( u32 i=0; i < SysConfHdr->ItemCount; ++i )
	{
		if( memcmp( SysconfBuf+(SysConfHdr->ItemOffset[i] + 1), "IPL.LNG", 7 ) == 0 )
		{
			return *(u8*)(SysconfBuf+SysConfHdr->ItemOffset[i] + 1 + 7);
			break;
		}
	}
	return 0;
}

u32	Config_SetSystemLanguage( u8 Value )
{
	if( !ConfigLoaded )
		return 0;

	for( u32 i=0; i < SysConfHdr->ItemCount; ++i )
	{
		if( memcmp( SysconfBuf+(SysConfHdr->ItemOffset[i] + 1), "IPL.LNG", 7 ) == 0 )
		{
			*(u8*)(SysconfBuf+SysConfHdr->ItemOffset[i] + 1 + 7) = Value;
			break;
		}
	}
	return 0;
}
u8	Config_GetSoundMode( void )
{
	if( !ConfigLoaded )
	{
		gprintf("Erro config not loaded!\n");
		return 0;
	}

	for( u32 i=0; i < SysConfHdr->ItemCount; ++i )
	{
		if( memcmp( SysconfBuf+(SysConfHdr->ItemOffset[i] + 1), "IPL.SND", 7 ) == 0 )
		{
			return *(u8*)(SysconfBuf+SysConfHdr->ItemOffset[i] + 1 + 7);
			break;
		}
	}
	gprintf("Error Entry not found!\n");
	return 0;
}

u32	Config_SetSoundMode( u8 Value )
{
	if( !ConfigLoaded )
		return 0;

	for( u32 i=0; i < SysConfHdr->ItemCount; ++i )
	{
		if( memcmp( SysconfBuf+(SysConfHdr->ItemOffset[i] + 1), "IPL.SND", 7 ) == 0 )
		{
			*(u8*)(SysconfBuf+SysConfHdr->ItemOffset[i] + 1 + 7) = Value;
			break;
		}
	}
	return 0;
}

u8	Config_GetEur60( void )
{
	if( !ConfigLoaded )
		return 0;

	for( u32 i=0; i < SysConfHdr->ItemCount; ++i )
	{
		if( memcmp( SysconfBuf+(SysConfHdr->ItemOffset[i] + 1), "IPL.E60", 7 ) == 0 )
		{
			return *(u8*)(SysconfBuf+SysConfHdr->ItemOffset[i] + 1 + 7);
			break;
		}
	}
	return 0;
}

u32	Config_SetEur60( u8 Value )
{
	if( !ConfigLoaded )
		return 0;

	for( u32 i=0; i < SysConfHdr->ItemCount; ++i )
	{
		if( memcmp( SysconfBuf+(SysConfHdr->ItemOffset[i] + 1), "IPL.E60", 7 ) == 0 )
		{
			*(u8*)(SysconfBuf+SysConfHdr->ItemOffset[i] + 1 + 7) = Value;
			break;
		}
	}
	return 0;
}

u8	Config_GetProgressive( void )
{
	if( !ConfigLoaded )
		return 0;

	for( u32 i=0; i < SysConfHdr->ItemCount; ++i )
	{
		if( memcmp( SysconfBuf+(SysConfHdr->ItemOffset[i] + 1), "IPL.PGS", 7 ) == 0 )
		{
			return *(u8*)(SysconfBuf+SysConfHdr->ItemOffset[i] + 1 + 7);
			break;
		}
	}
	return 0;
}

u32	Config_SetProgressive( u8 Value )
{
	if( !ConfigLoaded )
		return 0;

	for( u32 i=0; i < SysConfHdr->ItemCount; ++i )
	{
		if( memcmp( SysconfBuf+(SysConfHdr->ItemOffset[i] + 1), "IPL.PGS", 7 ) == 0 )
		{
			*(u8*)(SysconfBuf+SysConfHdr->ItemOffset[i] + 1 + 7) = Value;
			break;
		}
	}
	return 0;
}


u8	Config_GetScreenSaver( void )
{
	if( !ConfigLoaded )
		return 0;

	for( u32 i=0; i < SysConfHdr->ItemCount; ++i )
	{
		if( memcmp( SysconfBuf+(SysConfHdr->ItemOffset[i] + 1), "IPL.SSV", 7 ) == 0 )
		{
			return *(u8*)(SysconfBuf+SysConfHdr->ItemOffset[i] + 1 + 7);
			break;
		}
	}
	return 0;
}

u32	Config_SetScreenSaver( u8 Value )
{
	if( !ConfigLoaded )
		return 0;

	for( u32 i=0; i < SysConfHdr->ItemCount; ++i )
	{
		if( memcmp( SysconfBuf+(SysConfHdr->ItemOffset[i] + 1), "IPL.SSV", 7 ) == 0 )
		{
			*(u8*)(SysconfBuf+SysConfHdr->ItemOffset[i] + 1 + 7) = Value;
			break;
		}
	}
	return 0;
}


u32	Config_GetCounterBias( void )
{
	if( !ConfigLoaded )
		return 0;

	for( u32 i=0; i < SysConfHdr->ItemCount; ++i )
	{
		if( memcmp( SysconfBuf+(SysConfHdr->ItemOffset[i] + 1), "IPL.CB", 6 ) == 0 )
		{
			return *(u32*)(SysconfBuf+SysConfHdr->ItemOffset[i] + 1 + 6);
			break;
		}
	}
	return 0;
}
u32	Config_SetCounterBias( u32 Value )
{
	if( !ConfigLoaded )
		return 0;

	for( u32 i=0; i < SysConfHdr->ItemCount; ++i )
	{
		if( memcmp( SysconfBuf+(SysConfHdr->ItemOffset[i] + 1), "IPL.CB", 6 ) == 0 )
		{
			*(u32*)(SysconfBuf+SysConfHdr->ItemOffset[i] + 1 + 6) = Value;
			break;
		}
	}
	return 0;
}
char *Config_GetSettingsString( char *Name )
{
	if( !ConfigLoaded )
		return NULL;

	char *s = strstr( (char*)SettingsBuf, Name );
#ifdef CONFIG_DEBUG
	gprintf("%s\n", s );
#endif
	if( s == NULL )
		return NULL;

	s += strlen(Name)+1; //ie: "AREA="

	//get value
	u32 len=0;
	while( isascii(s[len]) && (u8)(s[len])>0x20 )
	{
		len++;
		if( len > 100 )
			return NULL;
	}

#ifdef CONFIG_DEBUG
	gprintf("len:%d\n", len );
	gprintf("str:%s\n", s );
#endif
	if( len <= 0 )
	{
#ifdef CONFIG_DEBUG
		gprintf("s-str <= 0\n");
#endif
		return NULL;
	}

	char *c = new char[ len + 1 ];
	memcpy( c, s, len + 1 );
	c[len] = 0;

	return c;
}
u32 Config_GetSettingsValue( char *Name )
{
	char *s = Config_GetSettingsString( Name );

	gprintf("StrVal:\"%s\"\n", s );

	if( s == NULL )
	{
		gprintf("Config:GetSettingsValue(%s) Could not find value in setting.txt!\n", Name );
		return -1;
	}

	if( strcmp( Name, "AREA") == 0 )
	{
		for( u32 i=0; i < 4; ++i )
		{
			if( strcmp( s, AreaStr[i] ) == 0 )
			{
#ifdef CONFIG_DEBUG
				gprintf("Config:GetSettingsValue(%s) AREA:NTSC\n", Name );
#endif
				free( s );
				return i;
			}
		}

	} else if( strcmp( Name, "VIDEO") == 0 ) { 

		for( u32 i=0; i < 3; ++i )
		{
			if( strcmp( s, VideoStr[i] ) == 0 )
			{
#ifdef CONFIG_DEBUG
				gprintf("Config:GetSettingsValue(%s) VIDEO:NTSC\n", Name );
#endif
				free( s );
				return i;
			}
		}

	} else if( strcmp( Name, "GAME") == 0 ) { 

		for( u32 i=0; i < 4; ++i )
		{
			if( strcmp( s, GameRegionStr[i] ) == 0 )
			{
#ifdef CONFIG_DEBUG
				gprintf("Config:GetSettingsValue(%s) GAME:NTSC\n", Name );
#endif
				free( s );
				return i;
			}
		}

	}

	gprintf("Config:GetSettingsValue(%s):\"%s\" Could not find entry, or unknown value!\n", Name, s );
	free( s );

	return -2;
}
s32 Config_SetSettingsValue( char *Name, u32 value )
{
	char	*area = NULL,
			*model = NULL,
			*dvd = NULL,
			*mpch = NULL,
			*code = NULL,
			*serno = NULL,
			*video = NULL,
			*game = NULL;

	model = Config_GetSettingsString("MODEL");
	dvd = Config_GetSettingsString("DVD");
	mpch = Config_GetSettingsString("MPCH");
	code = Config_GetSettingsString("CODE");
	serno = Config_GetSettingsString("SERNO");

	if( strcmp( Name, "AREA" ) == 0 )
	{
		area = AreaStr[value];
		video = Config_GetSettingsString("VIDEO");
		game = Config_GetSettingsString("GAME");

	} else if( strcmp( Name, "VIDEO" ) == 0 )
	{
		area = Config_GetSettingsString("AREA");
		video = VideoStr[value];
		game = Config_GetSettingsString("GAME");

	} else if( strcmp( Name, "GAME" ) == 0 )
	{
		area = Config_GetSettingsString("AREA");
		video = Config_GetSettingsString("VIDEO");
		game = GameRegionStr[value];
	}
	
	if( model == NULL ||
		area == NULL ||
		dvd == NULL ||
		mpch == NULL ||
		code == NULL ||
		serno == NULL ||
		video == NULL ||
		game == NULL )
	{
		gprintf("Config:SetSettingsValue( \"%s\", %d )->One setting string returned NULL\n", Name, value );
	}

	memset( SettingsBuf, 0, 256 );

	sprintf( (char*)SettingsBuf, "AREA=%s\nMODEL=%s\nDVD=%s\nMPCH=%s\nCODE=%s\nSERNO=%s\nVIDEO=%s\nGAME=%s\n", area, model, dvd, mpch, code, serno, video, game );
#ifdef CONFIG_DEBUG
	gprintf( (char*)SettingsBuf );
#endif
	if( strcmp( Name, "AREA" ) == 0 )
	{
		free( game );
		free( video );

	} else if( strcmp( Name, "VIDEO" ) == 0 )
	{
		free( area );
		free( game );

	} else if( strcmp( Name, "GAME" ) == 0 )
	{
		free( area );
		free( video );
	}

	free( model );
	free( dvd );
	free( mpch );
	free( code );
	free( serno );

	return Config_SettingsFlush();
}
s32 Config_SettingsFlush( void )
{
	//Encrypt buffer

	u32 magic = 0x73B5DBFA;

	for( u32 i=0; i < 256; ++i )
	{
		SettingsBuf[i] = SettingsBuf[i] ^ (magic&0xFF);
		magic = (magic<<1) | (magic>>31);
	}

	s32 fd = ISFS_Open("/title/00000001/00000002/data/setting.txt", 2 );
#ifdef CONFIG_DEBUG
	gprintf("Config:Config_SettingsFlush->ISFS_Open():%d\n", fd );
#endif
	if( fd < 0 )
	{
		gprintf("Config:Config_SettingsFlush->Failed to open setting.txt, error:%d\n", fd );
		free( SysconfBuf );
		Config_InitSetting();
		return fd;
	}

	s32 r = ISFS_Write( fd, SettingsBuf, 256 );
#ifdef CONFIG_DEBUG
	gprintf("Config:Config_SettingsFlush->ISFS_Write():%d\n", r );
#endif
	if( r < 0 || r != 256 )
	{
		gprintf("Config:Config_SettingsFlush->Error writing to file, error:%d\n", fd );
		free( SysconfBuf );
		Config_InitSetting();
		return -2;
	}
	ISFS_Close( fd );
#ifdef CONFIG_DEBUG
	gprintf("Config:Config_SettingsFlush->ISFS_Close()\n" );
#endif

	free( SysconfBuf );
	Config_InitSetting();

#ifdef CONFIG_DEBUG
	gprintf("%s", SettingsBuf );
#endif
	return 1;
}
