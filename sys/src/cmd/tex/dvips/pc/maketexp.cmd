@echo off
REM 
REM    This script file makes a new TeX PK font, because one wasn't
REM    found.  Parameters are:
REM 
REM    name dpi bdpi magnification [mode]
REM 
REM    `name' is the name of the font, such as `cmr10'.  `dpi' is
REM    the resolution the font is needed at.  `bdpi' is the base
REM    resolution, useful for figuring out the mode to make the font
REM    in.  `magnification' is a string to pass to MF as the
REM    magnification.  `mode', if supplied, is the mode to use.
REM 
REM    Because of MSDOS memory constraints, this batch file just
REM    creates a mfjob file, so that the fonts can be created later.
REM 

REM NAME=%1 DPI=%2 BDPI=%3 MAG=%4 MODE=%5

if exist dvips.mfj  goto :second
echo %% dvips.mfj - created by dvips > dvips.mfj
echo input [modes]; >> dvips.mfj
echo Created dvips.mfj

:second
echo { >> dvips.mfj
echo base=plain; >> dvips.mfj
echo font=%1; >> dvips.mfj
echo mag=%4;  >> dvips.mfj

if "%5" == ""  goto :nomode
if "%5" == "imagen"  goto :hpmode
if "%5" == "hplaser"  goto :hpmode

:goodmode
echo mode=%5[%3]; >> dvips.mfj
echo output=pk[c:\texfonts\%5\@Rrdpi\@f]; >> dvips.mfj
goto :donemode

:hpmode
echo mode=hplaser[%3]; >> dvips.mfj
echo output=pk[c:\texfonts\pixel.lj\@Rrdpi\@f]; >> dvips.mfj
goto :donemode

:nomode
echo m; >> dvips.mfj

:donemode
echo } >> dvips.mfj

REM Uncomment the following line and comment out the echo instruction to 
REM make operation automatic.

REM mfjob dvips
echo Run "mfjob dvips"
:exit 
