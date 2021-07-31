@echo off 
@rem $Id: pdfopt.bat,v 1.4 2001/07/23 06:57:27 lpd Exp $
@rem Convert PDF to "optimized" form.

if %1/==/ goto usage
if %2/==/ goto usage
call gssetgs.bat
echo -q -dNODISPLAY -dSAFER -dDELAYSAFER >_.at
:cp
if %3/==/ goto doit
echo %1 >>_.at
shift
goto cp

:doit
%GSC% -q @_.at -- pdfopt.ps %1 %2
goto end

:usage
echo "Usage: pdfopt input.pdf output.pdf"

:end
