@echo off
@rem $Id: gssetgs.bat,v 1.2 2000/10/01 15:25:37 alexcher Exp $

rem Set default values for GS (gs with graphics window) and GSC
rem (console mode gs) if the user hasn't set them.

if "%GS%"=="" set GS=gswin32
if "%GSC%"=="" set GSC=gswin32c
