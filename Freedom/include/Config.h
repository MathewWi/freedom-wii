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
#include <stdarg.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/errno.h>
#include <ogc/lwp_watchdog.h>

#include <sys/types.h>
#include <sys/errno.h>
#include <fcntl.h>

enum ConnectionType { CONNECTION_WIRELESS, CONNECTION_WIRED };

typedef struct
{
    u8 Selected;		// 0x26: not selected.  0xa6: selected
    u8 padding_1[3];	// 0x00
	u32 IP;
	u32 Netmask;
	u32 Gateway;
	u32 DNS;
	u8 passing_9[0x7B0];
    char ssid[32];		// Access Point name.
 
    u8 padding_2;		// 0x00
    u8 ssid_length;		// length of ssid[] (AP name) in bytes.
    u8 padding_3[2];	// 0x00
 
    u8 padding_4;		// 0x00
    u8 encryption;		// Encryption.  OPN: 0x00, WEP: 0x01, WPA-PSK (TKIP): 0x04, WPA2-PSK (AES): 0x05, WPA-PSK (AES): 0x06
    u8 padding_5[2];	// 0x00
 
    u8 padding_6;		// 0x00
    u8 key_length;		// length of key[] (encryption key) in bytes.
    u8 padding_7[2];	// 0x00
 
    char key[64];			// Encryption key
 
    u8 padding_8[236];	// 0x00
} ConnectionSetting;

struct NetworkConfig
{
    u64 header;		// 0x00000000 0x01070000
    ConnectionSetting Connection[3];
};

struct GC_SRAM 
{
	u16 CheckSum1;
	u16 CheckSum2;
	u32 ead0;
	u32 ead1;
	u32 CounterBias;
	u8	DisplayOffsetH;
	u8	ntd;
	u8	Language;
	u8	Flags;
	/*
		bit			desc			0		1
		0			Prog mode		off		on
		1			EU60			off		on
		2			Sound mode		Mono	Stereo
		3			always 1
		4			always 0
		5			always 1
		6			-\_ Video mode?
		7			-/
	*/
	u8	FlashID[2*12];
	u32	WirelessKBID;
	u16	WirlessPADID[4];
	u8	LastDVDError;
	u8	Reserved;
	u16	FlashIDChecksum[2];
	u16	Unused;
};

struct SysConfigHeader {
	u8		MagicBytes[4];
	u16		ItemCount;
	u16		ItemOffset[];
};


enum Area {
						JAP,
						USA,
						EUR,
						KOR
};

enum Game {
						JP,
						US,
						EU,
						KR,
};

enum Video {
						NTSC,
						PAL,
						MPAL,
};

enum Sound {			Monoral,
						Stereo,
						Surround
};

enum AspectRatio {		_4to3,
						_16to9
};

enum GeneralONOFF {		Off,
						On
};

enum SystemLanguage {	Japanse,
						English,
						German,
						French,
						Spanish,
						Italian,
						Dutch,
						ChineseSimple,
						ChineseTraditional,
						Korean
};

s32 Config_Init( void );
s32 Config_InitSRAM(void);

u32 Config_OSFlush( void );

u32	Config_OSGetLanguage( void );
u32 Config_OSSetLanguage( u8 Language );

u32	Config_OSGetEUR60( void );
u32 Config_OSSetEUR60( u8 EUR60Mode );

u32	Config_OSGetProgressive( void );
u32 Config_OSSetProgressive( u8 ProgressiveMode );

u32 Config_OSSetVideMode( u8 VideMode );
u32	Config_OSGetVideMode( void );

u32 Config_NetSaveConfig( void );
u32 Config_NetSlotUsed( u32 SlotID );
u32 Config_NetSlotSelected( u32 SlotID );
u32 Config_NetGetConnectionType( u32 SlotID );
u32 Config_NetGetDHCPStatus( u32 SlotID );
ConnectionSetting *Config_NetGetConnectionSetting( u32 SlotID );


u32 Config_UpdateSysconf( void );

u8	Config_GetAspectRatio( void );
u32	Config_SetAspectRatio( u8 Value );

u32 Config_GetBTSensitivity( void );
u32 Config_SetBTSensitivity( u32 Value );

u8	Config_GetSensorBarPostion( void );
u32	Config_SetSensorBarPostion( u8 Value );

u8	Config_GetSpeakerVolume( void );
u32	Config_SetSpeakerVolume( u8 Value );

u8	Config_GetRumbleMode( void );
u32	Config_SetRumbleMode( u8 Value );

u8 Config_GetSystemLanguage( void );
u32 Config_SetSystemLanguage( u8 Value );

u8	Config_GetSoundMode( void );
u32	Config_SetSoundMode( u8 Value );

u8	Config_GetEur60( void );
u32	Config_SetEur60( u8 Value );

u8	Config_GetProgressive( void );
u32	Config_SetProgressive( u8 Value );

u8	Config_GetScreenSaver( void );
u32	Config_SetScreenSaver( u8 Value );

u32	Config_GetCounterBias( void );
u32	Config_SetCounterBias( u32 Value );

char *Config_GetSettingsString( char *Name );
u32 Config_GetSettingsValue( char *Name );
s32 Config_SetSettingsValue( char *Name, u32 value );
s32 Config_SettingsFlush( void );
