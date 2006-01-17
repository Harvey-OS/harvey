@echo off
@rem $Id: ps2pdf12.bat,v 1.7 2002/02/21 21:49:28 giles Exp $

rem Convert PostScript to PDF 1.2 (Acrobat 3-and-later compatible).

echo -dCompatibilityLevel#1.2 >_.at
goto bot

rem Pass arguments through a file to avoid overflowing the command line.
:top
echo %1 >>_.at
shift
:bot
if not %3/==/ goto top
call ps2pdfxx %1 %2
