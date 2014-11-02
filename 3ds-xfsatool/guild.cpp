#include <cstdio>
#include <cstring>
#include <vector>
#include "utils.h"
#include "guild.h"

typedef struct _ArchiveHeaderGuild
{
	int fileCount;
	int archiveSize;
	char pad[8];
} ArchiveHeaderGuild;

typedef struct _FileEntryGuild
{
	int fileOffset;
	int filesize;
	char pad[8];
	char filename[64];
} FileEntryGuild;


bool ParseGuild(FILE *infile, bool quietMode)
{
	ArchiveHeaderGuild header = { 0 };
	int archiveSize = 0;

	fseek(infile,0,SEEK_END);
	archiveSize = ftell(infile);
	rewind(infile);
	
	fread(&header.fileCount, 1, 4, infile);
	fread(&header.archiveSize, 1, 4, infile);

	if(header.archiveSize != archiveSize)
		return false;

	fread(header.pad, 1, 8, infile);

	if(archiveSize != header.archiveSize)
	{
		printf("Most likely not a valid archive. Attempting to parse anyway.\n");
	}

	char *buffer = NULL;
	int bufferSize = 0;
	for(int i = 0; i < header.fileCount; i++)
	{
		FileEntryGuild entry = { 0 };
		
		fread(&entry.fileOffset, 1, 4, infile);
		fread(&entry.filesize, 1, 4, infile);
		fread(entry.pad, 1, 8, infile);
		fread(entry.filename, 1, 64, infile);

		if(!quietMode)
		{
			printf("[%06d] %-38s size[%08x] offset[%08x]\n", i, entry.filename, entry.filesize, entry.fileOffset);
		}
		
		CreateFullPath(entry.filename);

		// Save current offset in header before reading data
		size_t curOffset = ftell(infile);
		
		if(entry.fileOffset > archiveSize)
		{
			//printf("Couldn't seek passed end of file\n");
			continue;
		}

		fseek(infile,entry.fileOffset,SEEK_SET);

		if(entry.filesize > bufferSize)
		{
			bufferSize = entry.filesize * 2;
			if(buffer == NULL)
			{
				buffer = (char*)malloc(bufferSize);
			}
			else
			{
				buffer = (char*)realloc(buffer, bufferSize);
			}
		}

		fread(buffer, 1, entry.filesize, infile);

		FILE *outfile = fopen(entry.filename, "wb");
		if(!outfile)
		{
			printf("ERROR: Could not open %s\n", entry.filename);
		}
		else
		{
			fwrite(buffer, 1, entry.filesize, outfile);
			fclose(outfile);
		}

		fseek(infile,curOffset,SEEK_SET);
	}

	if(buffer != NULL)
	{
		free(buffer);
	}

	return true;
}