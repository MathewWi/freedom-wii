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
#ifndef _CHANNEL
#define _CHANNEL

enum Location { LOCATION_SD, LOCATION_NAND };

struct ChannelType
{
	const char *Console;
	char	id;
};

#define TYPECOUNT 14

struct ChannelSetting
{
	u64		TitleID;
	char	Name[32];
	u32		Region;
};

class Channel
{
	private:
		s32		Status;
		s32		update;
		u32		TitleCount;
		std::vector<ChannelSetting> ChannelSettingsSD;
		std::vector<ChannelSetting> ChannelSettings;

	public:
					Channel( void );

		s32			GetStatus( void );
		s32			Update( void );
		u32			LoadConfig ( void );
		u32			SaveConfig ( void );

		char		*iGetName( u32 ID );
		char		*GetName( u32 ID );
		u64			GetTitleID( u32 ID );
		u32			SetRegion( u32 ID, u32 Region );
		u32			GetRegion( u32 ID );


		s32			LaunchByIOS( u32 ID );
		s32			Launch( u32 ID );

		u32			GetCount( void );

					~Channel();
};
#endif
