@echo off 

echo --- building project: %1 ---

cl /nologo /Zi /Fe:build\app.exe /Fo:build\ /Fd:build\ code/win32_handmade.cpp ^
 /link /SUBSYSTEM:WINDOWS user32.lib gdi32.lib

echo.
echo Build completed at %TIME:~0,8%

