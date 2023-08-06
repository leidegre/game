@echo off

set GAME_HOME=%~dp0

call "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=amd64 -host_arch=amd64

cd %GAME_HOME%

start "" cmd /c code .

rem start "" cmd /c code .\docs

start "" wt -d %GAME_HOME% -p PowerShell --title "Game (Windows)"

rem start wt -d /mnt/c/dev/game -p Debian --title "Game (Linux)"

exit /b 0
