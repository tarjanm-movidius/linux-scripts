#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>

#define DEBUGPRINTF(...)	fprintf(stderr, __VA_ARGS__)
#define LOGPRINTF(...)	fprintf(cutScript, __VA_ARGS__)
//#define LOGPRINTF		printf
#define BUFLEN			512

char ts1[10], ts2[10], cutcmd[128], cutNumbers=0;
FILE *cutScript;

int findTS(char *buf, int len, char* ts)
{
int i, firstNum = -1;

	for (i=0; i<len && buf[i]; i++)
	{
		if (buf[i] == ':' && firstNum >= 0)
		{
			if (isdigit(buf[i+1]))
			{
				for (i+=2; i<len && buf[i]; i++) if (!isdigit(buf[i]) && buf[i] != ':') break;
				if (ts)
				{
					strncpy (ts, buf + firstNum, i - firstNum);
					ts[i-firstNum] = 0;
				}
				return i;
			} else {
				i++;
				firstNum = -1;
			}
		} else if (isdigit(buf[i])) {
			if (firstNum < 0) firstNum = i;
		} else {
			firstNum = -1;
		}
	}
	if (ts) ts[0] = 0;
	return 0;
}

void parseLine(char *buf)
{

static unsigned int lineNo = 1;
int i=0, t1ofs, itmp=0;

	// Getting rid of track numbers
	if (cutNumbers) for (; i<BUFLEN && buf[i]; i++) if (!isdigit(buf[i])) break;
	// Getting rid of garbage at the beginning of the line
	for (; i<BUFLEN && buf[i]; i++) if (buf[i] != ' ' && buf[i] != '\t' && buf[i] != '-' && buf[i] != '.') break;
	if(buf[i] == 0 || buf[i] == '\n' ) return;

	// Finding timestamp(s)
	t1ofs = findTS(buf+i, BUFLEN-i, ts1);
	if (t1ofs)
	{
		// We've found a timestamp, printing as end of prev. song if no ts2
		if(ts2[0] == 0 && lineNo > 1) LOGPRINTF("%s\n", ts1);

		// If timestamp was at the beginning, skipping it
		if (strlen(ts1) == t1ofs)
		{
			i += t1ofs;
			t1ofs = 0;
			// Getting rid of garbage after ts1
			for (; i<BUFLEN && buf[i]; i++) if (buf[i] != ' ' && buf[i] != '\t' && buf[i] != '-' && buf[i] != '.') break;
		}

		// Searching for a second one, will zero ts2 if not found
		itmp = findTS(buf+i+t1ofs, BUFLEN-i-t1ofs, ts2);

		// If timestamp was at the beginning, skipping it
		if (!t1ofs && itmp && (strlen(ts2) == itmp))
		{
			i += itmp;
			itmp = 0;
			// Getting rid of garbage after ts2
			for (; i<BUFLEN && buf[i]; i++) if (buf[i] != ' ' && buf[i] != '\t' && buf[i] != '-' && buf[i] != '.') break;
		}
	} else {
		// Timestamp not found
		ts2[0] = 0;
		if (lineNo == 1) strcpy(ts1, "0:00");
		else ts1[0] = 0;
	}

	// Finding track title. We're on the first character, if line begins with timestamps they are skipped
	if (t1ofs)
	{
		// t1ofs points to after the timestamp, searching back to its beginning
		for (itmp = i+t1ofs-1; itmp>i; itmp--) if (!isdigit(buf[itmp]) && buf[itmp] != ':' && buf[itmp] != '(' && buf[itmp] != '[') break;
		buf[itmp+1] = 0;
	}

	// Searching for the end of the string
	for (itmp = i; itmp<BUFLEN && buf[itmp] && buf[itmp] != '\n'; itmp++) continue;
	if (itmp >= BUFLEN) itmp = BUFLEN - 1;
	// Getting rid of garbage at the end of the line
	for (itmp--; itmp>i; itmp--) if (isprint(buf[itmp]) && (buf[itmp] != ' ' && buf[itmp] != '\t' && buf[itmp] != '-')) break;
	if(itmp == i) return;
	buf[itmp+1] = 0;

	LOGPRINTF("%s", cutcmd);
	if (lineNo < 10) LOGPRINTF("0");
	LOGPRINTF("%u - %s.mp3\" -a %s -b ", lineNo, buf+i, ts1);
	if (ts2[0]) LOGPRINTF("%s\n", ts2);
	lineNo++;
}

int main(int argc, char* argv[])
{

FILE *trkListFile;
char buf[BUFLEN];
int i;

	if(argc < 3) { DEBUGPRINTF( "Error: parameter missing\n\nUsage: %s <tracklist.txt> <album.mp3> [-]\n", argv[0]); return 2; }

	// Opening tracklist
	trkListFile = fopen(argv[1], "rb");
	if (!trkListFile) { perror("fopen"); DEBUGPRINTF("Error opening file \"%s\"\n", argv[1]); return 3; }
	sprintf(cutcmd, "cutmp3 -i \"%s\" -O \"$TDIR/", argv[2]);

	strcpy(buf, argv[2]);
	for (i = strlen(buf); i && (buf[i] != '.'); i--) continue;

	// Opening output file
	if (argc > 3 && argv[3][0] == '-' && argv[3][1] == 0)
	{
		cutScript = stdout;
	} else {
		if (buf[i] == '.') buf[i] = 0;
		strcat (buf, ".sh");
		cutScript = fopen(buf, "w");
		if (!cutScript) { perror("fopen"); DEBUGPRINTF("Error opening output file \"%s\"\n", buf); return 3; }
		// strtol("0755", 0, 8) == 0x1ED
		if (chmod (buf, 0x1ED) < 0) { perror("chmod"); DEBUGPRINTF("Error changing output file mode\n"); return 3; }
	}

	// Script header
	if (buf[i] == '.') buf[i] = 0;
	LOGPRINTF("#!/bin/sh\n\nTDIR=\"%s\"\nmkdir -p \"$TDIR\"\n\n", buf);

	// Checking tracklist for numbers at the beginning of the line
	while(!feof(trkListFile))
	{
		if (fgets(buf, BUFLEN, trkListFile))
		{
			for (i = 0; i < BUFLEN && buf[i]; i++) if (!isdigit(buf[i])) break;
			if ((i == 0 || buf[i] == ':') && buf[i] != 0 && buf[i] != '\n') break;
		}
	}
	if (feof(trkListFile)) cutNumbers = 1;
	rewind(trkListFile);

	// Reading tracklist
	while(!feof(trkListFile))
	{
		if (fgets(buf, BUFLEN, trkListFile)) parseLine(buf);
	}
	if (!ts2[0]) LOGPRINTF("999:99\n");

	fclose(trkListFile);
	fclose(cutScript);
	return 0;
}
