#ifndef __BLZ_H__
#define __BLZ_H__

unsigned char *BLZ_Decode(unsigned char *pak_buffer, unsigned int pak_len, unsigned int *output_len);
unsigned char *BLZ_Encode(unsigned char *raw_buffer, unsigned int raw_len, unsigned int *output_len, int mode);

#endif