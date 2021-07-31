@echo off
@rem $Id: ps2pdf13.bat,v 1.3 2000/03/14 20:20:20 lpd Exp $

rem Convert PostScript to PDF 1.3 (Acrobat 4-and-later compatible).

echo -dCompatibilityLevel#1.3 >_.at
goto bot

rem Pass arguments through a file to avoid overflowing the command line.
:top
echo %1 >>_.at
shift
:bot
if not "%3"=="" goto top
ps2pdfxx %1 %2
