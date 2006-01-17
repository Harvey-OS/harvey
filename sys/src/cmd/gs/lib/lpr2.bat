@echo off
@rem $Id: lpr2.bat,v 1.4 2002/02/21 21:49:28 giles Exp $

call gssetgs.bat
%GSC% -sDEVICE#djet500 -dNOPAUSE -sPROGNAME=lpr2 -- gslp.ps -2r %1 %2 %3 %4 %5 %6 %7 %8 %9
