@echo off
setlocal enabledelayedexpansion
flex lexical.l
bison -d syntax.y

set "files="
for %%f in (*.cpp) do (
    set "filename=%%~nxf"
    if not "!filename!" == "lexical.cpp" (
        set "files=!files! !filename!"
    )
)

g++ !files! -o irGenerator.exe