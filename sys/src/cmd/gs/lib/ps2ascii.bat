@echo off
@rem $Id: ps2ascii.bat,v 1.1 2000/03/09 08:40:40 lpd Exp $
if '%1'=='' goto a0
if '%2'=='' goto a1
gs -q -dNODISPLAY -dNOBIND -dWRITESYSTEMDICT -dSIMPLE ps2ascii.ps %1 -c quit >%2
goto x
:a0
gs -q -dNODISPLAY -dNOBIND -dWRITESYSTEMDICT -dSIMPLE ps2ascii.ps - -c quit
goto x
:a1
gs -q -dNODISPLAY -dNOBIND -dWRITESYSTEMDICT -dSIMPLE ps2ascii.ps %1 -c quit
goto x
:x
