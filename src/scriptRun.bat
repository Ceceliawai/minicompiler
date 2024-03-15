@echo off
setlocal enabledelayedexpansion

for %%f in (..\test\NJUtest\Test2\*) do (
    output.exe %%f > ..\test\NJUtest\log2\%%~nf.cmm
)