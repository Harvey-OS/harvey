@echo off 
@rem $Id: eps2eps.bat,v 1.7 2002/02/21 21:49:28 giles Exp $
@rem "Distill" Encapsulated PostScript.

if %1/==/ goto usage
if %2/==/ goto usage
call gssetgs.bat
echo -dNOPAUSE -dBATCH -dSAFER >_.at
rem Watcom C deletes = signs, so use # instead.
echo -dDEVICEWIDTH#250000 -dDEVICEHEIGHT#250000 >>_.at
:cp
if %3/==/ goto doit
echo %1 >>_.at
shift
goto cp

:doit
rem Watcom C deletes = signs, so use # instead.
%GSC% -q -sDEVICE#epswrite -sOutputFile#%2 @_.at %1
if exist _.at erase _.at
goto end

:usage
echo "Usage: eps2eps ...switches... input.eps output.eps"

:end
