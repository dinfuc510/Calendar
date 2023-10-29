# Compiling:

### MinGW (target: x86_64-w64-mingw32)
    g++ -o calendar.exe calendar.cpp -luser32 -lgdi32 -lgdiplus -lcomctl32 -std=c++14 -DUNICODE -fpermissive -static
### MSVC
    cl .\calendar.cpp /nologo /Zi /MD /D UNICODE /link user32.lib gdi32.lib gdiplus.lib comctl32.lib
