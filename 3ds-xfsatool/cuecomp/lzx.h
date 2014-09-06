#ifndef __LZX_H__
#define __LZX_H__

unsigned char *LZX_Decode(unsigned char *pak_buffer, unsigned int pak_len, unsigned int *output_len);
unsigned char *LZX_Encode(unsigned char *raw_buffer, unsigned int raw_len, unsigned int *output_len, int cmd, int vram);

#endif