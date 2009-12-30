/*	
	__fwrite patch by svpe
	hook patches by Nuke

*/
#include "Patches.h"

u32 sig_fwrite[] =
{
	0x9421FFD0,
	0x7C0802A6,
	0x90010034,
	0xBF210014,
	0x7C9B2378,
	0x7CDC3378,
	0x7C7A1B78,
	0x7CB92B78
};

u32 patch_fwrite[] = {
	0x7c8521d7, //  mullw.  r4,r5,r4
	0x40810084, //  ble-    8080cc88 <my_fwrite+0x88>
	0x3ce0cd00, //  lis     r7,-13056
	0x3d40cd00, //  lis     r10,-13056
	0x3d60cd00, //  lis     r11,-13056
	0x60e76814, //  ori     r7,r7,26644
	0x614a6824, //  ori     r10,r10,26660
	0x616b6820, //  ori     r11,r11,26656
	0x38c00000, //  li      r6,0
	0x7c0618ae, //  lbzx    r0,r6,r3
	0x5400a016, //  rlwinm  r0,r0,20,0,11
	0x6408b000, //  oris    r8,r0,45056
	0x380000d0, //  li      r0,208
	0x90070000, //  stw     r0,0(r7)
	0x7c0006ac, //  eieio
	0x910a0000, //  stw     r8,0(r10)
	0x7c0006ac, //  eieio
	0x38000019, //  li      r0,25
	0x900b0000, //  stw     r0,0(r11)
	0x7c0006ac, //  eieio
	0x800b0000, //  lwz     r0,0(r11)
	0x7c0004ac, //  sync
	0x70090001, //  andi.   r9,r0,1
	0x4082fff4, //  bne+    8080cc50 <my_fwrite+0x50>
	0x800a0000, //  lwz     r0,0(r10)
	0x7c0004ac, //  sync
	0x39200000, //  li      r9,0
	0x91270000, //  stw     r9,0(r7)
	0x7c0006ac, //  eieio
	0x74090400, //  andis.  r9,r0,1024
	0x4182ffb8, //  beq+    8080cc30 <my_fwrite+0x30>
	0x38c60001, //  addi    r6,r6,1
	0x7f862000, //  cmpw    cr7,r6,r4
	0x409effa0, //  bne+    cr7,8080cc24 <my_fwrite+0x24>
	0x7ca32b78, //  mr      r3,r5
	0x4e800020, //  blr
};

static const u32 viwiihooks[4] =
{
	0x7CE33B78,	0x38870034,
	0x38A70038,	0x38C7004C 
	//0x906402E4, 0x38000000,
	//0x900402E0, 0x909E0004
};

static const u32 kpadhooks[4] =
{
	0x9A3F005E,	0x38AE0080,
	0x389FFFFC,	0x7E0903A6
};

static const u32 kpadoldhooks[6] =
{
	0x801D0060,	0x901E0060,
	0x801D0064,	0x901E0064,
	0x801D0068,	0x901E0068
};

static const u32 joypadhooks[4] =
{
	0x3AB50001,	0x3A73000C,
	0x2C150004,	0x3B18000C
};

PatchInfo PI[] =
{
	{ sig_fwrite, sizeof(sig_fwrite), patch_fwrite, sizeof(patch_fwrite) },
	{ viwiihooks, sizeof(viwiihooks), NULL, 0 },
	{ kpadhooks, sizeof(kpadhooks), NULL, 0 },
	{ kpadoldhooks, sizeof(kpadoldhooks), NULL, 0 },
	{ joypadhooks, sizeof(joypadhooks), NULL, 0 },

	//{ sig_err001, sizeof(sig_err001), patch_err001, sizeof(patch_err001) },
	//{ sig_err002, sizeof(sig_err002), patch_err002, sizeof(patch_err002) }
};

u32 FindPattern( void *Data, u32 Length, u32 Pattern )
{
	if( Pattern >= PATTERN_COUNT )
		return 0;

	u32 i;
	for( i=0; i < Length; ++i)
	{
		if( memcmp( Data+i, PI[Pattern].Pattern, PI[Pattern].PatternLength) == 0 )
			return i;
	}

	return 0;
}
u32 ApplyPatch( void *Data, u32 Length, u32 Pattern, u32 Patch )
{
	if( Pattern >= PATTERN_COUNT || Patch >= PATCH_COUNT )
		return 0;

	u32 i;
	for( i=0; i<Length; ++i)
	{
		if( memcmp( (u8*)(Data)+i, PI[Pattern].Pattern, PI[Pattern].PatternLength) == 0 )
		{
			memcpy( (u8*)(Data)+i, PI[Patch].Patch, PI[Patch].PatchLength );
			DCFlushRange( (u8*)(Data)+i, PI[Patch].PatchLength );
			return 1;
		}
	}

	return 0;
}
