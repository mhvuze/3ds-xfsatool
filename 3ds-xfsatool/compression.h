#ifndef __COMPRESSION_H__
#define __COMPRESSION_H__

char *decompress(char *input, int len, int *output_len);
char *decomp_lzss(char *input, int len, int *output_len);
char *decomp_lzx(char *input, int len, int *output_len);
char *decomp_huffman(char *input, int len, int *output_len);

#endif