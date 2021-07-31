@echo off
@rem $Id: ps2pdf.bat,v 1.5 2000/07/24 15:12:21 lpd Exp $

rem Convert PostScript to PDF 1.2 (Acrobat 3-and-later compatible).
rem The default PDF compatibility level may change in the future:
rem use ps2pdf12 or ps2pdf13 if you want a specific level.

rem The current default compatibility level is PDF 1.2.
echo -dCompatibilityLevel#1.2 >_.at
goto bot

rem Pass arguments through a file to avoid overflowing the command line.
:top
echo %1 >>_.at
shift
:bot
if not "%3"=="" goto top
call ps2pdfxx %1 %2
