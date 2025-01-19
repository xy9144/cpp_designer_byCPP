#define UNICODE
#define _UNICODE
#include <windows.h>
#include <commctrl.h>
#include "CodeWindow.h"
#include "Utils.h"
#include "Resource.h"
#include <fstream>
#include <vector>

// 重命名或添加命名空间
namespace CodeWindow {
    // 定义变量
    HIMAGELIST g_hImageList = NULL;
    HANDLE g_hProcess = NULL;  // 添加进程句柄定义

    // 代码窗口过程
    LRESULT CALLBACK CodeWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        static HWND hToolBar = NULL;
        static HWND hEdit = NULL;

        switch (uMsg)
        {
            case WM_CREATE:
            {
                // 创建工具条
                hToolBar = CreateWindowEx(
                    0,                          // 扩展样式
                    TOOLBARCLASSNAME,          // 工具条类名
                    NULL,                      // 窗口标题
                    WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS | CCS_ADJUSTABLE,  // 窗口样式
                    0, 0, 0, 0,               // 位置和大小
                    hwnd,                      // 父窗口句柄
                    (HMENU)100,               // 菜单句柄
                    GetModuleHandle(NULL),    // 实例句柄
                    NULL                      // 附加参数
                );

                // 设置工具条按钮大小和图片列表
                SendMessage(hToolBar, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
                SendMessage(hToolBar, TB_SETIMAGELIST, 0, (LPARAM)g_hImageList);

                // 创建工具条按钮
                TBBUTTON tbButtons[] = {
                    {8, ID_UNDO, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)L"撤销"},
                    {2, ID_SAVE, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)L"保存"},
                    {9, ID_RUN, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)L"运行"},
                    {10, ID_STOP, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)L"结束"}
                };

                SendMessage(hToolBar, TB_ADDBUTTONS, 4, (LPARAM)&tbButtons);
                SendMessage(hToolBar, TB_AUTOSIZE, 0, 0);

                // 创建编辑框
                hEdit = CreateWindowEx(
                    WS_EX_CLIENTEDGE,
                    L"EDIT",
                    L"",
                    WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | 
                    ES_MULTILINE | ES_AUTOHSCROLL | ES_AUTOVSCROLL | 
                    ES_NOHIDESEL | ES_WANTRETURN,  // 添加 ES_WANTRETURN 支持回车换行
                    0, 0, 0, 0,
                    hwnd,
                    NULL,
                    GetModuleHandle(NULL),
                    NULL
                );

                // 设置编辑框字体
                HFONT hFont = CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                    DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Consolas");
                SendMessage(hEdit, WM_SETFONT, (WPARAM)hFont, TRUE);

                // 设置Tab字符的宽度
                DWORD tabStops = 16;  // 4个空格的宽度
                SendMessage(hEdit, EM_SETTABSTOPS, 1, (LPARAM)&tabStops);

                return 0;
            }

            case WM_COMMAND:
            {
                switch (LOWORD(wParam))
                {
                    case ID_UNDO:  // 撤销按钮
                    {
                        SendMessage(hEdit, WM_UNDO, 0, 0);
                        return 0;
                    }

                    case ID_SAVE:  // 保存按钮
                    {
                        // 获取编辑框文本长度
                        int length = GetWindowTextLength(hEdit);
                        std::vector<wchar_t> buffer(length + 1);
                        GetWindowText(hEdit, buffer.data(), length + 1);

                        // 保存到文件
                        std::wstring exePath = GetExecutablePath();
                        std::wstring filePath = exePath + L"\\test.cpp";
                        std::ofstream file(filePath.c_str(), std::ios::binary);
                        if (file.is_open()) {
                            // 写入 UTF-8 BOM
                            unsigned char bom[] = { 0xEF, 0xBB, 0xBF };
                            file.write(reinterpret_cast<char*>(bom), sizeof(bom));
                            
                            // 转换为UTF-8并写入
                            std::string utf8Content = WideToUTF8(buffer.data());
                            file.write(utf8Content.c_str(), utf8Content.length());
                            file.close();
                        }
                        return 0;
                    }

                    case ID_RUN:  // 运行按钮
                    {
                        // 获取当前目录
                        std::wstring exePath = GetExecutablePath();
                        
                        // 运行编译脚本
                        STARTUPINFO si = { sizeof(STARTUPINFO) };
                        PROCESS_INFORMATION pi;
                        std::wstring cmdLine = exePath + L"\\编译cpp.bat";
                        
                        if (CreateProcess(NULL, (LPWSTR)cmdLine.c_str(), NULL, NULL, FALSE,
                            CREATE_NO_WINDOW, NULL, exePath.c_str(), &si, &pi)) {
                            // 等待编译完成
                            WaitForSingleObject(pi.hProcess, INFINITE);
                            CloseHandle(pi.hProcess);
                            CloseHandle(pi.hThread);
                            
                            // 延时5秒
                            Sleep(5000);
                            
                            // 运行编译后的程序
                            STARTUPINFO si2 = { sizeof(STARTUPINFO) };
                            PROCESS_INFORMATION pi2;
                            std::wstring exeFile = exePath + L"\\hello_release.exe";
                            
                            if (CreateProcess(NULL, (LPWSTR)exeFile.c_str(), NULL, NULL, FALSE,
                                0, NULL, exePath.c_str(), &si2, &pi2)) {
                                g_hProcess = pi2.hProcess;  // 保存进程句柄
                                CloseHandle(pi2.hThread);
                            }
                        }
                        return 0;
                    }

                    case ID_STOP:  // 结束按钮
                    {
                        if (g_hProcess != NULL) {
                            // 终止进程
                            TerminateProcess(g_hProcess, 0);
                            CloseHandle(g_hProcess);
                            g_hProcess = NULL;
                        }
                        return 0;
                    }
                }
                break;
            }

            case WM_SIZE:
            {
                RECT rcClient;
                GetClientRect(hwnd, &rcClient);
                
                // 获取工具条高度
                RECT rcToolBar;
                SendMessage(hToolBar, TB_AUTOSIZE, 0, 0);
                GetWindowRect(hToolBar, &rcToolBar);
                int toolBarHeight = rcToolBar.bottom - rcToolBar.top;

                // 调整编辑框大小
                MoveWindow(hEdit, 0, toolBarHeight, rcClient.right, rcClient.bottom - toolBarHeight, TRUE);
                return 0;
            }

            case WM_DESTROY:
                // 确保进程被终止
                if (g_hProcess != NULL) {
                    TerminateProcess(g_hProcess, 0);
                    CloseHandle(g_hProcess);
                    g_hProcess = NULL;
                }
                PostQuitMessage(0);
                return 0;
        }
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

}

// 移除 extern "C" 声明，直接使用命名空间版本
LRESULT CALLBACK CodeWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return CodeWindow::CodeWindowProc(hwnd, uMsg, wParam, lParam);
}
