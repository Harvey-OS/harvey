@echo off
@rem $Id: gssetgs.bat,v 1.3 2001/06/22 16:09:22 lpd Exp $

rem Set default values for GS (gs with graphics window) and GSC
rem (console mode gs) if the user hasn't set them.

if %GS%/==/ set GS=gswin32
if %GSC%/==/ set GSC=gswin32c
