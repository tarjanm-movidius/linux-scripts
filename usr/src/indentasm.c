#if 0
##### Run this file in shell to compile (e.g. sh indentasm.c) #####
echo "gcc -O2 -Wall -o indentasm indentasm.c"
gcc -O2 -Wall -o indentasm indentasm.c
exit
faszom
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <unistd.h>

#define DEBUGPRINTF(...)	fprintf(stderr, __VA_ARGS__)
#define OUTPRINTF(...)	fprintf(outFile, __VA_ARGS__)
#define OUTPUTC(ch_)	putc((ch_), outFile)
#define BUFLEN			65536
#define CUTCMD			"\ncutmp3 -i \"$ALBUM\" -O \"$TDIR/"

#define F_CUTNUMBERS	1
#define F_ISMP3		2

char ts2[10], flags=0;
FILE *outFile;

typedef struct sfuncName_t
{
	struct sfuncName_t *next;
	unsigned int loc;
	char name[0];
} funcName_t;

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

FILE *logFile;
char *buf;
char *curLine;
const char *path1 = "/mdk/testApps/keembay/tapeout_so";
const char *path2 = "/output/lst/";
const char *path3 = "_leon.lst";
unsigned int i, j, k;
unsigned int indents[3] = {2, 2, 2};
funcName_t funcNames = {0, 0};
funcName_t *fptr = &funcNames;

	if (argc < 2) { DEBUGPRINTF( "Error: parameter missing\n\nUsage: %s <ts_xxx.sim.log> [ts_xxx_leon.lst] [-]\n", argv[0]); return 1; }

	// Opening logfile
	logFile = fopen(argv[1], "rb");
//	if (!logFile) { perror("fopen"); DEBUGPRINTF("Error opening file \"%s\"\n", argv[1]); return 2; }

	buf = (char*)malloc(BUFLEN);
	if (!buf) { perror("malloc"); DEBUGPRINTF("Error allocating memory\n"); return 3; }

	// Opening disassembly listing
	if (argc > 2 && (argv[2][0] != '-' || argv[2][1] != 0))
	{
		// File specified
		strcpy (buf, argv[2]);
		i = 1;
	} else {
		if (argv[1][0] == '/')
		{
			// Logfile is absolute path
			strcpy (buf, argv[1]);
		} else {
			if (!getcwd(buf, BUFLEN)) { DEBUGPRINTF("Error getting working directory\n" ); return 4; }
			strcat (buf, "/");
			strcat (buf, argv[1]);
		}
		for (j=2, i=strlen(buf); j && i; i--) if (buf[i] == '/') j--;
		buf[i+1] = 0;
		if (!i) goto lsterr;
		for (i--; i && buf[i] != '.'; i--) continue;
		if (!i) goto lsterr;
		buf[i] = '/';
		for (i--; i && buf[i] != '/'; i--) continue;
		if (!i) goto lsterr;
		k = i;
		for (j=5, i--; j && i; i--) if (buf[i] == '/') j--;
		if (!i) goto lsterr;
		for (i++, j=0; path1[j]; i++, j++) buf[i] = path1[j];
		for (; buf[k]; i++, k++) if ((buf[i] = buf[k]) == '/') j = i+1;
		// j points to the beginning of the project name
		for (k=0; path2[k]; i++, k++) buf[i] = path2[k];
		for (k=j; buf[k]!='/'; i++, k++) buf[i] = buf[k];
		for (k=0; path3[k]; i++, k++) buf[i] = path3[k];
		lsterr:
		buf[i] = 0;
	}
	if (i && (outFile = fopen(buf, "rb")))
	{
		// Parsing disassembly listing for function names
		while (!feof(outFile))
		{
			if(!fgets(buf, BUFLEN, outFile)) continue;
			if (!i)
			{
				// 7010c218 <fclose>:
				for (i=0; i < 18 && buf[i] != '\n' && buf[i] != '<'; i++) continue;
				if (buf[i] != '<') continue;
				if (!sscanf(buf, "%x ", &k)) continue;
				for (j = ++i; buf[j] != '\n' && buf[j] != '>'; j++) continue;
				if (buf[j] != '>') continue;
				buf[j] = 0;
				fptr->next = (funcName_t*)malloc(sizeof(funcName_t)+j-i+1);
				if(!fptr->next) { perror("malloc"); DEBUGPRINTF("Error allocating memory\n"); return 3; }
				fptr = fptr->next;
				fptr->loc = k;
				strcpy(fptr->name, buf+i);
//				printf("%X  %s", j, buf);
				i = 1;
			} else {
				// Empty line
				if (buf[0] == '\n') i = 0;
			}
		}
		fptr->next = NULL;
		fclose(outFile);
	} else {
		DEBUGPRINTF("Error opening disassembly listing %s\n", buf);
	}
/*
for (fptr=funcNames.next; fptr; fptr = fptr->next)
{
printf("%X : %s\n", fptr->loc, fptr->name);
}
*/

	// Opening output file
	if ((argc == 3 && argv[2][0] == '-' && argv[2][1] == 0) || \
	    (argc > 3  && argv[3][0] == '-' && argv[3][1] == 0))
	{
		outFile = stdout;
	} else {
		strcpy(buf, argv[1]);
		for (i = strlen(buf); i && buf[i] != '.'; i--) continue;
		if (i && buf[i] == '.') buf[i] = 0;
		strcat (buf, "_indented.log");
		outFile = fopen(buf, "w");
		if (!outFile) { perror("fopen"); DEBUGPRINTF("Error opening output file \"%s\"\n", buf); return 3; }
	}

	while (!feof(logFile))
	{
/*
 3655908.171 NS : cpu0: 0x702014e0    mov  0x0000, %o4  [0x00000000]
3834651.631186 NS : cpu0: 0x702011e8    sethi  %hi(0x7ff17c00), %g2  [0x7ff17c00]
*/
		if(!fgets(buf, BUFLEN, logFile)) continue;
		for (i=0; i < 9 && isdigit(buf[i]) || buf[i] == ' '; i++) continue;
		if (buf[i] != '.') goto justprint;
		j = i;  // Timestamp decimal point
		for (; i < 18 && isdigit(buf[i]) || buf[i] == ' '; i++) continue;
		if (buf[i] != 'N' || buf[i+1] != 'S') goto justprint;
		i += 2;
		k = i;  // End of timestamp
		for (; i < 22 && buf[i] == ':' || buf[i] == ' '; i++) continue;
		if (buf[i] != 'c' || buf[i+1] != 'p' || buf[i+2] != 'u') goto justprint;
		i += 3;
		int core = buf[i] - '0';
		i += 2;
		// Printing initial spaces to align timestamp decimal point
		for (; j<9; j++) OUTPUTC(' ');
		// Printing timestamp and trailing spaces
		for (j=0; j<k; j++) OUTPUTC(buf[j]);
		for (j=0; j<19-k; j++) OUTPUTC(' ');
		// Printing 'cpuX:'
		for (j=k; j<i; j++) OUTPUTC(buf[j]);
		// Parsing and printing hex offset
		for (; i < 40 && buf[i] != 'x'; i++) continue;
		OUTPRINTF(" 0x");
		for (i++; i < 40 && isdigit(buf[i]) || (buf[i] >= 'a' && buf[i] <= 'f') ; i++) OUTPUTC(buf[i]);
		for (; i < 45 && buf[i] == ' '; i++) continue;
		for (j=0; j<indents[core]; j++) OUTPUTC(' ');
		if (buf[i] == 'r' && buf[i+1] == 'e' && buf[i+2] == 't' && indents[core] > 2) indents[core] -= 2;
		for (j = i; buf[j] && buf[j] != '\n'; i++) OUTPUTC(buf[j]);
//		OUTPRINTF("%s", buf + i);
		if (buf[i] == 'c' && buf[i+1] == 'a' && buf[i+2] == 'l' && buf[i+3] == 'l')
		{
			indents[core] += 2;
// 5337164.90089 NS : cpu0: 0x701e07b8    call  0x701c4a1c  [0x701e07b8]
			for (; i < 55 && buf[i] != 'x'; i++) continue;

		}

		continue;
		justprint:
		OUTPRINTF("%s", buf);
	}

return 0;

	while (!feof(logFile))
	i = fread (buf, 1, BUFLEN, logFile);
	if (logFile != stdin) fclose(logFile);
	if (i < BUFLEN) buf[i] = 0;
	  else { buf[BUFLEN-1] = 0; DEBUGPRINTF("File exceeds buffer size %u\n", BUFLEN); }

	// Checking tracklist for numbers at the beginning of the line
	curLine = buf;
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
	for (i = 0; argv[2][i] && argv[2][i] != '['; i++) continue;
	if (argv[2][i]) { if (i && argv[2][i-1] == ' ') argv[2][i-1] = 0; else argv[2][i] = 0; }
	OUTPRINTF("ALBUM=\"%s.mp3\"\nTDIR=\"%s\"\nmkdir -vp \"$TDIR\"\n", argv[2], argv[2]);

	// Reading tracklist
	curLine = buf;
	while(curLine)
	{
		char *nextLine = strchr(curLine, '\n');
		if (nextLine) *nextLine = 0;
		parseLine(curLine);
		curLine = nextLine ? (nextLine+1) : NULL;
	}
	free (buf);
	if (!ts2[0]) OUTPRINTF("999:99");

	if (flags & F_ISMP3) OUTPUTC('\n');
	  else OUTPRINTF("\n\n exec sed -i 's/^ /# /' \"$0\"\n");
	if (outFile != stdout) fclose(outFile);
	return 0;

}	// main()
