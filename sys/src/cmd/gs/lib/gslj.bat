@echo off
@rem $Id: gslj.bat,v 1.4 2002/02/21 21:49:28 giles Exp $

call gssetgs.bat
%GSC% -q -sDEVICE=laserjet -r300 -dNOPAUSE -sPROGNAME=gslj -- gslp.ps %1 %2 %3 %4 %5 %6 %7 %8 %9
