if not exist .\WinDebug\nul mkdir .\WinDebug
cl.exe /nologo /MT /W3 /GX /Z7 /YX /Od /I .. /I . /I ../include /D WIN32 /D WINDOWS32 /D _DEBUG /D _WINDOWS /FR.\WinDebug/ /Fp.\WinDebug/subproc.pch /Fo.\WinDebug/ /c misc.c
cl.exe /nologo /MT /W3 /GX /Z7 /YX /Od /I .. /I . /I ../include /I ../.. /D WIN32 /D WINDOWS32 /D _DEBUG /D _WINDOWS /FR.\WinDebug/ /Fp.\WinDebug/subproc.pch /Fo.\WinDebug/ /c sub_proc.c
cl.exe /nologo /MT /W3 /GX /Z7 /YX /Od /I .. /I . /I ../include /D WIN32 /D WINDOWS32 /D _DEBUG /D _WINDOWS /FR.\WinDebug/ /Fp.\WinDebug/subproc.pch /Fo.\WinDebug/ /c w32err.c
lib.exe /NOLOGO /OUT:.\WinDebug\subproc.lib  .\WinDebug/misc.obj  .\WinDebug/sub_proc.obj  .\WinDebug/w32err.obj
if not exist .\WinRel\nul mkdir .\WinRel
cl.exe /nologo /MT /W3 /GX /YX /O2 /I ../include /D WIN32 /D WINDOWS32 /D NDEBUG /D _WINDOWS /FR.\WinRel/ /Fp.\WinRel/subproc.pch /Fo.\WinRel/ /c misc.c
cl.exe /nologo /MT /W3 /GX /YX /O2 /I ../include /I ../.. /D WIN32 /D WINDOWS32 /D NDEBUG /D _WINDOWS /FR.\WinRel/ /Fp.\WinRel/subproc.pch /Fo.\WinRel/ /c sub_proc.c
cl.exe /nologo /MT /W3 /GX /YX /O2 /I ../include /D WIN32 /D WINDOWS32 /D NDEBUG /D _WINDOWS /FR.\WinRel/ /Fp.\WinRel/subproc.pch /Fo.\WinRel/ /c w32err.c
lib.exe /NOLOGO /OUT:.\WinRel\subproc.lib  .\WinRel/misc.obj  .\WinRel/sub_proc.obj  .\WinRel/w32err.obj
