#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "compression.h"
#include "cuecomp/cuecomp.h"

char *decompress(char *input, int len, int *output_len)
{
	if((unsigned char)input[4] == 0x0f || (unsigned char)input[4] == 0xff)
		return decomp_huffman(input, len, output_len);
	else
		return decomp_lzss(input, len, output_len);
}

char *decomp_lzss(char *input, int len, int *output_len)
{
	int size = *(unsigned int*)input / 8;
	
	if(len - 4 == size)
	{
		// Not compressed, return data with header chopped off.
		// Create a copy of the data instead of just returning the original pointer so
		// we don't run into a bug later on when freeing memory.
		char *output = (char*)calloc(size, sizeof(char));
		memcpy(output, input + 4, size);
		*output_len = size;
		return output;
	}

	*(unsigned int*)input = (size << 8) | 0x10;
	return (char*)LZSS_Decode((unsigned char*)input, len, (unsigned int*)output_len);
}

char *decomp_huffman(char *input, int len, int *output_len)
{
	int size = *(unsigned int*)input / 8;
	
	if(len - 4 == size)
	{
		char *output = (char*)calloc(size, sizeof(char));
		memcpy(output, input + 4, size);
		*output_len = size;
		return output;
	}

	unsigned char type = input[4] != 0x0f ? 0x28 : 0x24; // 8-bit or 4-bit huffman
	*(unsigned int*)input = (size << 8) | type;
	return (char*)HUF_Decode((unsigned char*)input, len, (unsigned int*)output_len);
}
