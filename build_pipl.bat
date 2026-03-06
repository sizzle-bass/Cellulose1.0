@echo off
setlocal

set CL="C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Tools\MSVC\14.29.30133\bin\Hostx64\x64\cl.exe"
set SDK_H="D:\Claude Sandbox\Cellulose\sdk\AfterEffectsSDK_25.6_61_win\ae25.6_61.64bit.AfterEffectsSDK\Examples\Headers"
set PIPL_R="D:\Claude Sandbox\Cellulose\src\Cellulose.r"
set PIPLTOOL=D:\Claude Sandbox\Cellulose\sdk\AfterEffectsSDK_25.6_61_win\ae25.6_61.64bit.AfterEffectsSDK\Examples\Resources\PiPLtool.exe

REM Output destination (lives in src/ so it's committed with the project)
set OUT_RC="D:\Claude Sandbox\Cellulose\src\CellulosePiPL.rc"

REM Use a temp directory with NO spaces for PiPLtool
set TMPDIR=C:\Temp\CellulosePiPL
if not exist %TMPDIR% mkdir %TMPDIR%

set RR=%TMPDIR%\Cellulose.rr
set RRC=%TMPDIR%\Cellulose.rrc
set RC=%TMPDIR%\Cellulose.rc

echo Step 1: Preprocessing .r -^> .rr ...
%CL% /nologo /I %SDK_H% /EP %PIPL_R% > %RR%
if errorlevel 1 (echo FAILED step 1 & exit /b 1)

echo Step 2: PiPLtool .rr -^> .rrc ...
"%PIPLTOOL%" %RR% %RRC%
if errorlevel 1 (echo FAILED step 2 & exit /b 1)

echo Step 3: Generating .rc ...
%CL% /nologo /D MSWindows /EP %RRC% > %RC%
if errorlevel 1 (echo FAILED step 3 & exit /b 1)

echo Copying .rc to src/ ...
copy /Y %RC% %OUT_RC%
if errorlevel 1 (echo FAILED copy & exit /b 1)

echo.
echo Success! CellulosePiPL.rc written to src\
endlocal
