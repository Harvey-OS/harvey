/* Copyright (C) 1999-2003, Ghostgum Software Pty Ltd.  All rights reserved.
  
  This software is provided AS-IS with no warranty, either express or
  implied.
  
  This software is distributed under license and may not be copied,
  modified or distributed except as expressly authorized under the terms
  of the license contained in the file LICENSE in this distribution.
  
  For more information about licensing, please refer to
  http://www.ghostscript.com/licensing/. For information on
  commercial licensing, go to http://www.artifex.com/licensing/ or
  contact Artifex Software, Inc., 101 Lucas Valley Road #110,
  San Rafael, CA  94903, U.S.A., +1(415)492-9861.
*/

// $Id: dwsetup.cpp,v 1.11 2005/03/04 21:58:55 ghostgum Exp $
//
//
// This is the setup program for Win32 AFPL Ghostscript
//
// The starting point is a self extracting zip archive
// with the following contents:
//   setupgs.exe
//   uninstgs.exe
//   filelist.txt      (contains list of program files)
//   fontlist.txt      (contains list of font files)
//   gs#.##\*          (files listed in filelist.txt)
//   fonts\*           (fonts listed in fontlist.txt)
// This is the same as the zip file created by Aladdin Enterprises,
// with the addition of setupgs.exe, uninstgs.exe, filelist.txt and 
// fontlist.txt.
//
// The first line of the files filelist.txt and fontlist.txt
// contains the uninstall name to be used.  
// The second line contains name of the main directory where 
// uninstall log files are to be placed.  
// Subsequent lines contain files to be copied (but not directories).
// For example, filelist.txt might contain:
//   AFPL Ghostscript 6.50
//   gs6.50
//   gs6.50\bin\gsdll32.dll
//   gs6.50\lib\gs_init.ps
// The file fontlist.txt might contain:
//   AFPL Ghostscript Fonts
//   fonts
//   fonts\n019003l.pfb
//   fonts\n019023l.pfb
//
// The default install directory is c:\gs.
// The default Start Menu Folder is Ghostscript.
// These are set in the resources.
// The setup program will create the following uninstall log files
//   c:\gs\gs#.##\uninstal.txt
//   c:\gs\fonts\uninstal.txt
// The uninstall program (accessed through control panel) will not 
// remove directories nor will it remove itself.
//
// If the install directory is the same as the current file 
// location, no files will be copied, but the existence of each file 
// will be checked.  This allows the archive to be unzipped, then
// configured in its current location.  Running the uninstall will not 
// remove uninstgs.exe, setupgs.exe, filelist.txt or fontlist.txt.


#define STRICT
#include <windows.h>
#include <shellapi.h>
#include <objbase.h>
#include <shlobj.h>
#include <stdio.h>
#include <direct.h>

#ifdef MAX_PATH
#define MAXSTR MAX_PATH
#else
#define MAXSTR 256
#endif

#include "dwsetup.h"
#include "dwinst.h"

extern "C" {
typedef HRESULT (WINAPI *PFN_SHGetFolderPath)(
  HWND hwndOwner,
  int nFolder,
  HANDLE hToken,
  DWORD dwFlags,
  LPSTR pszPath);

typedef BOOL (WINAPI *PFN_SHGetSpecialFolderPath)(
  HWND hwndOwner,
  LPTSTR lpszPath,
  int nFolder,
  BOOL fCreate);
}

//#define DEBUG

#define UNINSTALLPROG "uninstgs.exe"


/////////////////////////////////
// Globals

CInstall cinst;

// TRUE = Place Start Menu items in All Users.
// FALSE = Current User
BOOL g_bUseCommon;

// TRUE = Destination is the same as Source, so don't copy files.
BOOL g_bNoCopy;

// Source directory, usually a temporary directory created by
// unzip self extractor.
CHAR g_szSourceDir[MAXSTR];

// Target directory for program and fonts.
// Default loaded from resources
CHAR g_szTargetDir[MAXSTR];

// Target Group for shortcut.
// Default loaded from resources
CHAR g_szTargetGroup[MAXSTR];

// Setup application name, loaded from resources
CHAR g_szAppName[MAXSTR];

BOOL g_bInstallFonts = TRUE;
BOOL g_bCJKFonts = FALSE;
BOOL g_bAllUsers = FALSE;


HWND g_hMain;		// Main install dialog
HWND g_hWndText;	// Install log dialog
HINSTANCE g_hInstance;

// If a directory is listed on the command line, g_bBatch will
// be TRUE and a silent install will occur.
BOOL g_bBatch = FALSE;	

BOOL g_bQuit = FALSE;	// TRUE = Get out of message loop.
BOOL g_bError = FALSE;	// TRUE = Install was not successful
BOOL is_winnt = FALSE;	// Disable "All Users" if not NT.

#ifdef _WIN64
#define DLGRETURN INT_PTR
#else
#define DLGRETURN BOOL
#endif

// Prototypes
DLGRETURN CALLBACK MainDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void gs_addmess_count(const char *str, int count);
void gs_addmess(const char *str);
void gs_addmess_update(void);
BOOL init();
BOOL install_all();
BOOL install_prog();
BOOL install_fonts();
BOOL make_filelist(int argc, char *argv[]);
int get_font_path(char *path, unsigned int pathlen);
BOOL write_cidfmap(const char *gspath, const char *cidpath);
BOOL GetProgramFiles(LPTSTR path);


//////////////////////////////////////////////////////////////////////
// Entry point
//////////////////////////////////////////////////////////////////////

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
	MSG msg;
	g_hInstance = hInstance;
	
	if (!init()) {
		MessageBox(HWND_DESKTOP, "Initialisation failed", 
			g_szAppName, MB_OK);
		return 1;
	}
	
	if (!g_bBatch) {
		while (GetMessage(&msg, (HWND)NULL, 0, 0)) {
			if (!IsDialogMessage(g_hWndText, &msg) && 
				!IsDialogMessage(g_hMain, &msg)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		DestroyWindow(g_hMain);
	}
	
	return (g_bError ? 1 : 0);
}




//////////////////////////////////////////////////////////////////////
// Text log window
//////////////////////////////////////////////////////////////////////


#define TWLENGTH 32768
#define TWSCROLL 1024
char twbuf[TWLENGTH];
int twend;

// Modeless Dialog Box
DLGRETURN CALLBACK 
TextWinDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch(message) {
	case WM_INITDIALOG:
		EnableWindow(g_hMain, FALSE);
		return TRUE;
	case WM_COMMAND:
		switch(LOWORD(wParam)) {
		case IDC_TEXTWIN_COPY:
			{HGLOBAL hglobal;
			LPSTR p;
			DWORD result;
			int start, end;
			result = SendDlgItemMessage(hwnd, IDC_TEXTWIN_MLE, EM_GETSEL, (WPARAM)0, (LPARAM)0);
			start = LOWORD(result);
			end   = HIWORD(result);
			if (start == end) {
				start = 0;
				end = twend;
			}
			hglobal = GlobalAlloc(GHND | GMEM_SHARE, end-start+1);
			if (hglobal == (HGLOBAL)NULL) {
				MessageBeep(-1);
				return(FALSE);
			}
			p = (char *)GlobalLock(hglobal);
			if (p == (LPSTR)NULL) {
				MessageBeep(-1);
				return(FALSE);
			}
			lstrcpyn(p, twbuf+start, end-start);
			GlobalUnlock(hglobal);
			OpenClipboard(hwnd);
			EmptyClipboard();
			SetClipboardData(CF_TEXT, hglobal);
			CloseClipboard();
			}
			break;
		case IDCANCEL:
			g_bQuit = TRUE;
			DestroyWindow(hwnd);
			return TRUE;
		}
		break;
	case WM_CLOSE:
			DestroyWindow(hwnd);
			return TRUE;
	case WM_DESTROY:
			g_bQuit = TRUE;
			g_hWndText = (HWND)NULL;
			EnableWindow(g_hMain, TRUE);
			PostQuitMessage(0);
			break;
	}
	return FALSE;
}



// Add string to log window
void
gs_addmess_count(const char *str, int count)
{
	const char *s;
	char *p;
	int i, lfcount;
	MSG msg;
	
	// we need to add \r after each \n, so count the \n's
	lfcount = 0;
	s = str;
	for (i=0; i<count; i++) {
		if (*s == '\n')
			lfcount++;
		s++;
	}
	
	if (count + lfcount >= TWSCROLL)
		return;		// too large
	if (count + lfcount + twend >= TWLENGTH-1) {
		// scroll buffer
		twend -= TWSCROLL;
		memmove(twbuf, twbuf+TWSCROLL, twend);
	}
	p = twbuf+twend;
	for (i=0; i<count; i++) {
		if (*str == '\n') {
			*p++ = '\r';
		}
		*p++ = *str++;
	}
	twend += (count + lfcount);
	*(twbuf+twend) = '\0';
	
	
	// Update the dialog box
	if (g_bBatch)
		return;
	
	gs_addmess_update();
	while (PeekMessage(&msg, (HWND)NULL, 0, 0, PM_REMOVE)) {
		if (!IsDialogMessage(g_hWndText, &msg) && 
			!IsDialogMessage(g_hMain, &msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
}

void
gs_addmess(const char *str)
{
	gs_addmess_count(str, lstrlen(str));
	
}


void
gs_addmess_update(void)
{
	HWND hwndmess = g_hWndText;
	
	if (g_bBatch)
		return;
	
	if (IsWindow(hwndmess)) {
		HWND hwndtext = GetDlgItem(hwndmess, IDC_TEXTWIN_MLE);
		DWORD linecount;
		SendMessage(hwndtext, WM_SETREDRAW, FALSE, 0);
		SetDlgItemText(hwndmess, IDC_TEXTWIN_MLE, twbuf);
		linecount = SendDlgItemMessage(hwndmess, IDC_TEXTWIN_MLE, EM_GETLINECOUNT, (WPARAM)0, (LPARAM)0);
		SendDlgItemMessage(hwndmess, IDC_TEXTWIN_MLE, EM_LINESCROLL, (WPARAM)0, (LPARAM)linecount-14);
		SendMessage(hwndtext, WM_SETREDRAW, TRUE, 0);
		InvalidateRect(hwndtext, (LPRECT)NULL, TRUE);
		UpdateWindow(hwndtext);
	}
}


//////////////////////////////////////////////////////////////////////
// Browse dialog box 
//////////////////////////////////////////////////////////////////////

// nasty GLOBALS
char szFolderName[MAXSTR];
char szDirName[MAXSTR];

DLGRETURN CALLBACK 
DirDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	WORD notify_message;
	
	switch(message) {
	case WM_INITDIALOG:
		DlgDirList(hwnd, szDirName, IDC_FILES, IDC_FOLDER, 
			DDL_DRIVES | DDL_DIRECTORY);
		SetDlgItemText(hwnd, IDC_TARGET, szFolderName);
		return FALSE;
    case WM_COMMAND:
		notify_message = HIWORD(wParam);
		switch (LOWORD(wParam)) {
		case IDC_FILES:
			if (notify_message == LBN_DBLCLK) {
				CHAR szPath[MAXSTR];
				DlgDirSelectEx(hwnd, szPath, sizeof(szPath), IDC_FILES);
				DlgDirList(hwnd, szPath, IDC_FILES, IDC_FOLDER, 
					DDL_DRIVES | DDL_DIRECTORY);
			}
			return FALSE;
		case IDOK:
			GetDlgItemText(hwnd, IDC_FOLDER, szDirName, sizeof(szDirName));
			GetDlgItemText(hwnd, IDC_TARGET, szFolderName, sizeof(szFolderName));
			EndDialog(hwnd, TRUE);
			return TRUE;
		case IDCANCEL:
			EndDialog(hwnd, FALSE);
			return TRUE;
		}
		return FALSE;
	}
	return FALSE;
}


//////////////////////////////////////////////////////////////////////
// Initialisation and Main dialog box
//////////////////////////////////////////////////////////////////////

void
message_box(const char *str)
{
	MessageBox(HWND_DESKTOP, str, g_szAppName, MB_OK);
}


BOOL
init()
{
	DWORD dwVersion = GetVersion();
	// get source directory
	GetCurrentDirectory(sizeof(g_szSourceDir), g_szSourceDir);
	
	// load strings
	LoadString(g_hInstance, IDS_APPNAME, g_szAppName, sizeof(g_szAppName));
	LoadString(g_hInstance, IDS_TARGET_GROUP, 
		g_szTargetGroup, sizeof(g_szTargetGroup));
	
	if (LOBYTE(LOWORD(dwVersion)) < 4) {
        MessageBox(HWND_DESKTOP, 
			"This install program needs Windows 4.0 or later",
			g_szAppName, MB_OK);
		return FALSE;
	}
	if ( (HIWORD(dwVersion) & 0x8000) == 0)
		is_winnt = TRUE;
	
	
	cinst.SetMessageFunction(message_box);

#define MAXCMDTOKENS 128

	int argc;
	LPSTR argv[MAXCMDTOKENS];
	LPSTR p;
	char command[256];
	char *args;
	char *d, *e;
     
	p = GetCommandLine();

	argc = 0;
	args = (char *)malloc(lstrlen(p)+1);
	if (args == (char *)NULL)
		return 1;
       
	// Parse command line handling quotes.
	d = args;
	while (*p) {
		// for each argument

		if (argc >= MAXCMDTOKENS - 1)
			break;

		e = d;
		while ((*p) && (*p != ' ')) {
			if (*p == '\042') {
				// Remove quotes, skipping over embedded spaces.
				// Doesn't handle embedded quotes.
				p++;
				while ((*p) && (*p != '\042'))
					*d++ =*p++;
			}
			else 
				*d++ = *p;
			if (*p)
				p++;
		}
		*d++ = '\0';
		argv[argc++] = e;

		while ((*p) && (*p == ' '))
			p++;	// Skip over trailing spaces
	}
	argv[argc] = NULL;

	if (strlen(argv[0]) == 0) {
		GetModuleFileName(g_hInstance, command, sizeof(command)-1);
		argv[0] = command;
	}

	if (argc > 2) {
		// Probably creating filelist.txt
		return make_filelist(argc, argv);
	}


	// check if batch mode requested
	// get location of target directory from command line as argv[1]
	if (argc == 2) {
		strncpy(g_szTargetDir, argv[1], sizeof(g_szTargetDir));
		g_bBatch = TRUE;
		if (is_winnt)
			g_bAllUsers = TRUE;
	}
	if (g_bBatch) {
		if (!install_all()) {
			// display log showing error
			g_bBatch = FALSE;
			g_hWndText = CreateDialogParam(g_hInstance, 
				MAKEINTRESOURCE(IDD_TEXTWIN), 
				(HWND)HWND_DESKTOP, TextWinDlgProc, 
				(LPARAM)NULL);
			gs_addmess_update();
		}
		return TRUE;
	}
	
	// Interactive setup
	if (!GetProgramFiles(g_szTargetDir))
	    strcpy(g_szTargetDir, "C:\\Program Files");
	strcat(g_szTargetDir, "\\");
	LoadString(g_hInstance, IDS_TARGET_DIR, 
	    g_szTargetDir+strlen(g_szTargetDir), 
	    sizeof(g_szTargetDir)-strlen(g_szTargetDir));
	
	// main dialog box
	g_hMain = CreateDialogParam(g_hInstance, MAKEINTRESOURCE(IDD_MAIN), (HWND)NULL, MainDlgProc, (LPARAM)NULL);
	// centre dialog on screen
	int width = GetSystemMetrics(SM_CXFULLSCREEN);
	int height = GetSystemMetrics(SM_CYFULLSCREEN);
	RECT rect;
	GetWindowRect(g_hMain, &rect);
	MoveWindow(g_hMain, (width - (rect.right - rect.left))/2,
		(height - (rect.bottom - rect.top))/2,
		(rect.right - rect.left),
		(rect.bottom - rect.top), FALSE);
	
	// initialize targets
	cinst.SetMessageFunction(message_box);
	if (!cinst.Init(g_szSourceDir, "filelist.txt"))
		return FALSE;

	SetDlgItemText(g_hMain, IDC_TARGET_DIR, g_szTargetDir);
	SetDlgItemText(g_hMain, IDC_TARGET_GROUP, g_szTargetGroup);
	SetDlgItemText(g_hMain, IDC_PRODUCT_NAME, cinst.GetUninstallName());
	SendDlgItemMessage(g_hMain, IDC_INSTALL_FONTS, BM_SETCHECK, BST_CHECKED, 0);
	ShowWindow(g_hMain, SW_SHOWNORMAL);
	
	return (g_hMain != (HWND)NULL); /* success */
}


// Main Modeless Dialog Box
DLGRETURN CALLBACK 
MainDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message) {
	case WM_INITDIALOG:
		EnableWindow(GetDlgItem(hwnd, IDC_ALLUSERS), is_winnt);
		return TRUE;
	case WM_COMMAND:
		switch(LOWORD(wParam)) {
		case IDC_README:
			{
			char buf[MAXSTR];
			sprintf(buf, "%s\\%s\\doc\\Readme.htm", g_szSourceDir,
				cinst.GetMainDir());
			ShellExecute(hwnd, NULL, buf, NULL, g_szSourceDir, 
				SW_SHOWNORMAL);
			}
			return TRUE;
		case IDC_BROWSE_DIR:
			{ char dir[MAXSTR];
			char *p;
			GetDlgItemText(hwnd, IDC_TARGET_DIR, dir, sizeof(dir));
			strcpy(szDirName, dir);
			if ( (p = strrchr(szDirName, '\\')) != (char *)NULL ) {
				strcpy(szFolderName, p+1);
				if (p == szDirName+2)
					p++;	// step over c:\   //
				*p = '\0';
			}
			else {
				strcpy(szDirName, "c:\\");
				strcpy(szFolderName, dir);
			}
			if (DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_DIRDLG), 
				hwnd, DirDlgProc)) {
				strcpy(dir, szDirName);
				if (strlen(dir) && (dir[strlen(dir)-1] != '\\'))
					strcat(dir, "\\");
				strcat(dir, szFolderName);
				SetDlgItemText(hwnd, IDC_TARGET_DIR, dir);
			}
			}
			return TRUE;
		case IDC_BROWSE_GROUP:
			{ char dir[MAXSTR];
			char programs[MAXSTR];
			char *p;
			GetDlgItemText(hwnd, IDC_TARGET_GROUP, dir, sizeof(dir));
			cinst.GetPrograms(
				SendDlgItemMessage(hwnd, IDC_ALLUSERS,
				BM_GETCHECK, 0, 0) == BST_CHECKED,
				programs, sizeof(programs));
			strcpy(szDirName, programs);
			strcpy(szFolderName, dir);
			if (DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_DIRDLG), 
				hwnd, DirDlgProc)) {
				strcpy(dir, szFolderName);
				p = szDirName;
				if (strnicmp(szDirName, programs, 
					strlen(programs)) == 0) {
					p += strlen(programs);
					if (*p == '\\')
						p++;
					strcpy(dir, p);
					if (strlen(dir) && 
						(dir[strlen(dir)-1] != '\\'))
						strcat(dir, "\\");
					strcat(dir, szFolderName);
				}
				SetDlgItemText(hwnd, IDC_TARGET_GROUP, dir);
			}
			}
			return TRUE;
		case IDCANCEL:
			PostQuitMessage(0);
			return TRUE;
		case IDC_INSTALL:
			GetDlgItemText(hwnd, IDC_TARGET_DIR, 
				g_szTargetDir, sizeof(g_szTargetDir));
			GetDlgItemText(hwnd, IDC_TARGET_GROUP, 
				g_szTargetGroup, sizeof(g_szTargetGroup));
			g_bInstallFonts = (SendDlgItemMessage(g_hMain, 
				IDC_INSTALL_FONTS, BM_GETCHECK, 0, 0) 
				== BST_CHECKED);
			g_bCJKFonts = (SendDlgItemMessage(g_hMain, 
				IDC_CJK_FONTS, BM_GETCHECK, 0, 0) 
				== BST_CHECKED);
			g_bAllUsers = (SendDlgItemMessage(hwnd, 
				IDC_ALLUSERS, BM_GETCHECK, 0, 0
				) == BST_CHECKED);
			
			// install log dialog box
			g_hWndText = CreateDialogParam(g_hInstance, 
				MAKEINTRESOURCE(IDD_TEXTWIN), 
				(HWND)hwnd, TextWinDlgProc, (LPARAM)NULL);
			EnableWindow(GetDlgItem(hwnd, IDC_INSTALL), FALSE);
			if (install_all())
				PostQuitMessage(0);
			return TRUE;
		default:
			return(FALSE);
		}
		case WM_CLOSE:
			PostQuitMessage(0);
			return TRUE;
    }
    return FALSE;
}

// install program and files
BOOL
install_all()
{
	gs_addmess("Source Directory=");
	gs_addmess(g_szSourceDir);
	gs_addmess("\n");
	gs_addmess("Target Directory=");
	gs_addmess(g_szTargetDir);
	gs_addmess("\n");
	gs_addmess("Target Shell Folder=");
	gs_addmess(g_szTargetGroup);
	gs_addmess("\n");
	gs_addmess(g_bAllUsers ? "  All users\n" : "  Current user\n");
	
	if (stricmp(g_szSourceDir, g_szTargetDir) == 0) {
		// Don't copy files
		if (!g_bBatch)
			if (::MessageBox(g_hWndText, "Install location is the same as the current file location.  No files will be copied.", g_szAppName, MB_OKCANCEL) 
				!= IDOK) {
				return FALSE;
			}
		g_bNoCopy = TRUE;
	}
	
	
	if (g_bQuit)
		return FALSE;
	
	if (!install_prog()) {
		cinst.CleanUp();
		g_bError = TRUE;
		return FALSE;
	}
	if (g_bInstallFonts && !install_fonts()) {
		cinst.CleanUp();
		g_bError = TRUE;
		return FALSE;
	}
	
	gs_addmess("Install successful\n");
	
	// show start menu folder
	if (!g_bBatch) {
		char szFolder[MAXSTR];
		szFolder[0] = '\0';
		cinst.GetPrograms(g_bAllUsers, szFolder, sizeof(szFolder));
		strcat(szFolder, "\\");
		strcat(szFolder, g_szTargetGroup);
		ShellExecute(HWND_DESKTOP, "open", szFolder, 
			NULL, NULL, SW_SHOWNORMAL);
	}
	
#ifdef DEBUG
	return FALSE;
#endif

	return TRUE;
}

BOOL
install_prog()
{
	char *regkey1 = "AFPL Ghostscript";
	char regkey2[16];
	char szDLL[MAXSTR];
	char szLIB[MAXSTR+MAXSTR];
	char szProgram[MAXSTR];
	char szArguments[MAXSTR];
	char szDescription[MAXSTR];
	char szDotVersion[MAXSTR];
	char szPlatformSuffix[MAXSTR];
	const char *pSuffix = "";
	
	if (g_bQuit)
		return FALSE;
	
	cinst.SetMessageFunction(gs_addmess);
	cinst.SetTargetDir(g_szTargetDir);
	cinst.SetTargetGroup(g_szTargetGroup);
	cinst.SetAllUsers(g_bAllUsers);
	if (!cinst.Init(g_szSourceDir, "filelist.txt"))
		return FALSE;
	
	// Get GS version number
	gs_addmess("Installing Program...\n");
	int nGSversion = 0;
	const char *p = cinst.GetMainDir();
	while (*p && !isdigit(*p))	// skip over "gs" prefix
		p++;
	if (strlen(p) == 4)
		nGSversion = (p[0]-'0')*100 + (p[2]-'0')*10 + (p[3]-'0');
	else if (strlen(p) == 3)
		nGSversion = (p[0]-'0')*100 + (p[2]-'0')*10;
        strncpy(szDotVersion, p, sizeof(szDotVersion));
	strncpy(regkey2, szDotVersion, sizeof(regkey2));
	
	// copy files
	if (!cinst.InstallFiles(g_bNoCopy, &g_bQuit)) {
		gs_addmess("Program install failed\n");
		return FALSE;
	}
	
	if (g_bQuit)
		return FALSE;
	
	// write registry entries
	gs_addmess("Updating Registry\n");
	if (!cinst.UpdateRegistryBegin()) {
		gs_addmess("Failed to begin registry update\n");
		return FALSE;
	}
	if (!cinst.UpdateRegistryKey(regkey1, regkey2)) {
		gs_addmess("Failed to open/create registry application key\n");
		return FALSE;
	}
	strcpy(szDLL, g_szTargetDir);
	strcat(szDLL, "\\");
	strcat(szDLL, cinst.GetMainDir());
	strcat(szDLL, "\\bin\\gsdll32.dll");
	if (!cinst.UpdateRegistryValue(regkey1, regkey2, "GS_DLL", szDLL)) {
		gs_addmess("Failed to add registry value\n");
		return FALSE;
	}
	strcpy(szLIB, g_szTargetDir);
	strcat(szLIB, "\\");
	strcat(szLIB, cinst.GetMainDir());
	strcat(szLIB, "\\lib;");
	strcat(szLIB, g_szTargetDir);
	strcat(szLIB, "\\fonts;");
	strcat(szLIB, g_szTargetDir);
	strcat(szLIB, "\\");
	strcat(szLIB, cinst.GetMainDir());
	strcat(szLIB, "\\Resource");
	if (g_bCJKFonts) {
	    strcat(szLIB, ";");
	    get_font_path(szLIB+strlen(szLIB), sizeof(szLIB)-strlen(szLIB)-1);
	}
	if (!cinst.UpdateRegistryValue(regkey1, regkey2, "GS_LIB", szLIB)) {
		gs_addmess("Failed to add registry value\n");
		return FALSE;
	}
	if (!cinst.UpdateRegistryEnd()) {
		gs_addmess("Failed to end registry update\n");
		return FALSE;
	}
	if (g_bQuit)
		return FALSE;
	
	// Add Start Menu items
	gs_addmess("Adding Start Menu items\n");

        memset(szPlatformSuffix, 0, sizeof(szPlatformSuffix));
	if (GetProgramFiles(szPlatformSuffix)) {
	    /* If ProgramFiles has a suffix like " (x86)" then use
	     * it for Start menu entries to distinguish between
	     * 32-bit and 64-bit programs.
	     */
	    for (pSuffix = szPlatformSuffix; *pSuffix; pSuffix++)
		if ((pSuffix[0] == ' ') && (pSuffix[1] == '('))
		    break;
	}
	else {
	    pSuffix = "";
	}


	if (!cinst.StartMenuBegin()) {
		gs_addmess("Failed to begin Start Menu update\n");
		return FALSE;
	}
	strcpy(szProgram, g_szTargetDir);
	strcat(szProgram, "\\");
	strcat(szProgram, cinst.GetMainDir());
	strcat(szProgram, "\\bin\\gswin32.exe");
	strcpy(szArguments, "\042-I");
	strcat(szArguments, szLIB);
	strcat(szArguments, "\042");
	sprintf(szDescription, "Ghostscript %s%s", szDotVersion, pSuffix);
	if (!cinst.StartMenuAdd(szDescription, szProgram, szArguments)) {
		gs_addmess("Failed to add Start Menu item\n");
		return FALSE;
	}
	strcpy(szProgram, g_szTargetDir);
	strcat(szProgram, "\\");
	strcat(szProgram, cinst.GetMainDir());
	strcat(szProgram, "\\doc\\Readme.htm");
	sprintf(szDescription, "Ghostscript Readme %s%s", 
		szDotVersion, pSuffix);
	if (!cinst.StartMenuAdd(szDescription, szProgram, NULL)) {
		gs_addmess("Failed to add Start Menu item\n");
		return FALSE;
	}
	if (!cinst.StartMenuEnd()) {
		gs_addmess("Failed to end Start Menu update\n");
		return FALSE;
	}

        /* Create lib/cidfmap */
	if (g_bCJKFonts) {
		FILE *f;
		char szCIDFmap[MAXSTR];
		char szCIDFmap_bak[MAXSTR];
		char szGSPATH[MAXSTR];

		/* backup old cidfmap */
		strcpy(szCIDFmap, g_szTargetDir);
		strcat(szCIDFmap, "\\");
		strcat(szCIDFmap, cinst.GetMainDir());
		strcat(szCIDFmap, "\\lib\\cidfmap");
		strcpy(szCIDFmap_bak, szCIDFmap);
		strcat(szCIDFmap_bak, ".bak");
		gs_addmess("Backing up\n  ");
		gs_addmess(szCIDFmap);
		gs_addmess("\nto\n  ");
		gs_addmess(szCIDFmap_bak);
		gs_addmess("\n");
		rename(szCIDFmap, szCIDFmap_bak);

		/* mark backup for uninstall */
		cinst.AppendFileNew(szCIDFmap_bak);

		/* write new cidfmap */
		gs_addmess("Writing cidfmap\n   ");
		gs_addmess(szCIDFmap);
		gs_addmess("\n");
		strcpy(szGSPATH, g_szTargetDir);
		strcat(szGSPATH, "\\");
		strcat(szGSPATH, cinst.GetMainDir());
		if (!write_cidfmap(szGSPATH, szCIDFmap)) {
			gs_addmess("Failed to write cidfmap\n");
			return FALSE;
		}
	}
	
	// consolidate logs into one uninstall file
	if (cinst.MakeLog()) {
		// add uninstall entry for "Add/Remove Programs"
		gs_addmess("Adding uninstall program\n");
		if (!cinst.WriteUninstall(UNINSTALLPROG, g_bNoCopy)) {
			gs_addmess("Failed to write uninstall entry\n");
			return FALSE;
		}
	}
	else {
		gs_addmess("Failed to write uninstall log\n");
		// If batch install, files might be on a server
		// in a write protected directory.
		// Don't return an error for batch install.
		if (g_bBatch)
			return TRUE;
		return FALSE;
	}
	
	gs_addmess("Program install successful\n");
	return TRUE;
}


BOOL
install_fonts()
{
	cinst.SetMessageFunction(gs_addmess);
	cinst.SetTargetDir(g_szTargetDir);
	cinst.SetTargetGroup(g_szTargetGroup);
	cinst.SetAllUsers(g_bAllUsers);
	if (!cinst.Init(g_szSourceDir, "fontlist.txt"))
		return FALSE;
	
	// copy files
	if (!cinst.InstallFiles(g_bNoCopy, &g_bQuit)) {
		gs_addmess("Font install failed\n");
		return FALSE;
	}
	
	if (g_bQuit)
		return FALSE;
	
	if (g_bNoCopy) {
		// Don't write uninstall log or entry
		// since we didn't copy any files.
		cinst.CleanUp();
	}
	else {
		// consolidate logs into one uninstall file
		if (cinst.MakeLog()) {
			// add uninstall entry for "Add/Remove Programs"
			gs_addmess("Adding uninstall program\n");
			if (!cinst.WriteUninstall(UNINSTALLPROG, g_bNoCopy)) {
				gs_addmess("Failed to write uninstall entry\n");
				return FALSE;
			}
		}
		else {
			gs_addmess("Failed to write uninstall log\n");
			// If batch install, files might be on a server
			// in a write protected directory.
			// Don't return an error for batch install.
			if (g_bBatch)
				return TRUE;
			return FALSE;
		}
	}
	
	gs_addmess("Font install successful\n");
	return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Create lib/cidfmap based on installed fonts
//////////////////////////////////////////////////////////////////////

/* Get the path to enumerate for fonts */
int
get_font_path(char *path, unsigned int pathlen)
{
    int i;
    int len = GetWindowsDirectory(path, pathlen);
    if (len == 0)
       return -1;
    if (pathlen - strlen(path) < 8)
       return -1;
    strncat(path, "/fonts", pathlen - strlen(path) - 7);
    for (i = strlen(path)-1; i >= 0; i--)
       if (path[i] == '\\')
           path[i] = '/';
    return len;
}

BOOL write_cidfmap(const char *gspath, const char *cidpath)
{
    char fontpath[MAXSTR];
    char buf[4*MAXSTR];
    STARTUPINFO siStartInfo;
    PROCESS_INFORMATION piProcInfo;

    get_font_path(fontpath, sizeof(fontpath)-1);

    strcpy(buf, "\042");
    strcat(buf, gspath);
    strcat(buf, "\\bin\\gswin32c.exe\042 -q -dBATCH \042-sFONTDIR=");
    strcat(buf, fontpath);
    strcat(buf, "\042 \042");
    strcat(buf, "-sCIDFMAP=");
    strcat(buf, cidpath);
    strcat(buf, "\042 \042");
    strcat(buf, gspath);
    strcat(buf, "\\lib\\mkcidfm.ps\042");

    siStartInfo.cb = sizeof(STARTUPINFO);
    siStartInfo.lpReserved = NULL;
    siStartInfo.lpDesktop = NULL;
    siStartInfo.lpTitle = NULL;  /* use executable name as title */
    siStartInfo.dwX = siStartInfo.dwY = CW_USEDEFAULT;		/* ignored */
    siStartInfo.dwXSize = siStartInfo.dwYSize = CW_USEDEFAULT;	/* ignored */
    siStartInfo.dwXCountChars = 80;
    siStartInfo.dwYCountChars = 25;
    siStartInfo.dwFillAttribute = 0;			/* ignored */
    siStartInfo.dwFlags = STARTF_USESHOWWINDOW;
    siStartInfo.wShowWindow = SW_HIDE;
    siStartInfo.cbReserved2 = 0;
    siStartInfo.lpReserved2 = NULL;
    siStartInfo.hStdInput = NULL;
    siStartInfo.hStdOutput = NULL;
    siStartInfo.hStdError = NULL;

    /* Create the child process. */
    if (!CreateProcess(NULL,
        (char *)buf,  /* command line                       */
        NULL,          /* process security attributes        */
        NULL,          /* primary thread security attributes */
        FALSE,         /* handles are not inherited          */
        0,             /* creation flags                     */
        NULL,          /* environment                        */
        NULL,          /* use parent's current directory     */
        &siStartInfo,  /* STARTUPINFO pointer                */
        &piProcInfo))  /* receives PROCESS_INFORMATION  */
	    return FALSE;

    /* We don't care if ghostscript fails, so just return */

    CloseHandle(piProcInfo.hProcess);
    CloseHandle(piProcInfo.hThread);

    return TRUE;
}


//////////////////////////////////////////////////////////////////////
// Create file list
//////////////////////////////////////////////////////////////////////

FILE *fList;

typedef int (*PFN_dodir)(const char *name);

/* Called once for each directory */
int
dodir(const char *filename)
{
    return 0;
}

/* Called once for each file */
int
dofile(const char *filename)
{
    if (fList != (FILE *)NULL) {
		fputs(filename, fList);
		fputs("\n", fList);
    }
	
    return 0;
}


/* Walk through directory 'path', calling dodir() for given directory
 * and dofile() for each file.
 * If recurse=1, recurse into subdirectories, calling dodir() for
 * each directory.
 */
int 
dirwalk(char *path, int recurse, PFN_dodir dodir, PFN_dodir dofile)
{    
	WIN32_FIND_DATA find_data;
	HANDLE find_handle;
	char pattern[MAXSTR];	/* orig pattern + modified pattern */
	char base[MAXSTR];
	char name[MAXSTR];
	BOOL bMore = TRUE;
	char *p;
	
	
	if (path) {
		strcpy(pattern, path);
		if (strlen(pattern) != 0)  {
			p = pattern + strlen(pattern) -1;
			if (*p == '\\')
				*p = '\0';		// truncate trailing backslash
		}
		
		strcpy(base, pattern);
		if (strchr(base, '*') != NULL) {
			// wildcard already included
			// truncate it from the base path
			if ( (p = strrchr(base, '\\')) != NULL )
				*(++p) = '\0';
		}
		else if (isalpha(pattern[0]) && 
			pattern[1]==':' && pattern[2]=='\0')  {
			strcat(pattern, "\\*");		// search entire disk
			strcat(base, "\\");
		}
		else {
			// wildcard NOT included
			// check to see if path is a directory
			find_handle = FindFirstFile(pattern, &find_data);
			if (find_handle != INVALID_HANDLE_VALUE) {
				FindClose(find_handle);
				if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
					strcat(pattern, "\\*");		// yes, search files 
					strcat(base, "\\");
				}
				else {
					dofile(path);				// no, return just this file
					return 0;
				}
			}
			else
				return 1;	// path invalid
		}
	}
	else {
		base[0] = '\0';
		strcpy(pattern, "*");
	}
	
	find_handle = FindFirstFile(pattern,  &find_data);
	if (find_handle == INVALID_HANDLE_VALUE)
		return 1;
	
	while (bMore) {
		strcpy(name, base);
		strcat(name, find_data.cFileName);
		if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			if ( strcmp(find_data.cFileName, ".") && 
				strcmp(find_data.cFileName, "..") ) {
				dodir(name);
				if (recurse)
					dirwalk(name, recurse, dodir, dofile);
			}
		}
		else {
			dofile(name);
		}
		bMore = FindNextFile(find_handle, &find_data);
	}
	FindClose(find_handle);
	
	return 0;
}



// This is used when creating a file list.

BOOL make_filelist(int argc, char *argv[])
{
    char *title = NULL;
    char *dir = NULL;
    char *list = NULL;
    int i;
    g_bBatch = TRUE;	// Don't run message loop
	
    for (i=1; i<argc; i++) {
		if (strcmp(argv[i], "-title") == 0) {
			i++;
			title = argv[i];
		}
		else if (strcmp(argv[i], "-dir") == 0) {
			i++;
			dir = argv[i];
		}
		else if (strcmp(argv[i], "-list") == 0) {
			i++;
			list = argv[i];
		}
		else {
		    if ((title == NULL) || (strlen(title) == 0) ||
			(dir == NULL) || (strlen(dir) == 0) ||
			(list == NULL) || (strlen(list) == 0)) {
			message_box("Usage: setupgs -title \042AFPL Ghostscript #.##\042 -dir \042gs#.##\042 -list \042filelist.txt\042 spec1 spec2 specn\n");
			return FALSE;
		    }
		    if (fList == (FILE *)NULL) {
			    if ( (fList = fopen(list, "w")) == (FILE *)NULL ) {
					message_box("Can't write list file\n");
					return FALSE;
			    }
			    fputs(title, fList);
			    fputs("\n", fList);
			    fputs(dir, fList);
			    fputs("\n", fList);
		    }
		    if (argv[i][0] == '@') {
			// Use @filename with list of files/directories
			// to avoid DOS command line limit
			FILE *f;
			char buf[MAXSTR];
			int j;
			if ( (f = fopen(&(argv[i][1]), "r")) != (FILE *)NULL) {
			    while (fgets(buf, sizeof(buf), f)) {
				// remove trailing newline and spaces
				while ( ((j = strlen(buf)-1) >= 0) &&
				    ((buf[j] == '\n') || (buf[j] == ' ')) )
				    buf[j] = '\0';
			        dirwalk(buf, TRUE, &dodir, &dofile);
			    }
			    fclose(f);
			}
			else {
				wsprintf(buf, "Can't open @ file \042%s\042",
				    &argv[i][1]);
				message_box(buf);
			}
		    }
		    else
		        dirwalk(argv[i], TRUE, &dodir, &dofile);
		}
    }
	
    if (fList != (FILE *)NULL) {
        fclose(fList);
	fList = NULL;
    }
    return TRUE;
}

//////////////////////////////////////////////////////////////////////

#ifndef CSIDL_PROGRAM_FILES
#define CSIDL_PROGRAM_FILES 0x0026
#endif
#ifndef CSIDL_FLAG_CREATE
#define CSIDL_FLAG_CREATE 0x8000
#endif
#ifndef SHGFP_TYPE_CURRENT
#define SHGFP_TYPE_CURRENT 0
#endif

BOOL 
GetProgramFiles(LPTSTR path) 
{
    PFN_SHGetSpecialFolderPath PSHGetSpecialFolderPath = NULL;
    PFN_SHGetFolderPath PSHGetFolderPath = NULL;
    HMODULE hModuleShell32 = NULL;
    HMODULE hModuleShfolder = NULL;
    BOOL fOk = FALSE;
    hModuleShfolder = LoadLibrary("shfolder.dll");
    hModuleShell32 = LoadLibrary("shell32.dll");

    if (hModuleShfolder) {
	PSHGetFolderPath = (PFN_SHGetFolderPath)
	    GetProcAddress(hModuleShfolder, "SHGetFolderPathA");
	if (PSHGetFolderPath) {
	    fOk = (PSHGetFolderPath(HWND_DESKTOP, 
		CSIDL_PROGRAM_FILES | CSIDL_FLAG_CREATE, 
		NULL, SHGFP_TYPE_CURRENT, path) == S_OK);
	}
    }

    if (!fOk && hModuleShell32) {
	PSHGetFolderPath = (PFN_SHGetFolderPath)
	    GetProcAddress(hModuleShell32, "SHGetFolderPathA");
	if (PSHGetFolderPath) {
	    fOk = (PSHGetFolderPath(HWND_DESKTOP, 
		CSIDL_PROGRAM_FILES | CSIDL_FLAG_CREATE, 
		NULL, SHGFP_TYPE_CURRENT, path) == S_OK);
	}
    }

    if (!fOk && hModuleShell32) {
	PSHGetSpecialFolderPath = (PFN_SHGetSpecialFolderPath)
	    GetProcAddress(hModuleShell32, "SHGetSpecialFolderPathA");
	if (PSHGetSpecialFolderPath) {
	    fOk = PSHGetSpecialFolderPath(HWND_DESKTOP, path,
		CSIDL_PROGRAM_FILES, TRUE);
	}
    }

    if (!fOk) {
	/* If all else fails (probably Win95), try the registry */
	LONG rc;
	HKEY hkey;
	DWORD cbData;
	DWORD keytype;
	rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
	    "SOFTWARE\\Microsoft\\Windows\\CurrentVersion", 0, KEY_READ, &hkey);
	if (rc == ERROR_SUCCESS) {
	    cbData = MAX_PATH;
	    keytype =  REG_SZ;
	    if (rc == ERROR_SUCCESS)
		rc = RegQueryValueEx(hkey, "ProgramFilesDir", 0, &keytype, 
		    (LPBYTE)path, &cbData);
	    RegCloseKey(hkey);
	}
	fOk = (rc == ERROR_SUCCESS);
    }
    return fOk;
}


//////////////////////////////////////////////////////////////////////
