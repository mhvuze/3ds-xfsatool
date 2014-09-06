#ifndef __HUFFMAN_H__
#define __HUFFMAN_H__

unsigned char *HUF_Decode(unsigned char *pak_buffer, unsigned int pak_len, unsigned int *output_len);
unsigned char *HUF_Encode(unsigned char *raw_buffer, unsigned int raw_len, unsigned int *output_len, int cmd);

#endif