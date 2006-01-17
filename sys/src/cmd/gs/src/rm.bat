@rem $Id: rm.bat,v 1.3 2002/02/21 22:24:53 giles Exp $
@echo off
:next
if '%1'=='' goto exit
if '%1'=='-f' goto sh
if exist %1 erase %1
:sh
shift
goto next
:exit
