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
copy "%BIN_DIR%..\Source\merlin32.exe" "%TARGET_BIN%"
if not exist "%MACRO_DIR%" mkdir "%MACRO_DIR%"
copy "%BIN_DIR%"..\Library\*.* "%MACRO_DIR%"
"%BIN_DIR%addtopath.bat" "%TARGET_BIN%"
