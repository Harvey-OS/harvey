@echo off 
@rem $Id: pfbtopfa.bat,v 1.1 2003/02/06 06:09:52 alexcher Exp $
@rem Convert .pfb fonts to .pfa format

if %1/==/ goto usage
if %2/==/ goto usage
if not %3/==/ goto usage
call gssetgs.bat

%GSC% -q -dNODISPLAY -- pfbtopfa.ps %1 %2
goto end

:usage
echo "Usage: pfbtopfa input.pfb output.pfa"

:end
