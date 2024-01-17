# Build
Requirement:
-    Windows SDK + MSVC compiler
-    MinGW compiler (an alternative to MSVC compiler, required for cross compilation from Linux to Windows)
-    Wine (only for cross compilation from Linux to Windows)

Install dependencies on Debian-based distros:

    sudo apt install g++-mingw-w64-x86-64
    sudo apt install wine wine64
### Windows MSVC
    cl calendar.cpp /nologo /Zi /MD /Ox /link user32.lib gdi32.lib
### MinGW
    g++ calendar.cpp -o calendar.exe -O3 -luser32 -lgdi32 -mwindows
Use Wine to run

    wine ./calendar.exe
