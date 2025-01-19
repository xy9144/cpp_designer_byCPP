#define UNICODE
#define _UNICODE
#include <windows.h>
#include <commctrl.h>
#include <string>
#include <vector>

//定义控件ID
#define 列表框1_ID 1
#define 按钮1_ID 2
#define 标签1_ID 3
#define 状态栏_ID 4

//定义全局变量，一般为控件句柄
HWND 列表框1句柄; 
HWND 按钮1句柄; 
HWND 标签1句柄; 
HWND 状态栏句柄;

// 窗口过程函数声明
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// 添加编译函数
void 编译文件(const std::wstring& 文件名) {
    std::wstring 命令 = L"cmd /c chcp 65001>nul && C:/mingw64/bin/g++.exe -fdiagnostics-color=always -O2 \"" + 文件名 + L"\" -o \"" + 文件名.substr(0, 文件名.length()-4) + L".exe\" -lole32 -loleaut32 -lurlmon -luuid -mwindows -lcomctl32";
    STARTUPINFO si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    
    // 设置窗口隐藏属性
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    
    CreateProcess(NULL, (LPWSTR)命令.c_str(), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}

std::vector<std::wstring> cpp文件列表;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // 注册窗口类
    const wchar_t CLASS_NAME[] = L"WindowClass";
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    RegisterClassW(&wc);

    // 初始化通用控件库
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&icex);

    //创建主窗口
    HWND hwnd = CreateWindowEx(
        0,L"WindowClass",
        L"CPP编译助手",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        0,0,
        544,568,
        NULL,NULL,
        hInstance,NULL);

    //创建列表框1
    列表框1句柄 = CreateWindowEx(
        512,L"ListBox",
        L"列表框1",
        WS_CHILD | WS_VISIBLE | WS_BORDER | LBS_NOTIFY | WS_VSCROLL,
        8,16,
        264,496,
        hwnd,(HMENU)列表框1_ID,
        hInstance,NULL);

    WIN32_FIND_DATAW findData;
    HANDLE hFind = FindFirstFileW(L"*.cpp", &findData);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            cpp文件列表.push_back(findData.cFileName);
            SendMessageW(列表框1句柄, LB_ADDSTRING, 0, (LPARAM)findData.cFileName);
        } while (FindNextFileW(hFind, &findData));
        FindClose(hFind);
    }

    //创建按钮1
    按钮1句柄 = CreateWindowEx(
        0,L"Button",
        L"全部编译",1442840576,
        360,48,
        104,48,
        hwnd,(HMENU)按钮1_ID,
        hInstance,NULL);

    //创建标签1
    标签1句柄 = CreateWindowEx(
        0,L"EDIT",
        L"使用说明：\r\n1. 双击左侧列表中的cpp文件可以编译单个文件\r\n2. 点击\"全部编译\"按钮可以编译所有cpp文件\r\n3. 编译状态将在下方状态栏显示",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
        312,120,
        200,382,
        hwnd,(HMENU)标签1_ID,
        hInstance,NULL);

    //创建状态栏
    状态栏句柄 = CreateWindowEx(
        0,STATUSCLASSNAME,
        L"就绪",
        WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
        0,0,0,0,
        hwnd,(HMENU)状态栏_ID,
        hInstance,NULL);

    // 显示窗口
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // 消息循环
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

// 窗口过程函数定义
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_SIZE:
        SendMessage(状态栏句柄, WM_SIZE, 0, 0);
        return 0;

    case WM_COMMAND:
        switch(LOWORD(wParam)){
            case 按钮1_ID:
                SendMessageW(状态栏句柄, SB_SETTEXT, 0, (LPARAM)L"正在编译所有文件...");
                for (const auto& 文件 : cpp文件列表) {
                    编译文件(文件);
                }
                SendMessageW(状态栏句柄, SB_SETTEXT, 0, (LPARAM)L"所有文件编译完成");
                break;
            
            case 列表框1_ID:
                if (HIWORD(wParam) == LBN_DBLCLK) {
                    int idx = SendMessageW(列表框1句柄, LB_GETCURSEL, 0, 0);
                    if (idx != LB_ERR) {
                        wchar_t 文件名[MAX_PATH];
                        SendMessageW(列表框1句柄, LB_GETTEXT, idx, (LPARAM)文件名);
                        SendMessageW(状态栏句柄, SB_SETTEXT, 0, (LPARAM)(L"正在编译: " + std::wstring(文件名)).c_str());
                        编译文件(文件名);
                        SendMessageW(状态栏句柄, SB_SETTEXT, 0, (LPARAM)(L"编译完成: " + std::wstring(文件名)).c_str());
                    }
                }
                break;
        }
        break;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}