#include <cstdio>
#include <cstring>
#include <vector>
#include <map>
#include "utils.h"
#include "compression.h"
#include "arc.h"

typedef struct _ArchiveHeaderArc
{
	unsigned char magic[4];
	int table1Offset; // Folder information?
	int table2Offset; // ?
	int table3Offset; // File information
	int filenameTableOffset; // Filename list
	int dataOffset; // Actual file data
} ArchiveHeaderArc;

typedef struct _FileEntryArc
{
	unsigned int crc32;
	int unknown;
	int offset;
	int filesize;
} FileEntryArc;

typedef struct _DirectoryEntryArc
{
	unsigned int crc32;
	short unknown1;
	short unknown2;
	short fileCount;
	short unknown3;
	int filenameOffset;
	int foldernameOffset;
} DirectoryEntryArc;

static void ParseFilenames(FILE *infile, ArchiveHeaderArc header);
static void ParseFileEntries(FILE *infile, ArchiveHeaderArc header);
static void ParseDirectoryEntries(FILE *infile, ArchiveHeaderArc header);

static std::vector<std::string> filenames;
static std::vector<std::string> foldernames;
static std::map<unsigned int, std::string> filename_mapping;	
static std::map<unsigned int, int> filename_index;
static std::map<unsigned int, int> file_index;
static std::vector<FileEntryArc> file_entries;
static std::vector<DirectoryEntryArc> directory_entries;

void ParseArc(FILE *infile, bool quietMode)
{
	ArchiveHeaderArc header = { 0 };
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
	//ParseDirectoryEntries(infile, header);

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

void ParseFilenames(FILE *infile, ArchiveHeaderArc header)
{
	int bufferSize = header.dataOffset - header.filenameTableOffset;
	char *buffer = (char*)calloc(bufferSize, sizeof(char));

	fseek(infile,header.filenameTableOffset,SEEK_SET);
	fread(buffer, 1, bufferSize, infile);

	int output_len = 0;
	char *decomp = decompress(buffer, bufferSize, &output_len);

	bool isPath = false;
	int idx = 0;
	for(int i = 0, startOffset = 0; i < output_len; i++)
	{
		if(decomp[i] == '\0')
		{
			if(startOffset != 0)
			{
				filename_index[startOffset] = filenames.size();

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

void ParseFileEntries(FILE *infile, ArchiveHeaderArc header)
{
	int bufferSize = header.filenameTableOffset - header.table3Offset;
	char *buffer = (char*)calloc(bufferSize, sizeof(char));

	fseek(infile,header.table3Offset,SEEK_SET);
	fread(buffer, 1, bufferSize, infile);
	
	int output_len = 0;
	unsigned int *decomp = (unsigned int*)decompress(buffer, bufferSize, &output_len);

	for(int i = 0; i < output_len / 4; i += 4)
	{
		FileEntryArc entry = { 0 };

		entry.crc32 = decomp[i];
		entry.unknown = decomp[i+1];
		entry.offset = decomp[i+2];
		entry.filesize = decomp[i+3];

		//printf("%08x %08x %08x %08x\n",entry.crc32,entry.unknown,entry.offset,entry.filesize);

		file_index[entry.crc32] = file_entries.size();
		file_entries.push_back(entry);
	}

	free(buffer);
	free(decomp);
}

void ParseDirectoryEntries(FILE *infile, ArchiveHeaderArc header)
{
	int bufferSize = header.table2Offset - header.table1Offset;
	char *buffer = (char*)calloc(bufferSize, sizeof(char));

	fseek(infile,header.table1Offset,SEEK_SET);
	fread(buffer, 1, bufferSize, infile);

	int output_len = 0;
	unsigned int *decomp = (unsigned int*)decompress(buffer, bufferSize, &output_len);

	for(int i = 0; i < output_len / 16; i += 5)
	{
		DirectoryEntryArc entry = { 0 };

		entry.crc32 = decomp[i];
		entry.unknown1 = decomp[i+1] >> 16;
		entry.unknown2 = decomp[i+1] & 0xffff;
		entry.fileCount = decomp[i+2] >> 16;
		entry.unknown3 = decomp[i+2] & 0xffff;
		entry.filenameOffset = decomp[i+3];
		entry.foldernameOffset = decomp[i+4];
		
		directory_entries.push_back(entry);

		//printf("%08x %04x %04x %08x %08x %08x\n",entry.crc32,entry.unknown1,entry.unknown2,entry.unknown3,entry.filenameOffset,entry.foldernameOffset);
	}

	free(buffer);
	free(decomp);
}
