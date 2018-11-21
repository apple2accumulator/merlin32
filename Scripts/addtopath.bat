@echo off

rem Script:  add-to-path.bat
rem Author:  Bill Chatfield
rem License: GPL2

rem
rem Check for help argument.
rem
set ARG=%1
if defined ARG (
    set ARG=%ARG:H=h%
    set ARG=!ARG:?=h!
    set ARG=!ARG:/=-!
    set ARG=!ARG:--=-!
    set ARG=!ARG:~0,2!
    if !ARG!==-h (
        echo Usage: %~n0 [directory]
    )
)

if "%1"=="" (
	set TARGET_DIR=%CD%
) else (
	if "%1"=="." (
		set TARGET_DIR=%CD%
	) else (
		set TARGET_DIR=%~1
	)
)

setlocal EnableDelayedExpansion

rem Retrieve the user's Path from the master environment.
set MY_PATH=
for /f "skip=1 tokens=3*" %%p in ('reg query hkcu\Environment /v Path') do (
    set MY_PATH=%%p %%q

    rem If it's already in the master environment then don't add it.
    echo !MY_PATH! | find /i "%TARGET_DIR%" > NUL:
    if !ERRORLEVEL! EQU 0 (
        echo Directory %TARGET_DIR% already exists in the permanent Path variable.
    ) else (
        rem Add to user's master enviroment.
        setx Path "!MY_PATH!;%TARGET_DIR%"
        if ERRORLEVEL 1 (
            echo Setx command failed 1>&2
            exit 1
        )
        echo Added directory %TARGET_DIR% to permanent Path variable.
    )
)

rem If the user's Path doesn't exist yet, create it.
if not defined MY_PATH (
    setx Path %TARGET_DIR%
    if !ERRORLEVEL! NEQ 0 (
        echo Setx command failed 1>&2
        exit 1
    )
    echo Created permanent Path variable with directory %TARGET_DIR%.
)

rem If it's already in the current environment then don't add it.
echo %PATH% | find /i "%TARGET_DIR%" > NUL:
if %ERRORLEVEL% EQU 0 (
    echo Directory %TARGET_DIR% already exists in the Path.
    goto :EOF
)

endlocal

rem Add to current environment.
set PATH=%PATH%;%TARGET_DIR%
echo Added directory %TARGET_DIR% to Path in current environment.
set TARGET_DIR=
