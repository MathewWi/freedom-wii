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
#ifndef _SAVEFILE
#define _SAVEFILE

class SaveFile
{
	private:
		s32		Status;
		s32		update;
		u64		*TitlesIDs;
		u32		TitleCount;

	public:
					SaveFile( void );

		s32			GetStatus( void );
		s32			Update( void );
		s32			Backup( u32 ID );
		char		*GetName( u32 ID );
		void		DumpDir( char *Dst, char *Src );
		u32			GetCount( void );

					~SaveFile();
};
#endif
