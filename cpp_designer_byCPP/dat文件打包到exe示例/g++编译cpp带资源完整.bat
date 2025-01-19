@echo off
chcp 65001
echo 正在编译资源文件...
windres -i resource.rc -o resource.o --codepage=65001
if %errorlevel% neq 0 (
    echo 资源编译失败！
    pause
    exit /b 1
)

echo 正在编译程序...
g++ hello.cpp resource.o -o hello.exe -mwindows -lgdiplus -lcomctl32 -lole32 -std=c++11
if %errorlevel% neq 0 (
    echo 程序编译失败！
    pause
    exit /b 1
)

echo 编译成功！
del resource.o
pause