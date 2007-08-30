@echo off
@rem $Id: ps2pdfxx.bat,v 1.13 2002/11/20 03:01:23 alexcher Exp $
rem Internal batch file for calling pdfwrite driver.

rem The files that call this one (ps2pdf*.bat) write the command-line
rem options into _.at, and then pass the last 2 (or fewer) arguments
rem to this file.

call gssetgs.bat
echo -q -dSAFER -dNOPAUSE -dBATCH -sDEVICE#pdfwrite >_.at2

if "%OS%"=="Windows_NT" goto nt

rem	Run ps2pdf on any Microsoft OS.

if %1/==/ goto usage
if %2/==/ goto usage

rem Watcom C deletes = signs, so use # instead.
rem We have to include the options twice because -I only takes effect if it
rem appears before other options.

:run
echo -sOutputFile#%2 >>_.at2
copy /b /y _.at2+_.at >NUL
echo -c .setpdfwrite -f %1 >>_.at2
%GSC% @_.at @_.at2
goto end

:usage
echo Usage: ps2pdf [options...] input.[e]ps output.pdf
goto end

rem	Run ps2pdf on Windows NT.

:nt
if not CMDEXTVERSION 1 goto run
if %1/==/ goto ntusage
if %2/==/ goto nooutfile
goto run

:ntusage
echo Usage: ps2pdf input.ps [output.pdf]
echo    or: ps2pdf [options...] input.[e]ps output.pdf
goto end

:nooutfile
rem We don't know why the circumlocution with _1 is needed....
set _1=%1
set _outf=%_1:.PS=.pdf%
if %_1%==%_outf% goto addsuff
call ps2pdfxx %1 %_outf%
goto postsuff

:addsuff
call ps2pdfxx %1 %1%.pdf

:postsuff
set _1=
set _outf=

:end
rem	Clean up.
if exist _.at erase _.at
if exist _.at2 erase _.at2
