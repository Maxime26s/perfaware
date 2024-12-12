@echo off

echo Executing on %1...
.\build\main.exe %1 > %2

exit /b 0
