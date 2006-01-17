@echo off
@rem $Id: gssetgs.bat,v 1.5 2002/02/21 21:49:28 giles Exp $

rem Set default values for GS (gs with graphics window) and GSC
rem (console mode gs) if the user hasn't set them.

if %GS%/==/ set GS=gswin32
if %GSC%/==/ set GSC=gswin32c
