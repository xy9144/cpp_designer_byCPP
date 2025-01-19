#define UNICODE
#define _UNICODE
#include "MainWindow.h"
#include "Controls.h"
#include "Utils.h"
#include "Resource.h"
#include <gdiplus.h>

// 工具条和状态栏相关全局变量
HWND 工具条1句柄;
HWND 状态栏1句柄;
HIMAGELIST 工具条图片组句柄;
HIMAGELIST 组件图标组句柄;
ULONG_PTR gdiplusToken;

// 全局变量定义
extern HIMAGELIST g_hImageList;

// 从.dat文件加载图片数据的结构体
struct ImageData {
    std::string name;
    std::vector<char> data;
};

// 函数声明
std::vector<ImageData> LoadImagesFromDat(const wchar_t* filename);
std::vector<ImageData> LoadComponentImages();
HBITMAP CreateBitmapFromMemory(const std::vector<char>& data);
void 展开所有节点(HWND 树型框句柄, HTREEITEM 节点句柄);

// 重命名或添加命名空间
namespace MainWindow {
    void SetupToolbarButtons(HWND hwnd) {
        // ... existing code ...
    }

    void InitializeToolbar(HWND hwnd, HINSTANCE hInst) {
        // ... existing code ...
    }
}

// 添加必要的函数声明
void InitializeToolbar(HWND hwnd, HINSTANCE hInstance);
void InitializeComponentIcons(HWND hwnd);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // 初始化 GDI+
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    // 注册主窗口类
    const wchar_t CLASS_NAME[] = L"WindowClass";
    WNDCLASSW wc = {};
    wc.lpfnWndProc = MainWindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    RegisterClassW(&wc);

    // 初始化通用控件库
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_WIN95_CLASSES | ICC_BAR_CLASSES | ICC_TREEVIEW_CLASSES;
    InitCommonControlsEx(&icex);
    //创建主窗口
    HWND hwnd = CreateWindowEx(
        0,L"WindowClass",
        L"生成C++测试窗口",
        WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,
        0,0,
        1000,800,
        NULL,NULL,
        hInstance,NULL);


    状态栏1句柄 = CreateWindowEx(
        0,L"msctls_statusbar32",
        L"状态栏1",1442840576,
        0,0,
        0,0,
        hwnd,(HMENU)状态栏1_ID,
        hInstance,NULL);

    树型框1句柄 = CreateWindowEx(
        512,L"SysTreeView32",
        L"树型框1",WS_VISIBLE|WS_CHILD|TVS_HASLINES|TVS_LINESATROOT|TVS_HASBUTTONS,
        800,50,
        180,240,
        hwnd,(HMENU)树型框1_ID,
        hInstance,NULL);

    树型框2句柄 = CreateWindowEx(
        512,L"SysTreeView32",
        L"树型框2",WS_VISIBLE|WS_CHILD|TVS_HASLINES|TVS_LINESATROOT|TVS_HASBUTTONS,
        800,310,
        180,280,
        hwnd,(HMENU)树型框2_ID,
        hInstance,NULL);

    // 设置树型控件项目间距为26，因为组件图标24x24
    TreeView_SetItemHeight(树型框1句柄, 26);
    TreeView_SetItemHeight(树型框2句柄, 26);
   
    工具条1句柄 = CreateWindowEx(0, TOOLBARCLASSNAME, NULL,
        WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS | CCS_ADJUSTABLE,
        0, 0, 100, 30, hwnd, (HMENU)100, hInstance, NULL);

    // 创建图片列表
    g_hImageList = ImageList_Create(16, 15, ILC_COLOR32 | ILC_MASK, 13, 0);

    // 加载.dat文件中的图片，工具条图标16x15
    auto images = LoadImagesFromDat(L"工具条图标组.dat");
    
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
        {0, ID_TOOLBAR_BUTTON1, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)L"新建"},
        {1, ID_TOOLBAR_BUTTON2, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)L"打开"},
        {2, ID_TOOLBAR_BUTTON3, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)L"保存"},
        {3, ID_TOOLBAR_BUTTON4, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)L"属性"},
        {4, ID_TOOLBAR_BUTTON5, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)L"剪切"},
        {5, ID_TOOLBAR_BUTTON6, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)L"复制"},
        {6, ID_TOOLBAR_BUTTON7, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)L"粘贴"},
        {7, ID_TOOLBAR_BUTTON8, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)L"恢复"},
        {8, ID_TOOLBAR_BUTTON9, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)L"撤销"},
        {9, ID_TOOLBAR_BUTTON10, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)L"运行"},
        {10, ID_TOOLBAR_BUTTON11, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)L"结束"},
        {11, ID_TOOLBAR_BUTTON12, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)L"纯净"},
        {12, ID_TOOLBAR_BUTTON13, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)L"常用"}
    };

    SendMessage(工具条1句柄, TB_ADDBUTTONS, 13, (LPARAM)&tbButtons);
    SendMessage(工具条1句柄, TB_AUTOSIZE, 0, 0);

    // 创建组件图标组
    组件图标组句柄 = ImageList_Create(24, 24, ILC_COLOR32, 39, 0);
    if (组件图标组句柄 == NULL) {
        MessageBox(NULL, L"创建图标组失败", L"错误", MB_OK);
        return 0;
    }

    // 加载组件图标
    auto componentImages = LoadComponentImages();
    
    // 检查是否加载成功
    if (componentImages.empty()) {
        return 0;
    }

    // 添加图片到图片列表并创建树节点，最后一个图片不添加
    HTREEITEM hParent = NULL;
    for (size_t i = 0; i < componentImages.size(); i++) {
        // 从内存创建位图
        Gdiplus::Bitmap* bitmap = nullptr;
        IStream* pStream = NULL;
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, componentImages[i].data.size());
        
        if (hMem) {
            void* pMemData = GlobalLock(hMem);
            memcpy(pMemData, componentImages[i].data.data(), componentImages[i].data.size());
            GlobalUnlock(hMem);

            if (SUCCEEDED(CreateStreamOnHGlobal(hMem, TRUE, &pStream))) {
                bitmap = Gdiplus::Bitmap::FromStream(pStream);
                pStream->Release();
            }
            
            if (bitmap) {
                HBITMAP hBitmap = NULL;
                if (bitmap->GetHBITMAP(Gdiplus::Color(255, 255, 255), &hBitmap) == Gdiplus::Ok) {
                    if (ImageList_Add(组件图标组句柄, hBitmap, NULL) != -1) {
                        // 创建树节点
                        TVINSERTSTRUCT tvInsert = {0};
                        tvInsert.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
                        
                                                // 从文件名中提取显示文本
                        std::wstring name = std::wstring(componentImages[i].name.begin(), componentImages[i].name.end());
                        
                        // 先转换为 UTF-8
                        int len = MultiByteToWideChar(CP_UTF8, 0, componentImages[i].name.c_str(), -1, NULL, 0);
                        if (len > 0) {
                            name.resize(len - 1);  // 减去结尾的 null 终止符
                            MultiByteToWideChar(CP_UTF8, 0, componentImages[i].name.c_str(), -1, &name[0], len);
                        }
                        
                        // 去掉前面的数字
                        size_t startPos = 0;
                        while (startPos < name.length() && iswdigit(name[startPos])) {
                            startPos++;
                        }
                        name = name.substr(startPos);
                        
                        // 去掉 .png 后缀
                        size_t dotPos = name.find(L".png");
                        if (dotPos != std::wstring::npos) {
                            name = name.substr(0, dotPos);
                        }
                        
                        std::vector<wchar_t> nodeText(name.length() + 1);
                        wcscpy_s(nodeText.data(), nodeText.size(), name.c_str());
                        tvInsert.item.pszText = nodeText.data();
                        tvInsert.item.iImage = i;
                        tvInsert.item.iSelectedImage = i;

                        if (i == 0) {
                            tvInsert.hParent = NULL;
                            tvInsert.hInsertAfter = TVI_ROOT;
                            hParent = TreeView_InsertItem(树型框1句柄, &tvInsert);
                        } else if (hParent && i != componentImages.size()-1) {
                            tvInsert.hParent = hParent;
                            tvInsert.hInsertAfter = TVI_LAST;
                            TreeView_InsertItem(树型框1句柄, &tvInsert);
                        }
                    }
                    DeleteObject(hBitmap);
                }
                delete bitmap;
            }
        }
    }

    if (!hParent) {
        MessageBox(NULL, L"创建树节点失败", L"错误", MB_OK);
        return 0;
    }

    // 在创建树型框后添加以下代码
    TreeView_SetImageList(树型框1句柄, 组件图标组句柄, TVSIL_NORMAL);
    TreeView_SetImageList(树型框2句柄, 组件图标组句柄, TVSIL_NORMAL);

    // 在树型框2中添加"窗口"根节点
    TVINSERTSTRUCT tvInsert = {0};
    tvInsert.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    tvInsert.item.pszText = (LPWSTR)L"窗口";
    tvInsert.item.iImage = 38;  // 38，因为图片索引从0开始
    tvInsert.item.iSelectedImage = 38;
    tvInsert.hParent = NULL;
    tvInsert.hInsertAfter = TVI_ROOT;
    HTREEITEM hRoot = TreeView_InsertItem(树型框2句柄, &tvInsert);

    // 在树型框2的根节点后添加展开代码
    TreeView_Expand(树型框2句柄, hRoot, TVE_EXPAND);

    // 展开树型框1的所有节点
    hRoot = TreeView_GetRoot(树型框1句柄);
    展开所有节点(树型框1句柄, hRoot);

    // 显示窗口
    ShowWindow(hwnd, nCmdShow);

    // 消息循环
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // 关闭 GDI+
    Gdiplus::GdiplusShutdown(gdiplusToken);
    return 0;
}

// 图片加载相关函数实现
std::vector<ImageData> LoadImagesFromDat(const wchar_t* filename) {
    std::vector<ImageData> images;
    std::ifstream file(filename, std::ios::binary);
    
    if (!file) {
        MessageBox(NULL, L"无法打开.dat文件", L"错误", MB_OK | MB_ICONERROR);
        return images;
    }

    size_t numFiles;
    file.read(reinterpret_cast<char*>(&numFiles), sizeof(numFiles));

    for (size_t i = 0; i < numFiles; i++) {
        ImageData img;
        
        // 读取文件名长度和文件名
        size_t nameLength;
        file.read(reinterpret_cast<char*>(&nameLength), sizeof(nameLength));
        
        // 使用vector<char>存储UTF-8编码的文件名
        std::vector<char> utf8Name(nameLength);
        file.read(utf8Name.data(), nameLength);
        
        // 转换UTF-8到std::string
        img.name = std::string(utf8Name.begin(), utf8Name.end());

        // 读取文件数据
        size_t dataSize;
        file.read(reinterpret_cast<char*>(&dataSize), sizeof(dataSize));
        img.data.resize(dataSize);
        file.read(img.data.data(), dataSize);

        images.push_back(img);
    }

    return images;
}

std::vector<ImageData> LoadComponentImages()
{
    auto images = LoadImagesFromDat(L"组件图标组.dat");
    if (images.empty()) {
        MessageBox(NULL, L"无法加载组件图标组.dat", L"错误", MB_OK | MB_ICONERROR);
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


void 展开所有节点(HWND 树型框句柄, HTREEITEM 节点句柄)
{
    if (节点句柄 == NULL) return;
    
    // 展开当前节点
    TreeView_Expand(树型框句柄, 节点句柄, TVE_EXPAND);
    
    // 展开所有子节点
    HTREEITEM 子节点 = TreeView_GetChild(树型框句柄, 节点句柄);
    while (子节点 != NULL)
    {
        展开所有节点(树型框句柄, 子节点);
        子节点 = TreeView_GetNextSibling(树型框句柄, 子节点);
    }
}

