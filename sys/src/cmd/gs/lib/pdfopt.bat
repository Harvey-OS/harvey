@echo off 
@rem $Id: pdfopt.bat,v 1.2 2000/05/20 20:53:05 lpd Exp $
@rem Convert PDF to "optimized" form.

if "%1"=="" goto usage
if "%2"=="" goto usage
call gssetgs.bat
echo -q -dNODISPLAY -dSAFER >_.at
:cp
if "%3"=="" goto doit
echo %1 >>_.at
shift
goto cp

:doit
%GSC% -q @_.at -- pdfopt.ps %1 %2
goto end

:usage
echo "Usage: pdfopt input.pdf output.pdf"

:end
