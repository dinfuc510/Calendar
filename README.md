# Compiling:

### MinGW
    g++ -o calendar.exe calendar.cpp -O3 -luser32 -lgdi32 -lgdiplus -fpermissive -static -mwindows
### MSVC
    cl .\calendar.cpp /nologo /EHsc /Zi /Ox /link user32.lib gdi32.lib gdiplus.lib /SUBSYSTEM:WINDOWS
