#ifndef __RLE_H__
#define __RLE_H__

unsigned char *RLE_Decode(unsigned char *pak_buffer, unsigned int pak_len, unsigned int *output_len);
unsigned char *RLE_Encode(unsigned char *raw_buffer, unsigned int raw_len, unsigned int *output_len);

#endif