@echo off
@rem $Id: gsbj.bat,v 1.2 2000/05/20 20:53:05 lpd Exp $

call gssetgs.bat
%GSC% -q -sDEVICE=bj10e -r180 -dNOPAUSE -sPROGNAME=gsbj -- gslp.ps %1 %2 %3 %4 %5 %6 %7 %8 %9
