@rem $Id: rm.bat,v 1.1 2000/03/09 08:40:44 lpd Exp $
@echo off
:next
if '%1'=='' goto exit
if '%1'=='-f' goto sh
if exist %1 erase %1
:sh
shift
goto next
:exit
