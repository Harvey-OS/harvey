@echo off 
@rem $Id: pdf2ps.bat,v 1.1 2000/03/09 08:40:40 lpd Exp $
@rem Convert PDF to PostScript.

if "%1"=="" goto usage
if "%2"=="" goto usage
echo -dNOPAUSE -dBATCH -sDEVICE#pswrite >_.at
:cp
if "%3"=="" goto doit
echo %1 >>_.at
shift
goto cp

:doit
rem Watcom C deletes = signs, so use # instead.
gs -q -sOutputFile#%2 @_.at %1
goto end

:usage
echo "Usage: pdf2ps [-dASCII85DecodePages=false] [-dLanguageLevel=n] input.pdf output.ps"

:end
