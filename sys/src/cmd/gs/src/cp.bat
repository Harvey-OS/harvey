@rem $Id: cp.bat,v 1.1 2000/03/09 08:40:40 lpd Exp $
@echo off
if "%2"=="." goto ne
if exist _.tmp erase _.tmp
rem	Both of the following lines are necessary:
rem	the first one works on MS DOS and Windows 95/98 but not Windows NT,
rem	the second works on Windows NT but not the other MS OSs.
rem > _.tmp
copy nul: _.tmp > nul:
copy /B %1+_.tmp %2
erase _.tmp
goto x
:ne
copy /B %1 %2
:x
