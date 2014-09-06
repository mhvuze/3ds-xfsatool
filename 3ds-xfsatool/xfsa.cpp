#include <cstdio>
#include <cstring>
#include <vector>
#include <map>
#include "utils.h"
#include "compression.h"
#include "arc.h"

typedef struct _ArchiveHeaderXfsa
{
	unsigned char magic[4];
	int table1Offset; // Folder information?
	int table2Offset; // ?
	int table3Offset; // File information
	int filenameTableOffset; // Filename list
	int dataOffset; // Actual file data
} ArchiveHeaderXfsa;

typedef struct _FileEntryXfsa
{
	unsigned int crc32;
	int unknown1;
	int offset;
	int unknown2;
	int filesize;
} FileEntryXfsa;

static void ParseFilenames(FILE *infile, ArchiveHeaderXfsa header);
static void ParseFileEntries(FILE *infile, ArchiveHeaderXfsa header);

static std::vector<std::string> filenames;
static std::vector<std::string> foldernames;
static std::map<unsigned int, std::string> filename_mapping;
static std::map<unsigned int, int> file_index;
static std::vector<FileEntryXfsa> file_entries;

void ParseXfsa(FILE *infile, bool quietMode)
{
	ArchiveHeaderXfsa header = { 0 };
	int archiveSize = 0;

	fseek(infile,0,SEEK_END);
	archiveSize = ftell(infile);
	rewind(infile);

	fread(header.magic, 1, 4, infile);
	fread(&header.table1Offset, 1, 4, infile);
	fread(&header.table2Offset, 1, 4, infile);
	fread(&header.table3Offset, 1, 4, infile);
	fread(&header.filenameTableOffset, 1, 4, infile);
	fread(&header.dataOffset, 1, 4, infile);

	ParseFilenames(infile, header);
	ParseFileEntries(infile, header);

	// Create all of the possible directory paths in one go instead of trying to create 
	// each full path during the file dumping loop
	for(int i = 0; i < foldernames.size(); i++)
	{
		CreateFullPath(foldernames[i]);
	}

	std::string currentFolder = "";
	int dumped = 0;
	char *buffer = NULL;
	int bufferSize = 0;
	for(int i = 0; i < filenames.size(); i++)
	{
		if(filenames[i][filenames[i].size() - 1] == '/')
		{
			currentFolder = filenames[i];
		}
		else
		{
			std::string fullpath = currentFolder + filenames[i];
			unsigned int crc32 = CalculateCrc32((char*)filenames[i].c_str(), filenames[i].size());

			if(file_index.find(crc32) != file_index.end())
			{
				int idx = file_index[crc32];

				if(!quietMode)
				{
					printf("[%06d] %-38s size[%08x] offset[%08x]\n", dumped, fullpath.c_str(), file_entries[idx].filesize, file_entries[idx].offset);
				}

				dumped++;

				if(file_entries[idx].filesize > bufferSize)
				{
					bufferSize = file_entries[idx].filesize * 2;

					if(buffer == NULL)					
					{
						buffer = (char*)calloc(bufferSize, sizeof(char));
					}
					else
					{
						buffer = (char*)realloc(buffer, bufferSize);
					}
				}

				if(header.dataOffset + file_entries[idx].offset > archiveSize)
				{
					//printf("Couldn't seek passed end of file\n");
					continue;
				}

				fseek(infile,header.dataOffset + file_entries[idx].offset,SEEK_SET);
				fread(buffer, 1, file_entries[idx].filesize, infile);

				FILE *outfile = fopen(fullpath.c_str(), "wb");

				if(!outfile)
				{
					printf("Could not open %s\n", fullpath.c_str());
					exit(-1);
				}

				fwrite(buffer, 1, file_entries[idx].filesize, outfile);
				fclose(outfile);
			}
			else
			{
				printf("Could not find crc32 %08x: %s\n",crc32, filenames[i].c_str());
				exit(1);
			}
		}
	}

	if(buffer != NULL)
		free(buffer);
}

void ParseFilenames(FILE *infile, ArchiveHeaderXfsa header)
{
	int bufferSize = header.dataOffset - header.filenameTableOffset;
	char *buffer = (char*)calloc(bufferSize, sizeof(char));

	fseek(infile,header.filenameTableOffset,SEEK_SET);
	fread(buffer, 1, bufferSize, infile);

	int output_len = 0;
	char *decomp = NULL;
	
	decomp = decompress(buffer, bufferSize, &output_len);

	/*
	FILE *outfile = fopen("output2.bin","wb");
	fwrite(decomp, 1, output_len, outfile);
	fclose(outfile);
	*/

	bool isPath = false;
	int idx = 0;
	for(int i = 0, startOffset = 0; i < output_len; i++)
	{
		if(decomp[i] == '\0')
		{
			if(startOffset != 0)
			{
				std::string filename(decomp + startOffset);

				unsigned int crc = CalculateCrc32((char*)filename.c_str(), filename.size());
				filename_mapping[crc] = filename;

				//printf("%08x %s\n",crc, filename.c_str());

				filenames.push_back(filename);

				if(isPath)
					foldernames.push_back(filename);
			}

			startOffset = i + 1;
			isPath = false;
		}

		if(decomp[i] == '/')
			isPath = true;
	}

	free(buffer);
	free(decomp);
}

void ParseFileEntries(FILE *infile, ArchiveHeaderXfsa header)
{
	int bufferSize = header.filenameTableOffset - header.table3Offset;
	char *buffer = (char*)calloc(bufferSize, sizeof(char));

	fseek(infile,header.table3Offset,SEEK_SET);
	fread(buffer, 1, bufferSize, infile);
	
	int output_len = 0;
	unsigned int *decomp = NULL;
	
	bool flipBits = buffer[4] == 0x0f;
	decomp = (unsigned int*)decompress(buffer, bufferSize, &output_len);

	if(flipBits)
	{
		unsigned char *d = (unsigned char*)decomp;
		for(int i = 0; i < output_len; i++)
		{
			d[i] = (d[i] << 4) | (d[i] >> 4);
		}
	}

	/*
	FILE *outfile = fopen("output.bin","wb");
	fwrite(decomp, 1, output_len, outfile);
	fclose(outfile);
	*/

	for(int i = 0; i < output_len / 4; i += 3)
	{
		FileEntryXfsa entry = { 0 };

		entry.crc32 = decomp[i];
		//entry.unknown1 = decomp[i+1] >> 24;
		entry.offset = (decomp[i+1] & 0xffffff) << 4;
		//entry.unknown2 = decomp[i+2] >> 24;
		entry.filesize = decomp[i+2] & 0x7fffff; // there's something weird going on here but i'm not sure what exactly
		
		//printf("%08x %02x %07x %02x %06x %s\n", entry.crc32, entry.unknown1, entry.offset, entry.unknown2, entry.filesize, filename_mapping[entry.crc32].c_str());

		file_index[entry.crc32] = file_entries.size();
		file_entries.push_back(entry);
	}

	//printf("%d %d\n", file_entries.size(), filenames.size() - foldernames.size());

	free(buffer);
	free(decomp);
}
