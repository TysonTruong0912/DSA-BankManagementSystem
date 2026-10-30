@echo off
REM Bien dich tren Windows (can MinGW-w64 g++/gcc trong PATH, vi du MSYS2 hoac w64devkit).
REM Dung kem sqlite3.c / sqlite3.h co san trong thu muc nay -> khong can cai them SQLite.

if exist sqlite3.c (
    echo [1/2] Bien dich SQLite...
    gcc -O2 -DSQLITE_OMIT_LOAD_EXTENSION -c sqlite3.c -o sqlite3.o || goto :loi
    echo [2/2] Bien dich chuong trinh...
    g++ -std=c++11 -O2 -Wall -Wextra -DUSE_SQLITE -I. *.cpp sqlite3.o -o bank.exe || goto :loi
) else (
    echo Khong thay sqlite3.c -> bien dich che do file .txt ^(khong co co so du lieu^)
    g++ -std=c++11 -O2 -Wall -Wextra *.cpp -o bank.exe || goto :loi
)
echo.
echo Thanh cong! Chay: bank.exe
goto :eof

:loi
echo.
echo Bien dich THAT BAI.
exit /b 1
