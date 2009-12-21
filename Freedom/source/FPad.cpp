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
#include "FPad.h"

//#define FPAD_DEBUG

extern "C"
{
	#include "misc.h"
}

u32 WPADDown[4];
u32 PADDown[4];

u32 WPADHeld[4];
u32 PADHeld[4];

u32 WPADUp[4];
u32 PADUp[4];

u32 WButtonMap[10]={0};
u32 CButtonMap[10]={0};
u32 PButtonMap[10]={0};


WiimoteNames WiimoteName[] = {

	{ "A", WPAD_BUTTON_A },
	{ "B", WPAD_BUTTON_B },
	{ "Up", WPAD_BUTTON_UP },
	{ "Down", WPAD_BUTTON_DOWN },
	{ "Left", WPAD_BUTTON_LEFT },
	{ "Right", WPAD_BUTTON_RIGHT },
	{ "1", WPAD_BUTTON_1 },
	{ "2", WPAD_BUTTON_2 },
	{ "Plus", WPAD_BUTTON_PLUS },
	{ "Minus", WPAD_BUTTON_MINUS },
	{ "Home", WPAD_BUTTON_HOME },
};


s32 FPad_Init( void )
{
	s32 r = PAD_Init();
#ifdef FPAD_DEBUG
	gprintf("PAD_Init():%d\n", r);
	if( r < 0 )
		return r;
#endif

	r = WPAD_Init();
#ifdef FPAD_DEBUG
	gprintf("WPAD_Init():%d\n", r);
	if( r < 0 )
		return r;
#endif

	r = WPAD_SetDataFormat(WPAD_CHAN_0, WPAD_FMT_BTNS_ACC_IR);
#ifdef FPAD_DEBUG
	gprintf("WPAD_SetDataFormat():%d\n", r);
	if( r < 0 )
		return r;
#endif

	WPAD_SetIdleTimeout(60);

	//Add loading from file code!
	WButtonMap[LEFT] = WPAD_BUTTON_LEFT;
	CButtonMap[LEFT] = WPAD_CLASSIC_BUTTON_LEFT;
	PButtonMap[LEFT] = PAD_BUTTON_LEFT;

	WButtonMap[RIGHT] = WPAD_BUTTON_RIGHT;
	CButtonMap[RIGHT] = WPAD_CLASSIC_BUTTON_RIGHT;
	PButtonMap[RIGHT] = PAD_BUTTON_RIGHT;

	WButtonMap[DOWN] = WPAD_BUTTON_DOWN;
	CButtonMap[DOWN] = WPAD_CLASSIC_BUTTON_DOWN;
	PButtonMap[DOWN] = PAD_BUTTON_DOWN;

	WButtonMap[UP] = WPAD_BUTTON_UP;
	CButtonMap[UP] = WPAD_CLASSIC_BUTTON_UP;
	PButtonMap[UP] = PAD_BUTTON_UP;

	WButtonMap[OK] = WPAD_BUTTON_A;
	CButtonMap[OK] = WPAD_CLASSIC_BUTTON_A;
	PButtonMap[OK] = PAD_BUTTON_A;

	WButtonMap[CANCEL] = WPAD_BUTTON_B;
	CButtonMap[CANCEL] = WPAD_CLASSIC_BUTTON_B;
	PButtonMap[CANCEL] = PAD_BUTTON_B;

	WButtonMap[START] = WPAD_BUTTON_HOME;
	CButtonMap[START] = WPAD_CLASSIC_BUTTON_HOME;
	PButtonMap[START] = PAD_BUTTON_START;

	WButtonMap[SPECIAL_1] = WPAD_BUTTON_1;
	CButtonMap[SPECIAL_1] = WPAD_CLASSIC_BUTTON_X;
	PButtonMap[SPECIAL_1] = PAD_BUTTON_X;

	return 1;
}
s32 FPad_LoadMapping( void )
{
	FILE *f=fopen("sd:/freedom/mapping.bin", "rb" );

	if( f == NULL )
		return -1;

	fread( WButtonMap, sizeof(u32), 10, f );
	fread( CButtonMap, sizeof(u32), 10, f );
	fread( PButtonMap, sizeof(u32), 10, f );

	fclose( f );

	return 1;

}
s32 FPad_SaveMapping( void )
{
	FILE *f=fopen("sd:/freedom/mapping.bin", "wb" );

	if( f == NULL )
		return -1;

	fwrite( WButtonMap, sizeof(u32), 10, f );
	fwrite( CButtonMap, sizeof(u32), 10, f );
	fwrite( PButtonMap, sizeof(u32), 10, f );

	fclose( f );

	return 1;
}
s32 FPad_Update( void )
{
	WPAD_ScanPads();
	PAD_ScanPads();

	for( u32 i=0; i<4; ++i )
	{
		WPADDown[i]		= WPAD_ButtonsDown(i);
		WPADHeld[i]		= WPAD_ButtonsHeld(i);
		WPADUp[i]		= WPAD_ButtonsUp(i);

		PADDown[i]		= PAD_ButtonsDown(i);
		PADHeld[i]		= PAD_ButtonsHeld(i);
		PADUp[i]		= PAD_ButtonsUp(i);
	}

	return 1;
}
u32 FPad_IsPressed( u32 Channel, u32 ButtonMask )
{
	u32 value=0;
	for( u32 i=0; i < 8; ++i)
	{
		u32 bit = Channel&(1<<i);
		switch( bit )
		{
			case WPAD_CHANNEL_0:
				value |= WPADDown[0];
				break;

			case WPAD_CHANNEL_1:
				value |= WPADDown[1];
				break;

			case WPAD_CHANNEL_2:
				value |= WPADDown[2];
				break;

			case WPAD_CHANNEL_3:
				value |= WPADDown[3];
				break;

			case WPAD_CHANNEL_ALL:
				for( u32 i=0; i<4; ++i )
					value |= WPADDown[i];
				break;

			case PAD_CHANNEL_0:
				value |= PADDown[0];
				break;

			case PAD_CHANNEL_1:
				value |= PADDown[1];
				break;

			case PAD_CHANNEL_2:
				value |= PADDown[2];
				break;

			case PAD_CHANNEL_3:
				value |= PADDown[3];
				break;

			case PAD_CHANNEL_ALL:
				for( u32 i=0; i<4; ++i )
					value |= PADDown[i];
				break;

			//case CHANNEL_ALL:
			//	for( u32 i=0; i<4; ++i )
			//	{
			//		value |= WPADDown[i];
			//		value |= PADDown[i];
			//	}
			//	break;
		}
	}
	return (bool)(value & ButtonMask);
}
u32 FPad_Pressed( u32 Button )
{
	if( Button >= SPECIAL_4 )
		return 0;

	return	FPad_IsPressed( WPAD_CHANNEL_ALL, WButtonMap[Button]|CButtonMap[Button] ) |
			FPad_IsPressed( PAD_CHANNEL_ALL, PButtonMap[Button] );
}

void FPad_ClearButtonMapping( u32 Controller )
{
	if( Controller > 3 )
		return;

	for( u32 i=0; i < 10; ++i )
	{
		switch( Controller )
		{
			case 0:
				WButtonMap[i] = 0;
				break;
			case 1:
				CButtonMap[i] = 0;
				break;
			case 2:
				PButtonMap[i] = 0;
				break;
		}
	}
}
u32 FPadGetButtonMapping( u32 Controller, u32 Button )
{
	if( Controller > 3 )
		return 0;

	for( u32 i=0; i < 10; ++i )
	{
		switch( Controller )
		{
			case 0:
				if( WButtonMap[i] == Button )
					return i;
				break;
			case 1:
				if( CButtonMap[i] == Button )
					return i;
				break;
			case 2:
				if( PButtonMap[i] == Button )
					return i;
				break;
		}
	}
	return 0xFFFF;
}
u32 FPadSetButtonMapping( u32 Controller, u32 Button, u32 FButton )
{
	if( Controller > 3 )
		return 0;

	switch( Controller )
	{
		case 0:
			WButtonMap[FButton] = Button;
			break;
		case 1:
			CButtonMap[FButton] = Button;
			break;
		case 2:
			PButtonMap[FButton] = Button;
			break;
	}

	return 0;
}
char *FPadGetButtonName( u32 Button )
{
	switch( Button )
	{
		case OK:
			return (char*)"OK";
		case CANCEL:
			return (char*)"Cancel";
		case UP:
			return (char*)"Up";
		case DOWN:
			return (char*)"Down";
		case LEFT:
			return (char*)"Left";
		case RIGHT:
			return (char*)"Right";
		case START:
			return (char*)"Home";
		case SPECIAL_1:
			return (char*)"Play/Pause";
		default:
			return (char*)"Unmapped";
	}

	return NULL;
}
