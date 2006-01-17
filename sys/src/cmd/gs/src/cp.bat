@rem $Id: cp.bat,v 1.3 2002/02/21 22:24:51 giles Exp $
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
