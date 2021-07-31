/* $Id: ps2ascii.cmd,v 1.1 2000/07/05 16:21:13 lpd Exp $ */
/*
 * This file is maintained by a user: if you have any questions about it,
 * please contact Mark Hale (mark.hale@physics.org).
 */

@echo off
if '%1'=='' goto a0
if '%2'=='' goto a1
gsos2 -q -dNODISPLAY -dNOBIND -dWRITESYSTEMDICT -dSIMPLE ps2ascii.ps %1 -c quit >%2
goto x
:a0
gsos2 -q -dNODISPLAY -dNOBIND -dWRITESYSTEMDICT -dSIMPLE ps2ascii.ps - -c quit
goto x
:a1
gsos2 -q -dNODISPLAY -dNOBIND -dWRITESYSTEMDICT -dSIMPLE ps2ascii.ps %1 -c quit
goto x
:x
