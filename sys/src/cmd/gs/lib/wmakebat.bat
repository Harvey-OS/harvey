@rem Execute this script with echo on, so we can see what's happening.
@rem $Id: wmakebat.bat,v 1.1 2000/03/09 08:40:40 lpd Exp $
wmakel -u -n -h %1 %2 %3 %4 %5 %6 %7 %8 %9 >_wm_temp.bat
_wm_temp.bat
