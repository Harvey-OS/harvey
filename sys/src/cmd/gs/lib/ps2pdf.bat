@echo off
@rem $Id: ps2pdf.bat,v 1.2 2000/03/14 20:20:20 lpd Exp $

rem Convert PostScript to PDF 1.3 (Acrobat 4-and-later compatible).
rem The PDF compatibility level may change in the future:
rem use ps2pdf12 or ps2pdf13 if you want a specific level.

rem >_.at
goto bot

rem Pass arguments through a file to avoid overflowing the command line.
:top
echo %1 >>_.at
shift
:bot
if not "%3"=="" goto top
ps2pdfxx %1 %2
