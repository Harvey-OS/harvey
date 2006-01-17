@echo off
@rem $Id: gsbj.bat,v 1.4 2002/02/21 21:49:28 giles Exp $

call gssetgs.bat
%GSC% -q -sDEVICE=bj10e -r180 -dNOPAUSE -sPROGNAME=gsbj -- gslp.ps %1 %2 %3 %4 %5 %6 %7 %8 %9
