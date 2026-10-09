@echo off
setlocal
cd /d "%~dp0"
where g++ >nul 2>nul
if errorlevel 1 (
 echo Instala MSYS2 y el compilador mingw-w64-ucrt-x86_64-gcc.
 echo Ejecuta este archivo desde una terminal UCRT64 con g++ y windres en PATH.
 exit /b 1
)
windres -I src src/app.rc -o app-res.o
if errorlevel 1 exit /b 1
g++ -std=c++17 -O2 -Wall -Wextra -Wno-misleading-indentation -I vendor src/main.cpp app-res.o -o LANDELTA.exe -municode -mwindows -static -static-libgcc -static-libstdc++ -lcomdlg32 -lgdiplus -lws2_32 -lshell32 -lole32 -luuid -luser32 -lgdi32
if errorlevel 1 exit /b 1
del app-res.o
echo Listo: LANDELTA.exe
