#define UNICODE
#define _UNICODE
#include <windows.h>
#include <commctrl.h>
#include <fstream>
#include <sstream>      // 添加这行
#include <vector>
#include <string>
#include <gdiplus.h>
#include <memory>
#include "resource.h"   // 添加这行

#pragma comment(lib, "gdiplus.lib")

// 添加资源ID定义
#define IDR_TOOLBAR_DAT 1001

//定义控件ID
#define ID_TOOLBAR_BUTTON1 200
#define ID_TOOLBAR_BUTTON2 201
#define ID_TOOLBAR_BUTTON3 202
#define ID_TOOLBAR_BUTTON4 203
#define ID_TOOLBAR_BUTTON5 204
#define ID_TOOLBAR_BUTTON6 205
#define ID_TOOLBAR_BUTTON7 206
#define ID_TOOLBAR_BUTTON8 207
#define ID_TOOLBAR_BUTTON9 208
#define ID_TOOLBAR_BUTTON10 209
#define ID_TOOLBAR_BUTTON11 210
#define ID_TOOLBAR_BUTTON12 211
#define ID_TOOLBAR_BUTTON13 212

//定义全局变量
HWND 工具条1句柄;
HIMAGELIST g_hImageList;
ULONG_PTR gdiplusToken;

// 从.dat文件加载图片数据
struct ImageData {
    std::string name;
    std::vector<char> data;
};

std::vector<ImageData> LoadImagesFromDat() {
    std::vector<ImageData> images;
    
    // 从资源加载数据
    HRSRC hRes = FindResource(NULL, MAKEINTRESOURCE(IDR_TOOLBAR_DAT), RT_RCDATA);
    if (!hRes) {
        MessageBox(NULL, L"未找到资源文件", L"错误", MB_OK | MB_ICONERROR);
        return images;
    }

    HGLOBAL hGlobal = LoadResource(NULL, hRes);
    if (!hGlobal) {
        MessageBox(NULL, L"加载资源失败", L"错误", MB_OK | MB_ICONERROR);
        return images;
    }

    LPVOID pData = LockResource(hGlobal);
    DWORD size = SizeofResource(NULL, hRes);

    // 使用内存缓冲区直接读取
    const char* buffer = static_cast<const char*>(pData);
    size_t position = 0;

    // 读取文件数量
    size_t numFiles;
    memcpy(&numFiles, buffer + position, sizeof(numFiles));
    position += sizeof(numFiles);

    for (size_t i = 0; i < numFiles; i++) {
        ImageData img;
        
        // 读取文件名长度
        size_t nameLength;
        memcpy(&nameLength, buffer + position, sizeof(nameLength));
        position += sizeof(nameLength);

        // 读取文件名
        img.name.resize(nameLength);
        memcpy(&img.name[0], buffer + position, nameLength);
        position += nameLength;

        // 读取文件数据大小
        size_t dataSize;
        memcpy(&dataSize, buffer + position, sizeof(dataSize));
        position += sizeof(dataSize);

        // 读取文件数据
        img.data.resize(dataSize);
        memcpy(img.data.data(), buffer + position, dataSize);
        position += dataSize;

        images.push_back(img);
    }

    return images;
}
// 从内存创建位图
HBITMAP CreateBitmapFromMemory(const std::vector<char>& data) {
    IStream* pStream = NULL;
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, data.size());
    if (!hMem) return NULL;

    void* pMemData = GlobalLock(hMem);
    memcpy(pMemData, data.data(), data.size());
    GlobalUnlock(hMem);

    if (FAILED(CreateStreamOnHGlobal(hMem, TRUE, &pStream))) {
        GlobalFree(hMem);
        return NULL;
    }

    // 使用GDI+加载图片
    Gdiplus::Bitmap* pBitmap = Gdiplus::Bitmap::FromStream(pStream);
    pStream->Release();

    if (!pBitmap) return NULL;

    // 创建16x15的缩略图
    Gdiplus::Bitmap* pResizedBitmap = new Gdiplus::Bitmap(16, 15, PixelFormat32bppARGB);
    Gdiplus::Graphics* graphics = Gdiplus::Graphics::FromImage(pResizedBitmap);
    
    graphics->SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
    graphics->DrawImage(pBitmap, 0, 0, 16, 15);
    
    HBITMAP hBitmap = NULL;
    pResizedBitmap->GetHBITMAP(Gdiplus::Color(0, 0, 0, 0), &hBitmap);

    delete graphics;
    delete pResizedBitmap;
    delete pBitmap;

    return hBitmap;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // 初始化GDI+
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    const wchar_t CLASS_NAME[] = L"WindowClass";
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    RegisterClassW(&wc);

    // 初始化通用控件库
    INITCOMMONCONTROLSEX icex = {sizeof(INITCOMMONCONTROLSEX), ICC_BAR_CLASSES};
    InitCommonControlsEx(&icex);

    // 创建主窗口
    HWND hwnd = CreateWindowEx(0, CLASS_NAME, L"工具栏示例", 
        WS_OVERLAPPEDWINDOW, 0, 0, 640, 504, NULL, NULL, hInstance, NULL);

    工具条1句柄 = CreateWindowEx(0, TOOLBARCLASSNAME, NULL,
        WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS | CCS_ADJUSTABLE,
        0, 0, 100, 30, hwnd, (HMENU)100, hInstance, NULL);

    // 创建图片列表
    g_hImageList = ImageList_Create(16, 15, ILC_COLOR32 | ILC_MASK, 13, 0);

    // 加载.dat文件中的图片
    auto images = LoadImagesFromDat();
    
    if (images.empty()) {
        MessageBox(NULL, L"没有找到图片数据", L"错误", MB_OK | MB_ICONERROR);
    }

    // 添加图片到图片列表
    int index = 0;
    for (const auto& img : images) {
        HBITMAP hBmp = CreateBitmapFromMemory(img.data);
        if (hBmp) {
            int result = ImageList_Add(g_hImageList, hBmp, NULL);
            if (result == -1) {
                wchar_t msg[256];
                swprintf(msg, 256, L"添加图片 %d 失败", index + 1);
                MessageBox(NULL, msg, L"错误", MB_OK | MB_ICONERROR);
            }
            DeleteObject(hBmp);
        } else {
            wchar_t msg[256];
            swprintf(msg, 256, L"创建图片 %d 失败", index + 1);
            MessageBox(NULL, msg, L"错误", MB_OK | MB_ICONERROR);
        }
        index++;
    }

    // 设置工具栏
    SendMessage(工具条1句柄, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
    SendMessage(工具条1句柄, TB_SETIMAGELIST, 0, (LPARAM)g_hImageList);

    // 创建工具栏按钮
    TBBUTTON tbButtons[] = {
        {0, ID_TOOLBAR_BUTTON1, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)L"按钮1"},
        {1, ID_TOOLBAR_BUTTON2, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)L"按钮2"},
        {2, ID_TOOLBAR_BUTTON3, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)L"按钮3"},
        {3, ID_TOOLBAR_BUTTON4, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)L"按钮4"},
        {4, ID_TOOLBAR_BUTTON5, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)L"按钮5"},
        {5, ID_TOOLBAR_BUTTON6, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)L"按钮6"},
        {6, ID_TOOLBAR_BUTTON7, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)L"按钮7"},
        {7, ID_TOOLBAR_BUTTON8, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)L"按钮8"},
        {8, ID_TOOLBAR_BUTTON9, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)L"按钮9"},
        {9, ID_TOOLBAR_BUTTON10, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)L"按钮10"},
        {10, ID_TOOLBAR_BUTTON11, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)L"按钮11"},
        {11, ID_TOOLBAR_BUTTON12, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)L"按钮12"},
        {12, ID_TOOLBAR_BUTTON13, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)L"按钮13"}
    };

    SendMessage(工具条1句柄, TB_ADDBUTTONS, 13, (LPARAM)&tbButtons);
    SendMessage(工具条1句柄, TB_AUTOSIZE, 0, 0);

    ShowWindow(hwnd, nCmdShow);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // 清理GDI+
    Gdiplus::GdiplusShutdown(gdiplusToken);
    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
    case WM_DESTROY:
        ImageList_Destroy(g_hImageList);
        PostQuitMessage(0);
        return 0;

    case WM_NOTIFY:
    {
        LPNMHDR pnmh = (LPNMHDR)lParam;
        if (pnmh->code == TTN_GETDISPINFO)
        {
            LPTOOLTIPTEXT lpttt = (LPTOOLTIPTEXT)lParam;
            switch (lpttt->hdr.idFrom)
            {
                case ID_TOOLBAR_BUTTON1:
                    lpttt->lpszText = L"按钮1";
                    break;
                case ID_TOOLBAR_BUTTON2:
                    lpttt->lpszText = L"按钮2";
                    break;
                case ID_TOOLBAR_BUTTON3:
                    lpttt->lpszText = L"按钮3";
                    break;
                case ID_TOOLBAR_BUTTON4:
                    lpttt->lpszText = L"按钮4";
                    break;
                case ID_TOOLBAR_BUTTON5:
                    lpttt->lpszText = L"按钮5";
                    break;
                case ID_TOOLBAR_BUTTON6:
                    lpttt->lpszText = L"按钮6";
                    break;
                case ID_TOOLBAR_BUTTON7:
                    lpttt->lpszText = L"按钮7";
                    break;
                case ID_TOOLBAR_BUTTON8:
                    lpttt->lpszText = L"按钮8";
                    break;
                case ID_TOOLBAR_BUTTON9:
                    lpttt->lpszText = L"按钮9";
                    break;
                 case ID_TOOLBAR_BUTTON10:
                    lpttt->lpszText = L"按钮10";
                    break;
                 case ID_TOOLBAR_BUTTON11:
                    lpttt->lpszText = L"按钮11";
                    break;
                 case ID_TOOLBAR_BUTTON12:
                    lpttt->lpszText = L"按钮12";
                    break;
                 case ID_TOOLBAR_BUTTON13:
                    lpttt->lpszText = L"按钮13";
                    break;
                }
            return 0;
        }
        break;
    }
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}