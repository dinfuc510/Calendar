# Build
### Windows MSVC
    cl .\calendar.c /Wall /O2 /link user32.lib gdi32.lib
### Windows MinGW
    gcc calendar.c -o calendar.exe -Wall -Wextra -O2 -luser32 -lgdi32
### Cross compilation from Linux to Windows
Install dependencies on Debian-based distros:

    sudo apt install gcc-mingw-w64-x86-64
    sudo apt install wine wine64

Build and run

    mingw-w64-x86_64-gcc calendar.c -o calendar.exe -Wall -Wextra -O2 -luser32 -lgdi32
    wine ./calendar.exe
