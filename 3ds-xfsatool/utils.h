#ifndef __UTILS_H__
#define __UTILS_H__

#include <string>

void CreateFullPath(std::string path, bool forceCreation = false);
void CreateFullPath(char *path, bool forceCreation = false);
unsigned int CalculateCrc32(char *buffer, int length);
unsigned int CalculateCrc32(unsigned char *buffer, int length);

#endif