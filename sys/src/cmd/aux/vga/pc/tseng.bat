rem this extracts modes from the cardinal VGA765 board
rem with its tseng 4000

if not "%1" == "" goto continue
echo usage: tseng label
goto exit

:continue

echo %1	320 200 8>tseng.log
dm -t tseng -m 13 "320x200x8" >>tseng.log

vmode 72hz
echo %1	640 480 1>>tseng.log
dm -t tseng -m 25 "640x480x1 72h" >>tseng.log
vmode 80x25

vmode 48k
echo %1	800 600 1>>tseng.log
dm -t tseng -m 29 "800x600x1 48k" >>tseng.log
vmode 80x25

vmode 72h
echo %1	640 480 8>>tseng.log
dm -t tseng -m 2e "640x480x8 72h" >>tseng.log
vmode 80x25

vmode 48k
echo %1	800 600 8>>tseng.log
dm -t tseng -m 30 "800x600x8 48k">>tseng.log
vmode 80x25

vmode 72m
echo %1	1024 768 1>>tseng.log
dm -t tseng -m 37 "1024x768x1 ni 72m" >>tseng.log
vmode 80x25

vmode 72m
echo %1	1024 768 8>>tseng.log
dm -t tseng -m 38 "1024x768x8 ni 72m" >>tseng.log
vmode 80x25

echo %1	1280 1024 1>>tseng.log
dm -t tseng -m 3d "1280x1024x1 i" >>tseng.log
vmode 80x25

:exit

