#if 0 /*
##### Run this file in shell to compile (e.g. sh genalbum.c) #####

COMPILE="gcc -Wall -O2 -s -o genalbum genalbum.c"

echo "$COMPILE" && $COMPILE
exit
*/
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>

#define DEBUGPRINTF(...)	fprintf(stderr, __VA_ARGS__)
#define OUTPRINTF(...)	fprintf(cutScript, __VA_ARGS__)
#define OUTPUTC(ch_)	putc((ch_), cutScript)
#define BUFLEN			65536
#define CUTCMD			"\ncutmp3 -i \"$ALBUM\" -O \"$TDIR/"

#define F_CUTNUMBERS	1
#define F_ISMP3		2

char ts2[10], flags=0;
FILE *cutScript;

// Find timestamp in buf[] and copy it to ts[]
int findTS (char *buf, char* ts)
{

int i, firstNum = -1;
unsigned short h=0, m=0;

	for (i=0; buf[i]; i++)
	{
		if (buf[i] == ':' && firstNum >= 0)
		{
			if (isdigit(buf[i+1]))
			{
				for (i+=2; isdigit(buf[i]) || buf[i] == ':'; i++) if (buf[i] == ':') h++;
				if (ts)
				{
					if (h)
					{
						// Converting hours to minutes for cutmp3
						for (h=0, i=firstNum; isdigit(buf[i]); i++) h = h * 10 + buf[i] - '0';
						for (i++; isdigit(buf[i]); i++) m = m * 10 + buf[i] - '0';
						m += h * 60;
						sprintf (ts, "%hu:", m);
						for (h = strlen(ts), i++; isdigit(buf[i]); h++, i++) ts[h] = buf[i];
					} else {
						strncpy (ts, buf + firstNum, i - firstNum);
					}
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

}	// findTS()


void parseLine (char *buf)
{

static unsigned int lineNo = 1;
static char ts1[10];
int i=0, j, t1ofs;

	// Getting rid of track numbers
	if (flags & F_CUTNUMBERS)
	{
		while (isdigit(buf[i]) || buf[i] == ' ' || buf[i] == '\t') i++;
		if (buf[i] == ':')
		{
			if (isdigit(buf[i+1])) i = 0;
			  else i++;
		}
	}
	// Getting rid of garbage at the beginning of the line
	while (buf[i] == ' ' || buf[i] == '\t' || buf[i] == '-' || buf[i] == '.' || buf[i] == ')' || ((buf[i] == '(' || buf[i] == '[') && isdigit(buf[i+1]))) i++;
	if (buf[i] == 0 || buf[i] == '\n') return;

	// Finding timestamp(s)
	t1ofs = findTS(buf+i, ts1);
	if (t1ofs)
	{
		// We've found a timestamp, printing as end of prev. song if no ts2
		if(ts2[0] == 0 && lineNo > 1) OUTPRINTF("%s", ts1);

		// If timestamp was at the beginning, skipping it
		if (strlen(ts1) >= t1ofs-2)
		{
			i += t1ofs;
			t1ofs = 0;
			// Getting rid of garbage after ts1
			while (buf[i] == ' ' || buf[i] == '\t' || buf[i] == '-' || buf[i] == '.' || buf[i] == ')' || buf[i] == ']' || ((buf[i] == '(' || buf[i] == '[') && isdigit(buf[i+1]))) i++;
		}

		// Searching for a second one, will zero ts2 if not found
		j = findTS(buf+i+t1ofs, ts2);

		// If timestamp was at the beginning, skipping it
		if (!t1ofs && j && (strlen(ts2) >= j-2))
		{
			i += j;
			// Getting rid of garbage after ts2
			while (buf[i] == ' ' || buf[i] == '\t' || buf[i] == '-' || buf[i] == '.' || buf[i] == ')' || buf[i] == ']') i++;
		}
	} else {
		// Timestamp not found
		ts2[0] = 0;
		if (lineNo == 1) strcpy(ts1, "0:00");
		else ts1[0] = 0;
	}

	// Finding track title. i points to the first character, if line begins with timestamps they are already skipped
	if (t1ofs)
	{
		// Timestamp in the end, t1ofs points to after the timestamp, searching back to its beginning
		for (j = i+t1ofs-1; j>i; j--) if (!isdigit(buf[j]) && buf[j] != ':' && buf[j] != '(' && buf[j] != '[') break;
		buf[j+1] = 0;
	}
	// Getting rid of garbage at the end of the line
	for (j=i+strlen(buf+i)-1; j>i; j--) if (buf[j] != ' ' && buf[j] != '\t' && buf[j] != '-') break;
	if (j == i)
	{
		// Recovering pure '---' titled tracks
		if (buf[j] == '-') { while (buf[j] == '-') j++; j--; }
		  else return;
	}
	buf[j+1] = 0;

	// Printing script line
	OUTPRINTF("%s", CUTCMD);
	if (lineNo < 10) OUTPUTC('0');
	OUTPRINTF("%u - ", lineNo);
	while (buf[i])
	{
		// Taking care of escaping
		if (buf[i] == '\"' || buf[i] == '$') OUTPUTC('\\');
		if (buf[i] == '/') buf[i] = '-';
		if (buf[i] == '`') buf[i] = '\'';
		OUTPUTC(buf[i]);
		i++;
	}
	OUTPRINTF(".mp3\" -a %s -b ", ts1);
	if (ts2[0]) OUTPRINTF("%s", ts2);
	lineNo++;

}	// parseLine()



int main (int argc, char* argv[])
{

FILE *trkListFile;
char *fileBuf;
char *curLine;
int i;

	if (argc < 3) { DEBUGPRINTF( "Error: parameter missing\n\nUsage: %s <tracklist.txt|-> <album_video> [-]\n", argv[0]); return 1; }

	// Opening tracklist
	if (argv[1][0] == '-' && argv[1][1] == 0)
	{
		trkListFile = stdin;
	} else {
		trkListFile = fopen(argv[1], "rb");
		if (!trkListFile) { perror("fopen"); DEBUGPRINTF("Error opening file \"%s\"\n", argv[1]); return 2; }
	}

	fileBuf = (char*)malloc(BUFLEN);
	if (!fileBuf) { perror("malloc"); DEBUGPRINTF("Error allocating memory\n"); return 4; }

	// Opening output file
	if (argc > 3 && argv[3][0] == '-' && argv[3][1] == 0)
	{
		cutScript = stdout;
	} else {
		strcpy(fileBuf, argv[2]);
		for (i = strlen(fileBuf); i && fileBuf[i] != '.'; i--) continue;
		if (i && fileBuf[i] == '.') fileBuf[i] = 0;
		strcat (fileBuf, ".sh");
		cutScript = fopen(fileBuf, "w");
		if (!cutScript) { perror("fopen"); DEBUGPRINTF("Error opening output file \"%s\"\n", fileBuf); return 3; }
		// strtol("0755", 0, 8) == 0x1ED
		if (chmod (fileBuf, 0x1ED) < 0) { perror("chmod"); DEBUGPRINTF("Error changing output file mode\n"); }
	}

	i = fread (fileBuf, 1, BUFLEN, trkListFile);
	if (trkListFile != stdin) fclose(trkListFile);
	if (i < BUFLEN) fileBuf[i] = 0;
	  else { fileBuf[BUFLEN-1] = 0; DEBUGPRINTF("File exceeds buffer size %u\n", BUFLEN); }

	// Checking tracklist for numbers at the beginning of the line
	curLine = fileBuf;
	while(curLine)
	{
		char *nextLine = strchr(curLine, '\n');
		for (i = 0; isdigit(curLine[i]) || curLine[i] == '\t' || curLine[i] == ' '; i++) continue;
		if ((i == 0 || (curLine[i] == ':' && isdigit(curLine[i+1]))) && curLine[i] != 0 && curLine[i] != '\n') break;
		curLine = nextLine ? (nextLine+1) : NULL;
	}
	if (!curLine) flags |= F_CUTNUMBERS;

	// Script header
	OUTPRINTF("#!/bin/sh\n\n");
	for (i = strlen(argv[2]); i && argv[2][i] != '.'; i--) continue;
	if (i)
	{
		if ((argv[2][i+1] == 'M' || argv[2][i+1] == 'm') \
		 && (argv[2][i+2] == 'P' || argv[2][i+2] == 'p') \
		 &&  argv[2][i+3] == '3') flags |= F_ISMP3;
		  else OUTPRINTF(" flvcvt \"%s\"\n\n", argv[2]);
		argv[2][i] = 0;
	}
	if (!(flags & F_ISMP3))
	{
		for (i = 0; argv[2][i] && argv[2][i] != '['; i++) continue;
		if (argv[2][i]) { if (i && argv[2][i-1] == ' ') argv[2][i-1] = 0; else argv[2][i] = 0; }
	}
	OUTPRINTF("ALBUM=\"%s.mp3\"\nTDIR=\"%s\"\nmkdir -vp \"$TDIR\"\n", argv[2], argv[2]);

	// Reading tracklist
	curLine = fileBuf;
	while(curLine)
	{
		char *nextLine = strchr(curLine, '\n');
		if (nextLine) *nextLine = 0;
		parseLine(curLine);
		curLine = nextLine ? (nextLine+1) : NULL;
	}
	free (fileBuf);
	if (!ts2[0]) OUTPRINTF("999:99");

	if (flags & F_ISMP3) OUTPUTC('\n');
	  else OUTPRINTF("\n\n exec sed -i 's/^ /# /' \"$0\"\n");
	if (cutScript != stdout) fclose(cutScript);
	return 0;

}	// main()
