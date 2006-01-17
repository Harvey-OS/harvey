@echo off
@rem $Id: bdftops.bat,v 1.5 2002/02/21 21:49:28 giles Exp $

call gssetgs.bat
%GSC% -q -dBATCH -dNODISPLAY -- bdftops.ps %1 %2 %3 %4 %5 %6 %7 %8 %9
