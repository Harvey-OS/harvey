@rem $Id: ps2write.bat,v 1.16 2005/05/30 21:00:33 igor Exp $
@rem Converting Postscript 3 or PDF into PostScript 2.

set invoke0=%GSC% -dBATCH -dNOPAUSE -dNOOUTERSAVE -sDEVICE=pdfwrite -sOutputFile=temp.pdf %more_param% 
set invoke1=-c mark /ForOPDFRead true /PreserveHalftoneInfo true /TransferFunctionInfo /Preserve 
set invoke2=/MaxViewerMemorySize 8000000 /CompressPages false /CompressFonts false /ASCII85EncodePages true 
set invoke3=%more_deviceparam% .dicttomark setpagedevice -f %more_param1%
set invoke=%invoke0% %invoke1% %invoke2% %invoke3%

set procsets=%GS_LIBPATH%opdfread.ps+%GS_LIBPATH%gs_agl.ps+%GS_LIBPATH%gs_mro_e.ps+%GS_LIBPATH%gs_mgl_e.ps

if %jobserver%. == yes. goto s
%invoke%  %1
goto j
:s
%invoke% -c false 0 startjob pop -f - < %1
:j
copy /b %procsets%+temp.pdf+%GS_LIBPATH%EndOfTask.ps %2
