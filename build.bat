@echo off

REM 4100 Unreferenced func param (cleanup)
REM 4820 struct padding
REM 4255 () != (void)
REM 4668 Macro not defined. Subst w/0
REM 4710 Func not inlined
set comment_for_cleanup=/wd4100
set suppressed=%comment_for_cleanup% /wd4820 /wd4255 /wd4668 /wd4710


cl /Wall /WX /nologo /Zi /Od  -D_CRT_SECURE_NO_WARNINGS %suppressed% proyecto01.c /FeProyecto01.exe
