@echo off

if "%1"=="" (
	set PREFIX=%USERPROFILE%\Applications\Merlin32
) else (
	set PREFIX=%1
)

set TARGET_BIN=%PREFIX%\bin
set MACRO_DIR=%PREFIX%\asminc
set BIN_DIR=%~dp0

if not exist "%TARGET_BIN%" mkdir "%TARGET_BIN%"
if not exist "%MACRO_DIR%" mkdir "%MACRO_DIR%"

if exist "%BIN_DIR%merlin32.exe" (
    copy "%BIN_DIR%merlin32.exe" "%TARGET_BIN%"
) else (
    copy "%BIN_DIR%..\Source\merlin32.exe" "%TARGET_BIN%"
)

if exist "%BIN_DIR%"Library (
    copy "%BIN_DIR%"Library\*.* "%MACRO_DIR%"
) else (
    copy "%BIN_DIR%"..\Library\*.* "%MACRO_DIR%"
)
    
if exist "%BIN_DIR%uninstall.bat" (
    copy "%BIN_DIR%uninstall.bat" "%TARGET_BIN%\uninstall-merlin32.bat"
) else (
    copy "%BIN_DIR%..\Scripts\uninstall.bat" "%TARGET_BIN%\uninstall-merlin32.bat"
)

call "%BIN_DIR%addtopath.bat" "%TARGET_BIN%"

rem ///////////////////////////////////////////
rem
rem Pause for GUI
rem
rem //////////////////////////////////////////
rem
for /f "tokens=2" %%i in ("%CMDCMDLINE%") do (
    if "%%i"=="/c" (
        pause
    )
)

