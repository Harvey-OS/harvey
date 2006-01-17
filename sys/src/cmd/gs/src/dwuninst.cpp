/* Copyright (C) 1999, Ghostgum Software Pty Ltd.  All rights reserved.
  
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

// $Id: dwuninst.cpp,v 1.6 2005/03/04 21:58:55 ghostgum Exp $

#define STRICT
#include <windows.h>
#include <objbase.h>
#include <shlobj.h>
#include <shellapi.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <direct.h>
#include "dwuninst.h"


#ifdef _MSC_VER
#define _export
#define chdir(x) _chdir(x)
#define mkdir(x) _mkdir(x)
#endif
#define DELAY_STEP 500
#define DELAY_FILE 0
#define MAXSTR 256
#define UNINSTALLKEY TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall")

#ifdef _WIN64
#define DLGRETURN INT_PTR
#else
#define DLGRETURN BOOL
#endif


HWND hDlgModeless;
HWND hText1;
HWND hText2;
char path[MAXSTR];
int language = 0;
BOOL is_win4 = FALSE;
HINSTANCE phInstance;
char szSection[] = "////////////////////////////////";
BOOL bQuit = FALSE;
BOOL gError = FALSE;	// set TRUE if an uninstall was not successful 

char szTitle[MAXSTR];
char szLogFile[MAXSTR];
char szLine[MAXSTR];
FILE *fLog;

void do_message(void);
BOOL dofiles(void);
BOOL registry_delete(void);
BOOL registry_import(void);
BOOL shell_new(void);
BOOL shell_old(void);
BOOL doEOF(void);

// #define gs_addmess(str) fputs(str, stdout)	// for debug 
#define gs_addmess(str)


// linked list for deleting registry entries in reverse order
typedef struct tagKEY {
    long index;
    struct tagKEY *previous;
} KEY;
KEY *last_key = NULL;


// read a line from the log, removing trailing new line character
BOOL GetLine(void)
{
    BOOL err = TRUE;
    int i;
    szLine[0] = '\0';
    if (fLog)
        err = (fgets(szLine, sizeof(szLine)-1, fLog) == NULL);
    i = strlen(szLine) - 1;
    if ( (szLine[0] != '\0') && (szLine[i] == '\n'))
		szLine[i] = '\0';
    return !err;
}

BOOL IsSection(void)
{
    return (strncmp(szLine, szSection, strlen(szSection)) == 0);
}

BOOL
NextSection(void)
{
    while (GetLine()) {
		do_message();
		if (bQuit)
			return FALSE;
		if (IsSection())
			return TRUE;
	}

	return TRUE;
}

BOOL ReadSection(void)
{
	do_message();
	if (bQuit)
		return FALSE;
	GetLine();
	if (strlen(szLine) == 0) {
		doEOF();
		return TRUE;
	}
	else if (strcmp(szLine, "FileNew")==0) {
		SetWindowText(hText1, "Removing Files");
		Sleep(DELAY_STEP);
		if (!dofiles())
			return FALSE;
		SetWindowText(hText1, "");
		return TRUE;
	}
	else if (strcmp(szLine, "RegistryNew")==0) {
		SetWindowText(hText1, "Removing Registry entries");
		Sleep(DELAY_STEP);
		if (!registry_delete())
			return FALSE;
		SetWindowText(hText1, "");
		return TRUE;
	}
	else if (strcmp(szLine, "RegistryOld")==0) {
		SetWindowText(hText1, "Restoring Registry entries");
		Sleep(DELAY_STEP);
		if (!registry_import())
			return FALSE;
		SetWindowText(hText1, "");
		return TRUE;
	}
	else if (strcmp(szLine, "ShellNew")==0) {
		SetWindowText(hText1, "Removing Start Menu items");
		Sleep(DELAY_STEP);
		if (!shell_new())
			return FALSE;
		SetWindowText(hText1, "");
		return TRUE;
	}
	else if (strcmp(szLine, "ShellOld")==0) {
		SetWindowText(hText1, "Restoring Start Menu items");
		Sleep(DELAY_STEP);
		if (!shell_old())
			return FALSE;
		SetWindowText(hText1, "");
		return TRUE;
	}
	return FALSE;
}


BOOL
dofiles(void)
{
    while (GetLine()) {
	do_message();
	if (bQuit)
	    return FALSE;
	if (IsSection()) {
	    SetWindowText(hText2, "");
	    return TRUE;
	}
	if (szLine[0] != '\0') {
	    SetWindowText(hText2, szLine);
	    Sleep(DELAY_FILE);
	    gs_addmess("Deleting File: ");
	    gs_addmess(szLine);
	    gs_addmess("\n");
	    DeleteFile(szLine);
	}
    }
    return FALSE;
}

BOOL
doEOF(void)
{
    fclose(fLog);
    fLog = NULL;
    unlink(szLogFile);
    PostMessage(hDlgModeless, WM_COMMAND, IDC_DONE, 0L);
	bQuit = TRUE;
    return TRUE;
}


BOOL
registry_delete_key(void)
{
char keyname[MAXSTR];
HKEY hkey = HKEY_CLASSES_ROOT;
HKEY hrkey = HKEY_CLASSES_ROOT;
char *rkey, *skey;
char *name;
DWORD dwResult;
    keyname[0] = '\0';
    while (GetLine()) {
	if ((szLine[0] == '\0') || (szLine[0] == '\r') || (szLine[0] == '\n'))
	    break;
	if (szLine[0] == '[') {
	    // key name
	    rkey = strtok(szLine+1, "\\]\n\r");
	    if (rkey == (char *)NULL)
		return FALSE;
	    skey = strtok(NULL, "]\n\r");
	    if (strcmp(rkey, "HKEY_CLASSES_ROOT")==0)
		hrkey = HKEY_CLASSES_ROOT;
	    else if (strcmp(rkey, "HKEY_CURRENT_USER")==0)
		hrkey = HKEY_CURRENT_USER;
	    else if (strcmp(rkey, "HKEY_LOCAL_MACHINE")==0)
		hrkey = HKEY_LOCAL_MACHINE;
	    else if (strcmp(rkey, "HKEY_USERS")==0)
		hrkey = HKEY_USERS;
	    else
		return FALSE;
	    if (skey == (char *)NULL)
		return FALSE;
	    gs_addmess("Opening registry key\n   ");
	    gs_addmess(rkey);
	    gs_addmess("\\");
	    gs_addmess(skey);
	    gs_addmess("\n");
	    if (RegCreateKeyEx(hrkey, skey, 0, "", 0, KEY_ALL_ACCESS, 
		NULL, &hkey, &dwResult)
		!= ERROR_SUCCESS)
		return FALSE;
	    strcpy(keyname, skey);
	}
	else if (szLine[0] == '@') {
	    // default value
	    RegDeleteValue(hkey, NULL);
	    gs_addmess("Deleting registry default value\n");
	}
	else if (szLine[0] == '\042') {
	    // named value
	    name = strtok(szLine+1, "\042\r\n");
	    RegDeleteValue(hkey, name);
	    gs_addmess("Deleting registry named value\n   ");
	    gs_addmess(name);
	    gs_addmess("\n");
	}
    }
    // close key
    if (hkey != HKEY_CLASSES_ROOT)
	RegCloseKey(hkey);
    // delete the key
    if (strlen(keyname)) {
	gs_addmess("Deleting registry key\n   ");
	gs_addmess(keyname);
	gs_addmess("\n");
	RegOpenKeyEx(hrkey, NULL, 0, 0, &hkey);
	RegDeleteKey(hkey, keyname);
	RegCloseKey(hkey);
    }
    return TRUE;
}

BOOL
registry_delete()
{
	long logindex;
	KEY *key;
	
    // scan log file
    // so we can remove keys in reverse order
    logindex = 0;
    while (GetLine() && !IsSection()) {
		KEY *key;
		if (szLine[0] == '[') {
			if ((key = (KEY *)malloc(sizeof(KEY)))
				!= (KEY *)NULL) {
				key->previous = last_key;
				key->index = logindex;
				last_key = key;
			}
		}
		logindex = ftell(fLog);
    }
	
    // Remove keys
    for (key = last_key; key != NULL; 
	key = key->previous) {
		if (key != last_key)
			free(last_key);
		fseek(fLog, key->index, SEEK_SET);
		registry_delete_key();
		last_key = key;
    }
    free(last_key);
	
    fseek(fLog, logindex, SEEK_SET);
	GetLine();
    return TRUE;
}



void
registry_unquote(char *line)
{
char *s, *d;
int value;
    s = d = line;
    while (*s) {
	if (*s != '\\') {
	    *d++ = *s;
	}
	else {
	    s++;
	    if (*s == '\\')
		*d++ = *s;
	    else {
		value = 0;
		if (*s) {
		    value = *s++ - '0';
		}
		if (*s) {
		    value <<= 3;
		    value += *s++ - '0';
		}
		if (*s) {
		    value <<= 3;
		    value += *s - '0';
		}
		*d++ = (char)value;
	    }
	}
	s++;
    }
    *d = '\0';
}

BOOL
registry_import()
{
	HKEY hkey = HKEY_CLASSES_ROOT;
	HKEY hrkey;
	char *rkey, *skey;
	char *value;
	char *name;
	DWORD dwResult;
    GetLine();
    if (strncmp(szLine, "REGEDIT4", 8) != 0)
		return FALSE;
	
    while (GetLine()) {
		if (IsSection())
			break;
		if ((szLine[0] == '\0') || (szLine[0] == '\r') || (szLine[0] == '\n'))
			continue;
		if (szLine[0] == '[') {
			// key name
			if (hkey != HKEY_CLASSES_ROOT) {
				RegCloseKey(hkey);
				hkey = HKEY_CLASSES_ROOT;
			}
			rkey = strtok(szLine+1, "\\]\n\r");
			if (rkey == (char *)NULL)
				return FALSE;
			skey = strtok(NULL, "]\n\r");
			if (strcmp(rkey, "HKEY_CLASSES_ROOT")==0)
				hrkey = HKEY_CLASSES_ROOT;
			else if (strcmp(rkey, "HKEY_CURRENT_USER")==0)
				hrkey = HKEY_CURRENT_USER;
			else if (strcmp(rkey, "HKEY_LOCAL_MACHINE")==0)
				hrkey = HKEY_LOCAL_MACHINE;
			else if (strcmp(rkey, "HKEY_USERS")==0)
				hrkey = HKEY_USERS;
			else
				return FALSE;
			if (skey == (char *)NULL)
				return FALSE;
			gs_addmess("Creating registry key\n   ");
			gs_addmess(rkey);
			gs_addmess("\\");
			gs_addmess("skey");
			gs_addmess("\n");
			if (RegCreateKeyEx(hrkey, skey, 0, "", 0, KEY_ALL_ACCESS, 
				NULL, &hkey, &dwResult)
				!= ERROR_SUCCESS)
				return FALSE;
		}
		else if (szLine[0] == '@') {
			// default value
			if (strlen(szLine) < 4)
				return FALSE;
			value = strtok(szLine+3, "\042\r\n");
			if (value) {
				registry_unquote(value);
				gs_addmess("Setting registry key value\n   ");
				gs_addmess(value);
				gs_addmess("\n");
				if (RegSetValueEx(hkey, NULL, 0, REG_SZ, 
					(CONST BYTE *)value, strlen(value)+1)
					!= ERROR_SUCCESS)
					return FALSE;
			}
		}
		else if (szLine[0] == '\042') {
			// named value
			name = strtok(szLine+1, "\042\r\n");
			strtok(NULL, "\042\r\n");
			value = strtok(NULL, "\042\r\n");
			registry_unquote(value);
			gs_addmess("Setting registry key value\n   ");
			gs_addmess(name);
			gs_addmess("=");
			gs_addmess(value);
			gs_addmess("\n");
			if (RegSetValueEx(hkey, name, 0, REG_SZ, (CONST BYTE *)value, strlen(value)+1)
				!= ERROR_SUCCESS)
				return FALSE;
		}
    }
    if (hkey != HKEY_CLASSES_ROOT)
		RegCloseKey(hkey);
    return TRUE;
}

// recursive mkdir
// requires a full path to be specified, so ignores root \ 
// apart from root \, must not contain trailing \ 
// Examples:
//  c:\          (OK, but useless)
//  c:\gstools   (OK)
//  c:\gstools\  (incorrect)
//  c:gstools    (incorrect)
//  gstools      (incorrect)
// The following UNC names should work,
// but didn't under Win3.1 because gs_chdir wouldn't accept UNC names
// Needs to be tested under Windows 95.
//  \\server\sharename\gstools    (OK)
//  \\server\sharename\           (OK, but useless)
//

BOOL MakeDir(char *dirname)
{
char newdir[MAXSTR];
char *p;
    if (strlen(dirname) < 3)
        return -1;

    gs_addmess("Making Directory\n  ");
    gs_addmess(dirname);
    gs_addmess("\n");
    if (isalpha(dirname[0]) && dirname[1]==':' && dirname[2]=='\\') {
        // drive mapped path
        p = dirname+3;
    }
    else if (dirname[1]=='\\' && dirname[1]=='\\') {
        // UNC path
        p = strchr(dirname+2, '\\');    // skip servername
        if (p == NULL)
            return -1;
        p++;
        p = strchr(p, '\\');            // skip sharename
        if (p == NULL)
            return -1;
    }
    else {
        // not full path so error
        return -1;
    }

    while (1) {
        strncpy(newdir, dirname, (int)(p-dirname));
        newdir[(int)(p-dirname)] = '\0';
        if (chdir(newdir)) {
            if (mkdir(newdir))
                return -1;
        }
        p++;
        if (p >= dirname + strlen(dirname))
            break;              // all done
        p = strchr(p, '\\');
        if (p == NULL)
            p = dirname + strlen(dirname);
    }

    return SetCurrentDirectory(dirname);
}


BOOL shell_new(void)
{

	char *p, *q;
	char group[MAXSTR];
	// remove shell items added by Ghostscript
	// We can only delete one group with this code
	group[0] = '\0';
	while (GetLine()) {
		if (IsSection()) {
			if (strlen(group) != 0) {
				gs_addmess("Removing shell folder\n  ");
				gs_addmess(group);
				gs_addmess("\n");
				RemoveDirectory(group);
			}
			return TRUE;
		}
		p = strtok(szLine, "=");
		q = strtok(NULL, "");
		if (p == NULL) {
			continue;
		}
		else if (strcmp(p, "Group")==0) {
			if (q)
				strncpy(group, q, sizeof(group)-1);
			// defer this until we have remove contents
		}
		else if (strcmp(p, "Name") == 0) {
			if (q) {
				gs_addmess("Removing shell link\n  ");
				gs_addmess(q);
				gs_addmess("\n");
				DeleteFile(q);
			}
		}
	}

	return TRUE;
}


BOOL CreateShellLink(LPCSTR name, LPCSTR description, LPCSTR program, 
	LPCSTR arguments, LPCSTR directory, LPCSTR icon, int nIconIndex)
{
	HRESULT hres;    
	IShellLink* psl;

	// Ensure string is UNICODE.
	WCHAR wsz[MAX_PATH];
	MultiByteToWideChar(CP_ACP, 0, name, -1, wsz, MAX_PATH);

	// Save new shell link

	// Get a pointer to the IShellLink interface.
	hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
		IID_IShellLink, (void **)&psl);
	if (SUCCEEDED(hres))    {
		IPersistFile* ppf;
		// Query IShellLink for the IPersistFile interface for 
		// saving the shell link in persistent storage.
		hres = psl->QueryInterface(IID_IPersistFile, (void **)&ppf);
		if (SUCCEEDED(hres)) {            
			gs_addmess("Adding shell link\n  ");
			gs_addmess(name);
			gs_addmess("\n");

			// Set the path to the shell link target.
			hres = psl->SetPath(program);         
			if (!SUCCEEDED(hres)) {
				gs_addmess("SetPath failed!");
				gError = TRUE;
			}
			// Set the description of the shell link.
			hres = psl->SetDescription(description);         
			if (!SUCCEEDED(hres)) {
				gs_addmess("SetDescription failed!");
				gError = TRUE;
			}
			if ((arguments != (LPCSTR)NULL) && *arguments) {
				// Set the arguments of the shell link target.
				hres = psl->SetArguments(arguments);         
				if (!SUCCEEDED(hres)) {
					gs_addmess("SetArguments failed!");
					gError = TRUE;
				}
			}
			if ((directory != (LPCSTR)NULL) && *directory) {
				// Set the arguments of the shell link target.
				hres = psl->SetWorkingDirectory(directory);         
				if (!SUCCEEDED(hres)) {
					gs_addmess("SetWorkingDirectory failed!");
					gError = TRUE;
				}
			}
			if ((icon != (LPCSTR)NULL) && *icon) {
				// Set the arguments of the shell link target.
				hres = psl->SetIconLocation(icon, nIconIndex);         
				if (!SUCCEEDED(hres)) {
					gs_addmess("SetIconLocation failed!");
					gError = TRUE;
				}
			}

			// Save the link via the IPersistFile::Save method.
			hres = ppf->Save(wsz, TRUE);    
			// Release pointer to IPersistFile.         
			ppf->Release();
		}
		// Release pointer to IShellLink.       
		psl->Release();    
	}

	return (hres == 0);
}



BOOL shell_old(void)
{
	// Add shell items removed by Ghostscript
	char *p, *q;
	char name[MAXSTR];
	char description[MAXSTR];
	char program[MAXSTR];
	char arguments[MAXSTR];
	char directory[MAXSTR];
	char icon[MAXSTR];
	int nIconIndex;
	// Remove shell items added by Ghostscript
	name[0] = description[0] = program[0] = arguments[0] 
		= directory[0] = icon[0] = '\0';
	nIconIndex = 0;
	
	while (GetLine()) {
		if (IsSection())
			return TRUE;
		p = strtok(szLine, "=");
		q = strtok(NULL, "");
		if (strlen(szLine) == 0) {
			if (name[0] != '\0') {
				// add start menu item
				CreateShellLink(name, description, program, arguments, 
					directory, icon, nIconIndex);
			}
			name[0] = description[0] = program[0] = arguments[0] 
				= directory[0] = icon[0] = '\0';
			nIconIndex = 0;
			continue;
		}
		else if (p == (char *)NULL) {
			continue;
		}
		else if (strcmp(p, "Group")==0) {
			MakeDir(q);
		}
		else if (strcmp(p, "Name") == 0)
			strncpy(name, q, sizeof(name)-1);
		else if (strcmp(p, "Description") == 0)
			strncpy(description, q, sizeof(description)-1);
		else if (strcmp(p, "Program") == 0)
			strncpy(program, q, sizeof(program)-1);
		else if (strcmp(p, "Arguments") == 0)
			strncpy(arguments, q, sizeof(arguments)-1);
		else if (strcmp(p, "Directory") == 0)
			strncpy(directory, q, sizeof(directory)-1);
		else if (strcmp(p, "IconLocation") == 0)
			strncpy(icon, q, sizeof(icon)-1);
		else if (strcmp(p, "IconIndex") == 0)
			nIconIndex = atoi(q);
	}
	
	return TRUE;
}



#ifdef __BORLANDC__
#pragma argsused
#endif
DLGRETURN CALLBACK _export
RemoveDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch(message) {
    case WM_INITDIALOG:
	    SetWindowText(hwnd, szTitle);
	    return TRUE;
	case WM_COMMAND:
	    switch(LOWORD(wParam)) {
		case IDC_DONE:
		    // delete registry entries for uninstall
			if (is_win4) {
				HKEY hkey;
				if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
					UNINSTALLKEY, 0, KEY_ALL_ACCESS, &hkey) 
					== ERROR_SUCCESS) {
					RegDeleteKey(hkey, szTitle);
					RegCloseKey(hkey);
				}
			}

		    SetWindowText(hText1, "Uninstall successful");
		    SetWindowText(hText2, "");
		    EnableWindow(GetDlgItem(hwnd, IDOK), FALSE);
		    EnableWindow(GetDlgItem(hwnd, IDCANCEL), TRUE);
		    SetDlgItemText(hwnd, IDCANCEL, "Exit");
		    SetFocus(GetDlgItem(hwnd, IDCANCEL));
		    return TRUE;
		case IDOK:
		    // Start removal
		    EnableWindow(GetDlgItem(hwnd, IDOK), FALSE);
			EnableWindow(GetDlgItem(hwnd, IDC_PRESSOK), FALSE);
		    while (!bQuit) {
			do_message();
			if (!ReadSection()) {
			    SetWindowText(hText1, "Uninstall FAILED");
			    SetWindowText(hText2, "");
			    EnableWindow(GetDlgItem(hwnd, IDOK), FALSE);
			    EnableWindow(GetDlgItem(hwnd, IDCANCEL), TRUE);
			    SetDlgItemText(hwnd, IDCANCEL, "Exit");
			    SetFocus(GetDlgItem(hwnd, IDCANCEL));
				bQuit = TRUE;
			}
		    }
		    return TRUE;
		case IDCANCEL:
		    bQuit = TRUE;
		    DestroyWindow(hwnd);
		    hDlgModeless = 0;
		    return TRUE;
	    }
	case WM_CLOSE:
	    DestroyWindow(hwnd);
	    hDlgModeless = 0;
	    return TRUE;
    }
    return FALSE;
}

void
do_message(void)
{
MSG msg;
    while (hDlgModeless && PeekMessage(&msg, (HWND)NULL, 0, 0, PM_REMOVE)) {
	if ((hDlgModeless == 0) || !IsDialogMessage(hDlgModeless, &msg)) {
	    TranslateMessage(&msg);
	    DispatchMessage(&msg);
	}
    }
}



BOOL
init(void)
{
	DWORD version = GetVersion();
	char *p, *s;
    // get location of uninstall log from command line as argv[1]
    p = GetCommandLine();
	s = p;
	if (*s == '\042') {
		// skip over program name
		s++;
		while (*s && *s!='\042')
			s++;
		if (*s)
			s++;
	}
	else if (*s != ' ') {
		// skip over program name
		s++;
		while (*s && *s!=' ')
			s++;
		if (*s)
			s++;
	}
	while (*s && *s==' ')
		s++;
	if (*s == '\042')
		s++;
	strncpy(szLogFile, s, sizeof(szLogFile));
	s = szLogFile;
	while (*s) {
		if (*s == '\042') {
			*s = '\0';
			break;
		}
		s++;
	}
	if (strlen(szLogFile) == 0) {
		MessageBox(HWND_DESKTOP, "Usage: uninstgs logfile.txt", 
			"AFPL Ghostscript Uninstall", MB_OK);
		return FALSE;
	}
	
	// read first few lines of file to get title
	fLog = fopen(szLogFile, "r");
	if (fLog == (FILE *)NULL) {
		MessageBox(HWND_DESKTOP, szLogFile, "Can't find file", MB_OK);
		return FALSE;
	}
	GetLine();
	if (!IsSection()) {
		MessageBox(HWND_DESKTOP, szLogFile, "Not valid uninstall log", 
			MB_OK);
		return FALSE;
	}
	GetLine();
	if (strcmp(szLine, "UninstallName") != 0) {
		MessageBox(HWND_DESKTOP, szLogFile, "Not valid uninstall log", 
			MB_OK);
		return FALSE;
	}
	GetLine();
	strcpy(szTitle, szLine);
	
	NextSection();
	
	if (LOBYTE(LOWORD(version)) >= 4)
		is_win4 = TRUE;
	return TRUE;
}

#ifdef __BORLANDC__
#pragma argsused
#endif
int PASCAL 
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int cmdShow)
{
MSG msg;

    phInstance = hInstance;
    if (!init())
	return 1;


    CoInitialize(NULL);

    hDlgModeless = CreateDialogParam(hInstance, 
	    MAKEINTRESOURCE(IDD_UNSET),
	    HWND_DESKTOP, RemoveDlgProc, (LPARAM)NULL);
    hText1 = GetDlgItem(hDlgModeless, IDC_T1);
    hText2 = GetDlgItem(hDlgModeless, IDC_T2);

    SetWindowPos(hDlgModeless, HWND_TOP, 0, 0, 0, 0, 
	SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE);

    while (hDlgModeless && GetMessage(&msg, (HWND)NULL, 0, 0)) {
	if ((hDlgModeless == 0) || !IsDialogMessage(hDlgModeless, &msg)) {
	    TranslateMessage(&msg);
	    DispatchMessage(&msg);
	}
    }

	if (fLog)
		fclose(fLog);
	
	CoUninitialize();

    return 0;
}

