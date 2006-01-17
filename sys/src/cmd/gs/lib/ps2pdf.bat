@echo off
@rem $Id: ps2pdf.bat,v 1.9 2005/08/31 18:31:01 igor Exp $

rem Convert PostScript to PDF 1.4 (Acrobat 4-and-later compatible).
rem The default PDF compatibility level may change in the future:
rem use ps2pdf12 or ps2pdf13 if you want a specific level.

rem The current default compatibility level is PDF 1.4.
echo -dCompatibilityLevel#1.4 >_.at
goto bot

rem Pass arguments through a file to avoid overflowing the command line.
:top
echo %1 >>_.at
shift
:bot
if not %3/==/ goto top
call ps2pdfxx %1 %2
