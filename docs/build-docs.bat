@echo off
rem Builds the developer documentation (Doxygen) and opens it in the browser.
rem Double-click it, or run docs\build-docs.bat from the repository folder.
rem
rem Needs Doxygen (once per PC):    winget install DimitriVanHeesch.Doxygen
rem Optional, for the diagrams:     winget install Graphviz.Graphviz
rem Without them: download "docs-html" from the latest "Docs" run under Actions on GitHub.
setlocal
pushd "%~dp0"

rem Doxygen: on the PATH, or in the default install folder
set "DOXYGEN=doxygen"
where doxygen >nul 2>&1 || set "DOXYGEN=%ProgramFiles%\doxygen\bin\doxygen.exe"
if not "%DOXYGEN%"=="doxygen" if not exist "%DOXYGEN%" (
    echo Doxygen was not found. Install it with:  winget install DimitriVanHeesch.Doxygen
    echo or download "docs-html" from the latest "Docs" run under Actions on GitHub.
    popd
    pause
    exit /b 1
)

rem Graphviz (dot) draws the include and class diagrams; the pages are made without it too
set "LR10K_HAVE_DOT=NO"
where dot >nul 2>&1 && set "LR10K_HAVE_DOT=YES"
if "%LR10K_HAVE_DOT%"=="NO" if exist "%ProgramFiles%\Graphviz\bin\dot.exe" (
    set "PATH=%ProgramFiles%\Graphviz\bin;%PATH%"
    set "LR10K_HAVE_DOT=YES"
)
if "%LR10K_HAVE_DOT%"=="NO" echo Graphviz not found: building without diagrams.

rem The version shown on the pages: LR10KAppVersion in common.h
set "LR10K_VERSION="
for /f tokens^=2^ delims^=^" %%v in ('findstr /c:"LR10KAppVersion =" "..\main\LloydLR10KTensile\common.h"') do set "LR10K_VERSION=%%v"

echo Building the documentation for version %LR10K_VERSION% ...
if exist html rmdir /s /q html
"%DOXYGEN%" Doxyfile
if errorlevel 1 (
    echo Doxygen failed.
    popd
    pause
    exit /b 1
)

for %%f in (doxygen-warnings.txt) do if %%~zf gtr 0 echo Warnings (wrong @param names etc.) are in docs\doxygen-warnings.txt
start "" "html\index.html"
popd
endlocal
