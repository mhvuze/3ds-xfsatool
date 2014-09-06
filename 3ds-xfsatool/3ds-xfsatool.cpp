// 3ds-xfsatool.cpp : Defines the entry point for the console application.
//

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#ifdef _WIN32
#include <direct.h>
#endif

#include "utils.h"
#include "arc.h"
#include "guild.h"
#include "xfsa.h"

void ParseArchive(char *filename, char *outputFolder, bool quietMode);

void DisplayHelp(char *exename)
{
	printf("usage: %s (flags)\nFlags:\n"\
		"-i - Input file\n-o Output folder\n"\
		"-q Quiet mode (faster)\n\n"\
		"Example:\n"\
		"%s -i file.fa -o output-file\n"\
		"%s -i file2.fa -o output-file2 -q\n\n", exename, exename, exename);
	exit(0);
}

int main(int argc, char **argv)
{
	char *filename = NULL;
	char *outputFolder = NULL;
	bool quietMode = false;

	for(int i = 0; i < argc; i++)
	{
		if(argv[i][0] == '-' && strlen(argv[i]) > 1)
		{
			switch(argv[i][1])
			{
			case 'i':
				{
					if(i + 1 < argc)
					{
						filename = argv[i+1];
						i++;
					}
				}
				break;
			case 'o':
				{
					if(i + 1 < argc)
					{
						outputFolder = argv[i+1];
						i++;
					}
				}
				break;
			case 'q':
				{
					quietMode = true;
				}
				break;
			}
		}
		else
		{
		}
	}

	if(filename == NULL)
	{
		DisplayHelp(argv[0]);
	}

	if(outputFolder == NULL)
	{
		outputFolder = "output";
	}

	clock_t start = clock();

	ParseArchive(filename, outputFolder, quietMode);

	clock_t end = clock();
	printf("Completed in %f seconds\n", (double)(end - start) / CLOCKS_PER_SEC);
}

void ParseArchive(char *filename, char *outputFolder, bool quietMode)
{
	FILE *infile = fopen(filename, "rb");
	char magic[4] = { 0 };

	if(!infile)
	{
		printf("Could not open %s\n", filename);
		exit(-1);
	}

	fread(magic, 1, 4, infile);
	rewind(infile);

	CreateFullPath(outputFolder, true);
	chdir(outputFolder);

	if(memcmp(magic, "XFSA", 4) == 0) // XFSA
	{
		ParseXfsa(infile, quietMode);
	}
	else if(memcmp(magic, "ARC0", 4) == 0) // ARC0
	{
		ParseArc(infile, quietMode);
	}
	else
	{
		ParseGuild(infile, quietMode);

		//printf("Not a valid XFSA or ARC0 archive.\n");
		//exit(-2);
	}

	fclose(infile);
}