/* Copyright (C) 1996, 1998, Russell Lang.  All rights reserved.
  
  This file is part of Aladdin Ghostscript.
  
  Aladdin Ghostscript is distributed with NO WARRANTY OF ANY KIND.  No author
  or distributor accepts any responsibility for the consequences of using it,
  or for whether it serves any particular purpose or works at all, unless he
  or she says so in writing.  Refer to the Aladdin Ghostscript Free Public
  License (the "License") for full details.
  
  Every copy of Aladdin Ghostscript must include a copy of the License,
  normally in a plain ASCII text file named PUBLIC.  The License grants you
  the right to copy, modify and redistribute Aladdin Ghostscript, but only
  under certain conditions described in the License.  Among other things, the
  License requires that the copyright notice and this notice be preserved on
  all copies.
*/


// $Id: dwdll.cpp,v 1.1 2000/03/09 08:40:40 lpd Exp $

// gsdll class  for MS-Windows

#define STRICT
#include <windows.h>
#include <string.h>
#include <stdio.h>

extern "C" {
#include "stdpre.h"
#undef public
#include "gpgetenv.h"
#include "gsdll.h"
#include "gsdllwin.h"
#include "gscdefs.h"
}

#include "dwdll.h"   // gsdll_class

static char not_loaded[] = "DLL is not loaded";
static char func_null[]  = "A function pointer to the DLL is NULL";
static char not_init[] = "Not initialized";

int
gsdll_class::load(const HINSTANCE in_hinstance, 
	const char *name, const long need_version)
{
char fullname[1024];
const char *shortname;
char *p;
long version;
int len;

    // Don't load if already loaded
    if (hmodule)
	return 0;
    hinstance = in_hinstance;
    initialized = FALSE;
  

    // First try to load DLL with name in registry or environment variable
    hmodule = (HINSTANCE)NULL;
    len = sizeof(fullname);
    if (gp_getenv("GS_DLL", fullname, &len) == 0)
        hmodule = LoadLibrary(fullname);

    // Next try to load DLL first with given path
    if (hmodule < (HINSTANCE)HINSTANCE_ERROR)
        hmodule = LoadLibrary(name);
    if (hmodule < (HINSTANCE)HINSTANCE_ERROR) {
	// failed
	// try again, with path of EXE
	if ((shortname = strrchr((char *)name, '\\')) == (const char *)NULL)
	    shortname = name;
	GetModuleFileName(hinstance, fullname, sizeof(fullname));
	if ((p = strrchr(fullname,'\\')) != (char *)NULL)
	    p++;
	else
	    p = fullname;
	*p = '\0';
	strcat(fullname, shortname);
        hmodule = LoadLibrary(fullname);
        if (hmodule < (HINSTANCE)HINSTANCE_ERROR) {
	    // failed again
	    // try once more, this time on system search path
            hmodule = LoadLibrary(shortname);
            if (hmodule < (HINSTANCE)HINSTANCE_ERROR) {
		sprintf(last_error, "Couldn't load DLL, LoadLibrary error code %d", (int)hmodule);
		hmodule = (HINSTANCE)0;
		return 1;
	    }
	}
    }

    // DLL is now loaded
    // Get pointers to functions
    c_revision = (PFN_gsdll_revision) GetProcAddress(hmodule, "gsdll_revision");
    if (c_revision == NULL) {
	sprintf(last_error, "Can't find gsdll_revision\n");
	unload();
	return 1;
    }
    // check DLL version
    c_revision(NULL, NULL, &version, NULL);
    if (version != need_version) {
	sprintf(last_error, "Wrong version of DLL found.\n  Found version %ld\n  Need version  %ld\n", version, need_version);
	unload();
	return 1;
    }

    // continue loading other functions */
    c_init = (PFN_gsdll_init) GetProcAddress(hmodule, "gsdll_init");
    if (c_init == NULL) {
	sprintf(last_error, "Can't find gsdll_init\n");
	unload();
	return 1;
    }
    c_execute_begin = (PFN_gsdll_execute_begin) GetProcAddress(hmodule, "gsdll_execute_begin");
    if (c_execute_begin == NULL) {
	sprintf(last_error, "Can't find gsdll_execute_begin\n");
	unload();
	return 1;
    }
    c_execute_cont = (PFN_gsdll_execute_cont) GetProcAddress(hmodule, "gsdll_execute_cont");
    if (c_execute_cont == NULL) {
	sprintf(last_error, "Can't find gsdll_execute_cont\n");
	unload();
	return 1;
    }
    c_execute_end = (PFN_gsdll_execute_end) GetProcAddress(hmodule, "gsdll_execute_end");
    if (c_execute_end == NULL) {
	sprintf(last_error, "Can't find gsdll_execute_end\n");
	unload();
	return 1;
    }
    c_exit = (PFN_gsdll_exit) GetProcAddress(hmodule, "gsdll_exit");
    if (c_exit == NULL) {
	sprintf(last_error, "Can't find gsdll_exit\n");
	unload();
	return 1;
    }
    c_lock_device = (PFN_gsdll_lock_device) GetProcAddress(hmodule, "gsdll_lock_device");
    if (c_lock_device == NULL) {
	sprintf(last_error, "Can't find gsdll_lock_device\n");
	unload();
	return 1;
    }
    c_copy_dib = (PFN_gsdll_copy_dib) GetProcAddress(hmodule, "gsdll_copy_dib");
    if (c_copy_dib == NULL) {
	sprintf(last_error, "Can't find gsdll_copy_dib\n");
	unload();
	return 1;
    }
    c_copy_palette = (PFN_gsdll_copy_palette) GetProcAddress(hmodule, "gsdll_copy_palette");
    if (c_copy_palette == NULL) {
	sprintf(last_error, "Can't find gsdll_copy_palette\n");
	unload();
	return 1;
    }
    c_draw = (PFN_gsdll_draw) GetProcAddress(hmodule, "gsdll_draw");
    if (c_draw == NULL) {
	sprintf(last_error, "Can't find gsdll_draw\n");
	unload();
	return 1;
    }
    return 0;
}

int
gsdll_class::unload(void)
{
    // exit Ghostscript interpreter
    if (initialized) {
	if (!execute_code)
	    c_execute_end();
	c_exit();
#ifndef __WIN32__
	FreeProcInstance((FARPROC)callback);
#endif
        callback = NULL;
    }

    // Set functions to NULL to prevent use
    c_revision = NULL;
    c_init = NULL;
    c_execute_begin = NULL;
    c_execute_cont = NULL;
    c_execute_end = NULL;
    c_exit = NULL;
    c_lock_device = NULL;
    c_copy_dib = NULL;
    c_copy_palette = NULL;
    c_draw = NULL;

    // need to do more than this
    device = NULL;

    // Don't do anything else if already unloaded
    if (hmodule == (HINSTANCE)NULL)
	return 0;

    FreeLibrary(hmodule);
    return 0;
}

int 
gsdll_class::revision(char FAR * FAR *product, char FAR * FAR *copyright, 
	long FAR *revision, long FAR *revisiondate)
{
    if (!hmodule) {
	strcpy(last_error, not_loaded);
	return 1;
    }
    if (!c_revision)
	return c_revision(product, copyright, revision, revisiondate);
    strcpy(last_error, func_null);
    return 1;
}

int 
gsdll_class::init(GSDLL_CALLBACK in_callback, HWND hwnd, int argc, char FAR * FAR *argv)
{
int rc;
    execute_code = 0;
    if (!hmodule) {
	strcpy(last_error, not_loaded);
	return 1;
    }
    if (initialized) {
	strcpy(last_error, "Already initialized");
	return 1;
    }
    if (in_callback == (GSDLL_CALLBACK)NULL) {
	strcpy(last_error, "Callback not provided");
	return 1;
    }

#ifdef __WIN32__
    callback = in_callback;
#else
    callback = (GSDLL_CALLBACK)MakeProcInstance((FARPROC)in_callback, hinstance);
#endif

    if (c_init && c_execute_begin) {
	rc = c_init(callback, hwnd, argc, argv);
	if (rc) {
	    if (rc == GSDLL_INIT_IN_USE)
	        sprintf(last_error, "gsdll_init returns %d\nDLL already in use", rc);
	    else
	        sprintf(last_error, "gsdll_init returns %d\n", rc);
	    return rc;
	}
	else {
	    if ((rc = c_execute_begin()) != 0)
	        sprintf(last_error, "gsdll_execute_begin returns %d", rc);
	    else
	        initialized = TRUE;
	    return rc;
	}
    }
    strcpy(last_error, func_null);
    return 1;
}

int
gsdll_class::restart(int argc, char FAR * FAR *argv)
{
    if (!hmodule) {
	strcpy(last_error, not_loaded);
	return 1;
    }
    if (!initialized) {
	strcpy(last_error, not_init);
	return 1;
    }
    if (c_execute_end && c_exit) {
	if (!execute_code)
	    c_execute_end();
	c_exit();
#ifndef __WIN32__
	FreeProcInstance((FARPROC)callback);
#endif
	// ignore return codes since they we may be aborting from an error
	initialized = FALSE;
	return init(callback, hwnd, argc, argv);
    }

    strcpy(last_error, func_null);
    return 1;
}

int 
gsdll_class::get_last_error(char *str, int len)
{
    strncpy(str, last_error,  
	(len < sizeof(last_error) ? len : sizeof(last_error)));
    return 0;
}


int 
gsdll_class::execute(const char FAR *str, int len)
{
    if (!hmodule) {
	strcpy(last_error, not_loaded);
	return 1;
    }
    if (!initialized) {
	strcpy(last_error, not_init);
	return 1;
    }
    if (c_execute_cont) {
	execute_code = c_execute_cont(str, len);
	return execute_code;
    }

    strcpy(last_error, func_null);
    return 1;
}

int 
gsdll_class::draw(const char FAR *device, HDC hdc, int dx, int dy, int wx, int wy, int sx, int sy)
{
RECT source, dest;
    source.left = sx;
    source.right = sx+wx;
    source.top = sy;
    source.bottom = sy+wy;

    dest.left = dx;
    dest.right = dx+wx;
    dest.top = dy;
    dest.bottom = dy+wy;
    c_draw((unsigned char FAR *)device, hdc, &dest, &source);
    return 0;
}

HPALETTE 
gsdll_class::copy_palette(const char FAR *device)
{
    return c_copy_palette((unsigned char FAR *)device);
}


HGLOBAL
gsdll_class::copy_dib(const char FAR *device)
{
    return c_copy_dib((unsigned char FAR *)device);
}


int 
gsdll_class::lock_device(const char FAR *device, int lock)
{
    return c_lock_device((unsigned char FAR *)device, lock);
}
