@echo off
@rem $Id: lpr2.bat,v 1.2 2000/05/20 20:53:05 lpd Exp $

call gssetgs.bat
%GSC% -sDEVICE#djet500 -dNOPAUSE -sPROGNAME=lpr2 -- gslp.ps -2r %1 %2 %3 %4 %5 %6 %7 %8 %9
