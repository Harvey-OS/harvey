@echo off
@rem $Id: ps2pdfxx.bat,v 1.1 2000/03/14 20:20:20 lpd Exp $
rem Internal batch file for calling pdfwrite driver.

rem	NOTE: for questions about using this file on Windows NT,
rem	please contact Matt Sergeant (sergeant@geocities.com).

rem The files that call this one (ps2pdf*.bat) write the command-line
rem options into _.at, and then pass the last 2 (or fewer) arguments
rem to this file.

echo -q -dNOPAUSE -dBATCH -sDEVICE#pdfwrite >_.at2
set PS2PDFGS=gswin32c

if "%OS%"=="Windows_NT" goto nt

rem	Run ps2pdf on any Microsoft OS.

if "%1"=="" goto usage
if "%2"=="" goto usage

rem Watcom C deletes = signs, so use # instead.
rem Doing an initial 'save' helps keep fonts from being flushed between pages.
rem We have to include the options twice because -I only takes effect if it
rem appears before other options.

:run
echo -sOutputFile#%2 >>_.at2
copy /b /y _.at2+_.at >NUL
echo -c save pop -f %1 >>_.at2
%PS2PDFGS% @_.at @_.at2
goto end

:usage
echo Usage: ps2pdf [options...] input.ps output.pdf
goto end

rem	Run ps2pdf on Windows NT.

:nt
set PS2PDFGS=gswin32c
if not CMDEXTVERSION 1 goto run
if "%1"=="" goto ntusage
if "%2"=="" goto nooutfile
goto run

:ntusage
echo Usage: ps2pdf input.ps [output.pdf]
echo    or: ps2pdf [options...] input.ps output.pdf
goto end

:nooutfile
ps2pdfxx %1 %1:.PS=.PDF%

:end
rem	Clean up.
SET PS2PDFGS=
if exist _.at erase _.at
if exist _.at2 erase _.at2
