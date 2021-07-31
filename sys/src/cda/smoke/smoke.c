/*
 * Smoke - New static checker
 * (where there's smoke, there's fire)
 */

#include "smoke.h"
#include <stdio.h>

int aFlag = 0;
int bFlag = 0;
int cFlag = 0;
int dFlag = 0;
int gFlag = 0;
int mFlag = 0;
int nFlag = 0;
int pFlag = 0;
int sFlag = 0;
int tFlag = 0;
int uFlag = 0;
int wFlag = 0;
int xFlag = 0;
int LFlag = 0;
int lineNumber = 1;
int undefinedCount = 0;
int max_loads = 10;
char *fileName;

extern int warningCount;
extern char *wFileName;

void Trouble(char *, char *);
void Warning(char *, char *);
void Msg(char *, char *);
void Bug(char *);

void ReadAndBuildNetwork(FILE *);
void StaticCheck(void);
void CheckChips(void);
int CountUndefinedChips(void);
void DisplayChipsAndInstances(void);
void DisplayChips(int);

void main(int argc, char **argv)
{
	FILE *wxStream;
	int i;

	if (argc == 1)
	{
		(void) Trouble("correct usage: smoke [-ablmpx] *.wx pins", NULL);
#ifdef PLAN9
		exits("incorrect argument usage");
#else
		exit(1);
#endif
	} /* end if */
	setbuf(stdout, (char *) malloc(BUFSIZ));
	ARGBEGIN{
		case 'a':
			aFlag++;
			break;
		case 'b':
			bFlag++;
			break;
		case 'c':
			cFlag++;
			break;
		case 'd':
			dFlag++;
			break;
		case 'g':
			gFlag++;
			break;
		case 'l':
			max_loads = atoi(ARGF());
			break;
		case 'm':
			mFlag++;
		case 'n':
			nFlag++;
			break;
		case 'p':
			pFlag++;
			break;
		case 's':
			sFlag++;
			break;
		case 't':
			tFlag++;
			break;
		case 'u':
			uFlag++; /* nop now */
			break;
		case 'w':
			wFlag++;
			break;
		case 'x':
			xFlag++;
			break;
		case 'L':
			LFlag++;
			break;
		default:
				(void) Trouble("Illegal switch; ignored", NULL);
	}ARGEND
	if (argc <= 0) {
		(void) ReadAndBuildNetwork(stdin);
	}
	else {
		for(i=0; i<argc; i++) {
			wxStream = fopen(argv[i], "r");
			if (wxStream == (FILE *) 0)
			{
				(void) Trouble("couldn't open %s", argv[i]);
				perror("smoke");
				break;
			} /* end if */
			lineNumber = 1;
			wFileName = "";
			fprintf(stderr, "%s:\n", argv[i]);
			fileName = argv[i];
			(void) ReadAndBuildNetwork(wxStream);
			fclose(wxStream);
		} /* end for */
	} /* end else */

	(void) StaticCheck();
	if (warningCount > 0)
		fprintf(stderr, "smoke: %d warning(s).\n", warningCount);
	warningCount = 0;
	if (pFlag) (void) CheckChips();
	if (warningCount > 0)
		fprintf(stderr, "smoke: %d input pins not connected.\n", warningCount);

	undefinedCount = CountUndefinedChips();
	if (gFlag) (void) DisplayChipsAndInstances();
	if (pFlag | gFlag | xFlag) (void) DisplayChips(pFlag | xFlag);
	if (undefinedCount > 0)
		fprintf(stderr, "smoke: %d undefined chip(s).\n", undefinedCount);
#ifdef PLAN9
	(void) exits((char *) 0);
#else
	(void) exit(0); /* so Make is happy */
#endif
} /* end main */

void Trouble(char *message, char *argument)
{
	fprintf(stderr, "smoke: ");
	fprintf(stderr, message, argument);
	fprintf(stderr, "\n");
} /* end Trouble */

void Warning(char *message, char *argument)
{
	fprintf(stderr, "smoke: %s, line %d: ", fileName, lineNumber);
	fprintf(stderr, message, argument);
	fprintf(stderr, "\n");
	warningCount++;
} /* end Warning */

void Bug(char *message)
{
	fprintf(stderr, "smoke bug: ");
	fprintf(stderr, message);
	fprintf(stderr, "\n");
	abort();
} /* end Bug */
