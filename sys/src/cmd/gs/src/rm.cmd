@echo off
@rem $Id: rm.cmd,v 1.3 2002/02/21 22:24:53 giles Exp $
:next
if '%1'=='' goto exit
if '%1'=='-f' goto sh
erase %1
:sh
shift
goto next
:exit
