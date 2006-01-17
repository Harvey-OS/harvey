/* $Id: ps2ascii.cmd,v 1.4 2002/02/21 21:49:28 giles Exp $ */
/*
 * This file is maintained by a user: if you have any questions about it,
 * please contact Mark Hale (mark.hale@physics.org).
 */

@echo off
if '%1'=='' goto a0
if '%2'=='' goto a1
gsos2 -q -dNODISPLAY -dDELAYBIND -dWRITESYSTEMDICT -dSIMPLE ps2ascii.ps %1 -c quit >%2
goto x
:a0
gsos2 -q -dNODISPLAY -dDELAYBIND -dWRITESYSTEMDICT -dSIMPLE ps2ascii.ps - -c quit
goto x
:a1
gsos2 -q -dNODISPLAY -dDELAYBIND -dWRITESYSTEMDICT -dSIMPLE ps2ascii.ps %1 -c quit
goto x
:x
