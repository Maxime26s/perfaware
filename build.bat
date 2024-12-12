@echo off

echo Building...
call vcvars.bat
if not exist build mkdir build
pushd build
cl -MT -nologo -Gm- -GR- -EHa- -Od -Oi -W0 -FC -Z7 ..\src\main.cpp
popd

exit /b 0
