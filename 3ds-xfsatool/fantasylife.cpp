#include <cstdio>
#include <cstring>
#include <vector>
#include "utils.h"
#include "fantasylife.h"
#include "compression.h"

typedef struct _ArchiveHeaderFantasyLife
{
	int fileCount;
	int pad;
} ArchiveHeaderFantasyLife;

typedef struct _FileEntryFantasyLife
{
	int fileOffset;
	int compFilesize;
	int decompFilesize;
	bool compression;
} FileEntryFantasyLife;

inline bool ISASCII(char c)
{
	// Roll my own isascii check to get around some bug I was experiencing... too lazy to figure out the proper way
	if((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))
		return true;

	return false;
}

bool ParseFantasyLife(FILE *infile, bool quietMode)
{
	// TODO: Come up with a good way to detect that the archive is a valid Fantasy Life archive.
	ArchiveHeaderFantasyLife header = { 0 };
	int archiveSize = 0;

	fseek(infile,0,SEEK_END);
	archiveSize = ftell(infile);
	rewind(infile);
	
	fread(&header.fileCount, 1, 4, infile);
	fread(&header.pad, 1, 4, infile);

	std::vector<FileEntryFantasyLife> fileEntries;
	
	// Get file offsets
	for(int i = 0; i < header.fileCount; i++)
	{
		FileEntryFantasyLife entry = { 0 };
		fread(&entry.fileOffset, 1, 4, infile);
		fileEntries.push_back(entry);
	}

	for(int i = 0; i < header.fileCount; i++)
	{
		fread(&fileEntries[i].decompFilesize, 1, 4, infile);
	}

	for(int i = 0; i < header.fileCount; i++)
	{
		fread(&fileEntries[i].compFilesize, 1, 4, infile);
	}

	for(int i = 0; i < header.fileCount; i++)
	{
		fread(&fileEntries[i].compression, 1, 1, infile);
	}
	
	static char outputname[20] = { 0 };
	for(int i = 0; i < header.fileCount; i++)
	{
		char *buffer = (char*)calloc(fileEntries[i].compFilesize, sizeof(char));
		int output_len = 0;
		
		fseek(infile, fileEntries[i].fileOffset, SEEK_SET);
		fread(buffer, 1, fileEntries[i].compFilesize, infile);
		
		/*
		fileEntries[i].compression = false;
		fileEntries[i].decompFilesize = fileEntries[i].compFilesize;
		*/

		if(fileEntries[i].compression)
		{
			char *decomp = decomp_lzx(buffer, fileEntries[i].compFilesize, &output_len);

			if(output_len != fileEntries[i].decompFilesize)
			{
				printf("WARNING: Possibly bad decompression detected on file %d... got %08x bytes, expected %08x bytes\n", i, output_len, fileEntries[i].decompFilesize);
			}

			free(buffer);
			buffer = decomp;
		}
		else
		{
			output_len = fileEntries[i].decompFilesize;
		}

		// Detect filetype if possible
		char ext[5] = "bin";
		if(output_len >= 4 && memcmp(buffer, "zmdl", 4) == 0)
		{
			strcpy(ext, "mdl\0");
		}
		else if(output_len >= 4 && memcmp(buffer, "zmta", 4) == 0)
		{
			strcpy(ext, "mta\0");
		}
		else if(output_len >= 4 && memcmp(buffer, "ztex", 4) == 0)
		{
			strcpy(ext, "tex\0");
		}
		else if(output_len >= 4 && memcmp(buffer, "BPAK", 4) == 0)
		{
			strcpy(ext, "pak\0");
		}
		else if(output_len >= 4 && memcmp(buffer, "CB01", 4) == 0)
		{
			strcpy(ext, "cb\0");
		}
		else if(output_len >= 4 && memcmp(buffer, "GB01", 4) == 0)
		{
			strcpy(ext, "gb\0");
		}
		else if(output_len >= 4 && memcmp(buffer, "FB01", 4) == 0)
		{
			strcpy(ext, "fb\0");
		}
		else if(output_len >= 3 && memcmp(buffer, "COL", 3) == 0)
		{
			strcpy(ext, "col\0");
		}
		else if(output_len >= 4 && memcmp(buffer, "CSAR", 4) == 0)
		{
			strcpy(ext, "csar\0");
		}
		else if(output_len >= 4 && memcmp(buffer, "MOTA", 4) == 0)
		{
			strcpy(ext, "mota\0");
		}
		else if(output_len >= 8 && memcmp(buffer, "\x02\x00\x00\x00\x10\x00\x00\x00", 8) == 0)
		{
			strcpy(ext, "scr\0");
		}
		else if(output_len >= 8 && memcmp(buffer, "\x13\x80\x03\x1D\x00\x00\x00\x00", 8) == 0)
		{
			strcpy(ext, "scr\0");
		}
		else if(output_len >= 4 && ISASCII(buffer[0]) && ISASCII(buffer[1]) && ISASCII(buffer[2]) && ISASCII(buffer[3]))
		{
			strcpy(ext, "bin\0");
			//printf("Header: %c%c%c%c\n",buffer[0],buffer[1],buffer[2],buffer[3]);
			//break;
		}

		sprintf(outputname, "%08d.%s\0", i, ext);

		if(!quietMode)
		{
			printf("[%05d] %s offset[%08x] compSize[%08x] decompSize[%08x]\n", i, outputname, fileEntries[i].fileOffset, fileEntries[i].compFilesize, fileEntries[i].decompFilesize, fileEntries[i].compression);
		}

		FILE *outfile = fopen(outputname, "wb");		
		fwrite(buffer, 1, output_len, outfile);
		fclose(outfile);
		free(buffer);
	}

	fclose(infile);

	return true;
}