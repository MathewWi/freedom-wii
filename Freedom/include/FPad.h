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

typedef struct {
	char	*ButtonName;
	u32		ButtonValue;
} WiimoteNames;

enum ControllerChannel { WPAD_CHANNEL_0=1, WPAD_CHANNEL_1=2, WPAD_CHANNEL_2=4, WPAD_CHANNEL_3=8, WPAD_CHANNEL_ALL=15,
						 PAD_CHANNEL_0=16, PAD_CHANNEL_1=32, PAD_CHANNEL_2=64, PAD_CHANNEL_3=128, PAD_CHANNEL_ALL=240,
						/* CHANNEL_ALL=255*/
						};

enum Buttons { LEFT, RIGHT, UP, DOWN, OK, CANCEL, START, SPECIAL_1, SPECIAL_2, SPECIAL_3, SPECIAL_4 };
enum WiiMoteControlType { Default, Sideways, Custom };

s32 FPad_Init( void );
s32 FPad_LoadMapping( void );
s32 FPad_SaveMapping( void );
s32 FPad_Update( void );
u32 FPad_IsPressed( u32 Channel, u32 ButtonMask );
u32 FPad_Pressed( u32 Button );
void FPad_ClearButtonMapping( u32 Controller );
u32 FPadGetButtonMapping( u32 Controller, u32 Button );
u32 FPadSetButtonMapping( u32 Controller, u32 Button, u32 FButton );
char *FPadGetButtonName( u32 Button );
