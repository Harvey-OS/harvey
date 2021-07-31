@echo off
@rem $Id: rm.cmd,v 1.1 2000/03/09 08:40:44 lpd Exp $
:next
if '%1'=='' goto exit
if '%1'=='-f' goto sh
erase %1
:sh
shift
goto next
:exit
