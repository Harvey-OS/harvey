/* Copyright (C) 1999-2002, Ghostgum Software Pty Ltd.  All rights reserved.
  
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

// $Id: dwinst.h,v 1.7 2004/11/18 06:48:41 ghostgum Exp $

#ifndef dwinst_INCLUDED
#  define dwinst_INCLUDED

// Definitions for Ghostscript installer

#ifndef MAXSTR
#ifdef MAX_PATH
#define MAXSTR MAX_PATH
#else
#define MAXSTR 256
#endif
#endif

class CInstall  
{
public:
	CInstall();
	virtual ~CInstall();
	void SetMessageFunction(void(*fn)(const char *));
	void AddMessage(const char *message);
	const char *GetMainDir();
	const char *GetUninstallName();
	BOOL GetPrograms(BOOL bUseCommon, char *buf, int buflen);
	BOOL Init(const char *szSourceDir, const char *szFileList);
	BOOL InstallFiles(BOOL bNoCopy, BOOL *pbQuit);
	BOOL InstallFile(char *filename, BOOL bNoCopy);
	BOOL MakeDir(const char *dirname);
	FILE * MakeTemp(char *name);

	BOOL SetAllUsers(BOOL bUseCommon);
	void SetTargetDir(const char *szTargetDir);
	void SetTargetGroup(const char *szTargetGroup);

	BOOL StartMenuBegin();
	BOOL StartMenuEnd();
	BOOL StartMenuAdd(const char *szDescription, const char *szProgram, const char *szArguments);

	BOOL UpdateRegistryBegin();
	BOOL UpdateRegistryKey(const char *product, const char *version);
	BOOL UpdateRegistryValue(const char *product, const char *version, const char *name, const char *value);
	BOOL UpdateRegistryEnd();

	BOOL WriteUninstall(const char *prog, BOOL bNoCopy);
	BOOL MakeLog(void);

	void CleanUp(void);

        void AppendFileNew(const char *filename);

private:
	BOOL m_bNoCopy;
	BOOL m_bUseCommon;
	BOOL m_bQuit;

	// Source directory
	char m_szSourceDir[MAXSTR];

	// File containing list of files to install
	char m_szFileList[MAXSTR];

	// Target directory for program and fonts.
	char m_szTargetDir[MAXSTR];

	// Target Group for shortcut
	char m_szTargetGroup[MAXSTR];

	// Directory where the Start Menu is located.
	char m_szPrograms[MAXSTR];

	// Name used for uninstall
	char m_szUninstallName[MAXSTR];

	// Main directory prefix, where log files should be written
	char m_szMainDir[MAXSTR];

	// Full directory where log files should be written
	char m_szLogDir[MAXSTR];

	// Temporary log files for uninstall
	char m_szFileNew[MAXSTR];
	char m_szRegistryNew[MAXSTR];
	char m_szRegistryOld[MAXSTR];
	char m_szShellNew[MAXSTR];
	char m_szShellOld[MAXSTR];

	// Log files
	FILE * m_fLogNew;
	FILE * m_fLogOld;


	BOOL SetRegistryValue(HKEY hkey, const char *value_name, const char *value);
	BOOL CreateShellLink(LPCSTR description, LPCSTR program, LPCSTR arguments, LPCSTR icon = NULL, int nIconIndex = 0);
	void CopyFileContents(FILE *df, FILE *sf);
	void ResetReadonly(const char *filename);

	void(*AddMessageFn)(const char *);

};


#endif /* dwinst_INCLUDED */
