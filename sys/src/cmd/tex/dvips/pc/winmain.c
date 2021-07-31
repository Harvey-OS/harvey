/*
 * dvips - winmain.c
 *   This module is Copyright 1992 by Russell Lang and Maurice Castro.
 *   This file may be freely copied and modified.
 */

#include <windows.h>
#include <dos.h>
#include <stdio.h>
#include <string.h>

/* local */
#define MAXSTR 255
HWND FAR hwndeasy;
static char FAR szAppName[] = "dvips";
char winline[MAXSTR];	/* command line for MS-Windows */
int wargc;		/* argc for windows */
char *wargv[32];	/* argv for windows */

/* external */
extern void help(); 	/* in dvips.c */
extern void error();	/* in dvips.c */

/* EasyWin */
extern POINT _ScreenSize;

int main(int argc, char *argv[], char *env[]);

/* A fake system() for Microsoft Windows */
int
system(command)
char *command;
{
char str[MAXSTR];
   strcpy(str,"Windows can't do system(\042");
   if (command) {
     strncat(str,command,MAXSTR-strlen(str));
   }
   strncat(str,"\042);",MAXSTR-strlen(str));
   error(str);
   return(1);  /* error */
}

/* Get a new command line */
void
winargs()
{
   fputs("Options: ",stdout);
   fgets(winline,MAXSTR,stdin);
   wargc=1;
   if ( (wargv[1] = strtok(winline," \n")) != (char *)NULL ) {
      wargc=2;
      while ( ((wargv[wargc] = strtok((char *)NULL," \n")) != (char *)NULL)
            && (wargc < 31) )
         wargc++;
   }
   wargv[wargc] = (char *)NULL;
}

int PASCAL WinMain(HANDLE hInstance, HANDLE hPrevInstance,
		LPSTR lpszCmdLine, int cmdShow)
{
	char modulename[MAXSTR];

        /* start up the text window */
	_ScreenSize.y = 50;
	_InitEasyWin();

	/* fix up the EasyWindows window provided by Borland */
	GetModuleFileName(hInstance, (LPSTR) modulename, MAXSTR);
	hwndeasy = FindWindow("BCEasyWin", modulename);
	SetWindowText(hwndeasy, szAppName);            /* change title */
	SetClassWord(hwndeasy, GCW_HICON, LoadIcon(hInstance, "RadicalEye")); /* change icon */

	if (_argc==1) {
		/* get new command line if no options or filenames */
		help();
		winargs();
		wargv[0] = _argv[0];
		_argc=wargc;
		_argv=wargv;
	}

	main(_argc, _argv, environ);
	/* unfortunately dvips doesn't return from main(), it exits */
	/* so the following code is never executed */
	DestroyWindow(hwndeasy);
	return 0;
}
