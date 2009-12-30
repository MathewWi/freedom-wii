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
#include <wiisprite.h>
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
#include <network.h>
#include <sys/errno.h>
#include <ogc/lwp_watchdog.h>

#include <sys/types.h>
#include <sys/errno.h>
#include <fcntl.h>

//Project files
#include "processor.h"
#include "Channel.h"
#include "Identify.h"
#include "Sound.h"
#include "Music.h"
#include "Config.h"
#include "FPad.h"
#include "dol.h"
#include "NAND.h"
#include "state.h"
#include "WAD.h"
#include "SaveFile.h"

//#define DEBUG
//#define NETWORK_DEBUG
//#define CONSOLE


extern "C"
{
	#include "Patches.h"
	#include "sha1.h"
	#include "di.h"
	#include "Game.h"
	#include "misc.h"
	#include "Loader_bin.h"
	extern void _unstub_start(void);
}

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

extern char *AreaStr[];
extern char *GameRegionStr[];
extern char *VideoStr[];
extern WiimoteNames WiimoteName[];
extern u32 Playing;
extern char TagInfo[1024];
extern s32 __IOS_ShutdownSubsystems();
extern vs32 GameReady;
extern vs32 DiscError;
extern char *GameTitle;
extern u32 MusicStatus;
extern vs32 CoverGame;

extern std::vector<char*> *ChannelNames;
extern std::vector<char*> Names;

Channel *Chans;
lwp_t GameThread;
u32 Shutdown=0;
u32 Reset=0;

s32 __IOS_LoadStartupIOS(void)
{
	s32 res = __ES_Init();
	if(res < 0)
		return res;
	//res = __IOS_LaunchNewIOS(250);
	//res = __IOS_LaunchNewIOS(*(vu16*)0x80003140);	//reload current version
	//if(res < 0)
	//	return res;
	return 0;
}
void HandleWiiMoteEvent(s32 chan)
{
	Shutdown=1;
}
void HandleSTMEvent(u32 event)
{
	switch(event)
	{
		default:
		case STM_EVENT_RESET:
			Reset=1;
			break;
		case STM_EVENT_POWER:
			Shutdown=1;
			break;
	}
}
void LoadHBC( void )
{
	u64 TitleID = 0x0001000148415858LL;	// HAXX

	u32 cnt ATTRIBUTE_ALIGN(32);
	s32 r = ES_GetNumTicketViews(TitleID, &cnt);
	if( cnt == 0 || r < 0 )
	{
		TitleID = 0x000100014A4F4449LL;	// JODI
		ES_GetNumTicketViews(TitleID, &cnt);
	}

	tikview *views = (tikview *)memalign( 32, sizeof(tikview)*cnt );
	ES_GetTicketViews(TitleID, views, cnt);

	WPAD_Shutdown();

	fatUnmount("SD");

	AUDIO_StopDMA();
	AUDIO_RegisterDMACallback(NULL);
	gprintf(".");

	mtmsr(mfmsr() & ~0x8000);
	mtmsr(mfmsr() | 0x2002);

	ES_LaunchTitle(TitleID, &views[0]);	
}
int main(int argc, char **argv)
{
	CheckForGecko();
	gprintf("Freedom OS\n");
	gprintf("Built   : %s %s\n", __DATE__, __TIME__ );
	gprintf("Version : %d.%db\n", VERSION>>16, VERSION&0xFFFF );
	gprintf("Firmware: %d.%d.%d\n", *(vu16*)0x80003140, *(vu8*)0x80003142, *(vu8*)0x80003143 );

	u32 version=0;
	ES_GetBoot2Version( &version );
	gprintf("Boot2   : %d\n", version );

	VIDEO_Init();
	gprintf("System:InitVideo()\n");
	
	rmode = VIDEO_GetPreferredMode(NULL);
	xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
	
	console_init(xfb,20,20,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*VI_DISPLAY_PIX_SZ);
	
	VIDEO_Configure(rmode);
	VIDEO_SetNextFramebuffer(xfb);
	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();

	VIDEO_WaitVSync();
	if(rmode->viTVMode&VI_NON_INTERLACE)
		VIDEO_WaitVSync();

//Sound init
	AUDIO_Init (NULL);
	DSP_Init ();
	AUDIO_StopDMA();
	AUDIO_RegisterDMACallback(NULL);

	SoundInit();

	gprintf("System:Audio init done\n");

	s32 r = FPad_Init();
	gprintf("System:FPad_Init():%d\n", r);
	gprintf("BootState:%d\n", CheckBootState() );

	//Check autoboot settings
	switch( CheckBootState() )
	{
		case 3:	// Boot System Menu
			/*fallthrough*/
		default:
			ClearState();
			/*fallthrough*/
		case 0:
			break;
		case 5:
			gprintf("BootState:Shutting down!\n");
			ClearState();
			STM_ShutdownToStandby();
			break;
	}

	FSStats *fs = NANDGetStats();

	if( fs != NULL )
	{
		gprintf("NAND space stats:\tblocks\t\tmegabytes\n");
		gprintf("\tUsed\t\t%d\t\t%d\n", fs->UsedBlocks, fs->UsedBlocks*fs->BlockSize/1024/1024 );
		gprintf("\tFree\t\t%d\t\t%d\n", fs->FreeBlocks, fs->FreeBlocks*fs->BlockSize/1024/1024 );
	}

	WPAD_SetPowerButtonCallback(HandleWiiMoteEvent);
	gprintf("System:WPAD_SetPowerButtonCallback()\n");

	gprintf("System:Creating disc thread...");
	LWP_CreateThread( &GameThread, LoadGame, NULL, NULL, NULL, 5);	
	gprintf("ok\n");

	r = FPad_LoadMapping();
	gprintf("System:FPad_LoadMapping():%d\n", r);
	
	r = Config_Init();
	gprintf("System:Config_Init():%d\n", r );
	gprintf("System:Console Region:%s\n", Config_GetSettingsString("AREA") );
	gprintf("System:Console Video :%s\n", Config_GetSettingsString("VIDEO") );
	gprintf("System:Console Game  :%s\n", Config_GetSettingsString("GAME") );

	STM_RegisterEventHandler(HandleSTMEvent);
	gprintf("System:STM_RegisterEventHandler()\n");

	Chans = new Channel;

	gprintf("Channel:Caching names...");
	if( ChannelNames == NULL )
	{
		ChannelNames = new std::vector<char*>;

		for( u32 i=0; i < Chans->GetCount(); ++i)
		{
			ChannelNames->resize( ChannelNames->size() + 1 );
			char *s=Chans->GetName( i );
			if( s == NULL )
			{
				s = strdup("No Name");
			} else if( s[0] == '\0' )
			{
				s = strdup("No Name");
			}
			(*ChannelNames)[ChannelNames->size()-1] = s;
			//gprintf("Name:\"%s\"\n", (*ChannelNames)[ChannelNames->size()-1] );
		}
	}

	gprintf("ok\n");


	while(1)
	{
		FPad_Update();
 
		if ( FPad_Pressed( START ) )
		{
			gprintf("Going home ...\n");
			SoundDeInit();
			LoadHBC();
		}

		if( Shutdown )
		{
			SoundDeInit();
			gprintf("Shutdown!\n");
			*(vu32*)0xCD8000C0 &= ~0x20;

			WPAD_Shutdown();

			//if( CONF_GetShutdownMode() == CONF_SHUTDOWN_STANDBY )
			//{
				gprintf("STM_ShutdownToStandby()\n");
				STM_ShutdownToStandby();
			//} else {
				//gprintf("STM_ShutdownToIdle()\n");
				//STM_ShutdownToIdle();
			//}
		}

		if( Reset )
		{
			SoundDeInit();
			*(vu32*)0xCD8000C0 &= ~0x20;

			WPAD_Shutdown();

			gprintf("Resetting System ...\n");
			STM_RebootSystem();
		}

		VIDEO_WaitVSync();
	}

	return 0;
}
