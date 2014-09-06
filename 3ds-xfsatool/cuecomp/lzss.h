#ifndef __LZSS_H__
#define __LZSS_H__

unsigned char *LZSS_Decode(unsigned char *pak_buffer, unsigned int pak_len, unsigned int *output_len);
unsigned char *LZSS_Encode(unsigned char *raw_buffer, unsigned int raw_len, unsigned int *output_len, int mode);

#endif