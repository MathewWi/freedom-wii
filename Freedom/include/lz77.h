#ifndef _LZ77_MODULE
#define _LZ77_MODULE

#define LZ77_0x10_FLAG 	0x10
#define LZ77_0x11_FLAG 	0x11

int isLZ77compressed(u8 *buffer);
u32 __GetDecompressSize( u8 *in, u8 type );
void __decompressLZ77_10(u8 *in, u32 inputLen, u8 **output);
void __decompressLZ77_11(u8 *in, u32 inputLen, u8 **output);

#endif

