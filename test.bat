@echo off

if not exist listings\ (
    echo Error: listings\ directory not found.
    exit /b 1
)

call build.bat

for %%f in (listings\*.asm) do (
    echo Processing %%f...
    call nasm.bat %%f
    if errorlevel 1 (
        echo Error during nasm compilation of %%f
        exit /b 1
    )

    call .\run.bat build\%%~nf build\output_%%~nf.asm
    if errorlevel 1 (
        echo Error during call to run.bat for %%f
        exit /b 1
    )

    call nasm.bat build\output_%%~nf.asm
    if errorlevel 1 (
        echo Error during nasm compilation of output.asm for %%f
        exit /b 1
    )

    git diff --no-index build\%%~nf build\output_%%~nf
)

echo All files processed.
exit /b 0
