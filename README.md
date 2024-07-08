# Compiling:

### MinGW
    g++ -Wall -Wextra -O2 calendar.cpp -o calendar.exe -luser32 -lgdi32 -lgdiplus -mwindows
### MSVC
    cl .\calendar.cpp /nologo /EHsc /O2 /link user32.lib gdi32.lib gdiplus.lib /SUBSYSTEM:WINDOWS
