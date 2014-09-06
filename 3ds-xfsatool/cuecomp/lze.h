#ifndef __LZE_H__
#define __LZE_H__

unsigned char *LZE_Decode(unsigned char *pak_buffer, unsigned int pak_len, unsigned int *output_len);
unsigned char *LZE_Encode(unsigned char *raw_buffer, unsigned int raw_len, unsigned int *output_len);

#endif