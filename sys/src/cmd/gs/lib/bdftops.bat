@echo off
@rem $Id: bdftops.bat,v 1.3 2001/03/27 21:15:55 alexcher Exp $

call gssetgs.bat
%GSC% -q -dBATCH -dNODISPLAY -- bdftops.ps %1 %2 %3 %4 %5 %6 %7 %8 %9
