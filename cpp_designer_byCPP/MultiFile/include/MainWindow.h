#pragma once

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <richedit.h>
#include <string>
#include <vector>
#include <fstream>
#include <shlobj.h>
#include <shlwapi.h>

// 常量定义
#define MAX_COMPONENTS 100

// 全局变量声明
extern HWND hListView2;
extern HWND 树型框1句柄;
extern HWND 树型框2句柄;
extern HIMAGELIST g_hImageList;
extern HIMAGELIST 工具条图片组句柄;
extern HIMAGELIST 组件图标组句柄;
extern HWND g_currentSelectedControl;
extern WNDPROC g_OldControlProc[MAX_COMPONENTS];
extern int g_ControlCount;
extern bool g_suppressListViewUpdate;
extern const wchar_t* g_currentClassName;
extern DWORD g_currentStyle;
extern DWORD g_currentExStyle;
extern bool 准备绘制;
extern bool 正在绘制;
extern POINT 起始点;
extern RECT 当前方块;
extern std::vector<RECT> 方块列表;
extern int g_componentCounters[MAX_COMPONENTS];
extern HWND g_CodeWindow;
extern HANDLE g_hProcess;

// 函数声明
void InitializeToolbar(HWND hwnd, HINSTANCE hInstance);
void InitializeComponentIcons(HWND hwnd);
void SetupToolbarButtons(HWND hToolBar);

// 窗口过程声明
LRESULT CALLBACK MainWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK 设计子窗口过程(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ControlProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK CodeWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);