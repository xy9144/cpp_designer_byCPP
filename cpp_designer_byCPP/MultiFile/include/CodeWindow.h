#pragma once
#include <windows.h>
#pragma comment(lib, "comctl32.lib")
#define _WIN32_IE 0x0300
#include <commctrl.h>

// 定义命令ID
#define ID_UNDO  1001
#define ID_SAVE  1002
#define ID_RUN   1003
#define ID_STOP  1004

namespace CodeWindow {
    // 声明函数
    LRESULT CALLBACK CodeWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void InitializeToolbar(HWND hwnd, HINSTANCE hInstance);
    
    // 声明外部变量
    extern HIMAGELIST g_hImageList;
}

// 声明全局函数
LRESULT CALLBACK CodeWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam); 