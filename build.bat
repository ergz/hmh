@echo off 

echo --- building project: %1 ---
echo.

cl /Zi /Fe:build\app.exe /Fo:build\ /Fd:build\ code/win32_handmade.cpp ^
 /link /SUBSYSTEM:WINDOWS user32.lib gdi32.lib XInput.lib

echo.
echo Build completed at %TIME:~0,8%
echo.

rem @echo off

rem REM Build your application here
rem clang++ -o build\app.exe -g code\win32_handmade.cpp -luser32 -lgdi32

rem REM Print an empty line and the current time to the console in 24-hour format without milliseconds
rem echo.
rem echo Build completed successfully at %TIME:~0,8%
