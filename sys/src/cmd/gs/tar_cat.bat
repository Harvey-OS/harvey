@echo off
copy dgc_head.mak+dvx-head.mak+gs.mak+jpeg.mak+devs.mak+dvx-tail.mak+unix-end.mak dvx-gcc.mak >nul
copy ansihead.mak+unixhead.mak+gs.mak+jpeg.mak+devs.mak+unixtail.mak+unix-end.mak unixansi.mak >nul
copy cc-head.mak+unixhead.mak+gs.mak+jpeg.mak+devs.mak+unixtail.mak+unix-end.mak unix-cc.mak >nul
copy gcc-head.mak+unixhead.mak+gs.mak+jpeg.mak+devs.mak+unixtail.mak+unix-end.mak unix-gcc.mak >nul
