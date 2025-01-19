#define UNICODE
#define _UNICODE
#define min1(a,b) (((a) < (b)) ? (a) : (b))
#define max1(a,b) (((a) > (b)) ? (a) : (b))
#define MAX_COMPONENTS 160  // 设置为比最大组件数量大的值，比如40
#include "cJSON.h"
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <gdiplus.h>
#include <vector>
#include <string>
#include <algorithm>
#include <fstream>
#include <memory>
#include <shobjidl.h>

#include <shlwapi.h>
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "ole32.lib")

// 在文件开头添加函数声明
LRESULT CALLBACK 设计子窗口过程(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

//定义控件ID
#define 状态栏1_ID 2
#define 树型框1_ID 3
#define 树型框2_ID 4
#define 设计子窗口_ID 5

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

HWND hListView2;
//定义全局变量，一般为控件句柄

HWND 状态栏1句柄; 
HWND 树型框1句柄; 
HWND 树型框2句柄; 
HIMAGELIST 工具条图片组句柄;
HIMAGELIST 组件图标组句柄;

std::vector<RECT> 方块列表;  // 存储画的方块
int 当前组件类型计数[100] = {0};  // 用于记录每种组件的数量
int 当前组件类型 = 0;  // 添加当前组件类型变量

// 添加绘制相关的全局变量
bool 正在绘制 = false;
POINT 起始点 = {0};
RECT 当前方块 = {0};
bool 准备绘制 = false;  // 添加准备绘制标志

// 添加全局变量
const wchar_t* g_currentClassName = L"";  // 当前选中的控件类名
DWORD g_currentStyle = 0;                 // 当前选中的控件样式
DWORD g_currentExStyle = 0;              // 当前选中的控件扩展样式
WNDPROC g_OldControlProc[100] = {0};  // 存储原始窗口过程
int g_ControlCount = 0;  // 控件计数

// 在全局变量区域添加组件计数器数组
int g_componentCounters[100] = {0};  // 用于记录每种组件的数量

// 添加全局变量
HWND g_currentSelectedControl = NULL;  // 当前选中的控件句柄

// 添加一个标志来控制是否更新列表框
bool g_suppressListViewUpdate = false;

// 在全局变量区域添加新窗口相关声明
WNDPROC g_OldEditProc = NULL;

HWND g_CodeWindow = NULL;
HANDLE g_hProcess = NULL;  // 保存运行进程句柄
// 添加代码窗口的窗口过程声明
LRESULT CALLBACK CodeWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
std::string GenerateCode(HWND designWnd);
std::string ToUTF8Comment(const std::string& comment);

// 窗口过程函数声明
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
// 添加子类化窗口过程声明
LRESULT CALLBACK ControlProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
// 添加更新列表框信息的函数
void UpdateListView2Info(HWND hControl);
void ClearDesignWindow(HWND hwnd);
void LoadComponentsFromJson(HWND hwnd, const wchar_t* filename);
void SaveComponentsToJson(HWND hwnd, const wchar_t* filename);
std::wstring GetExecutablePath();
std::wstring UTF8ToWide(const char* utf8Str);
std::string WideToUTF8(const wchar_t* wstr);

// 在文件开头，在全局变量声明之后，WinMain 之前添加函数声明
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

// 从.dat文件加载图片数据
struct ImageData {
    std::string name;
    std::vector<char> data;
};

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
        img.name.resize(nameLength);
        file.read(&img.name[0], nameLength);

        // 读取文件数据
        size_t dataSize;
        file.read(reinterpret_cast<char*>(&dataSize), sizeof(dataSize));
        img.data.resize(dataSize);
        file.read(img.data.data(), dataSize);

        images.push_back(img);
    }

    return images;
}

// 在LoadImagesFromDat函数后添加这个新函数
std::vector<ImageData> LoadComponentImages() {
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

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // 初始化 GDI+
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    // 注册主窗口类
    const wchar_t CLASS_NAME[] = L"WindowClass";
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProc;
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

    // 设置树型控件项目间距为26
    TreeView_SetItemHeight(树型框1句柄, 26);
    TreeView_SetItemHeight(树型框2句柄, 26);
   
    工具条1句柄 = CreateWindowEx(0, TOOLBARCLASSNAME, NULL,
        WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS | CCS_ADJUSTABLE,
        0, 0, 100, 30, hwnd, (HMENU)100, hInstance, NULL);

    // 创建图片列表
    g_hImageList = ImageList_Create(16, 15, ILC_COLOR32 | ILC_MASK, 13, 0);

    // 加载.dat文件中的图片
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

    // 添加图片到图片列表并创建树节点
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
                        } else if (hParent) {
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

// 添加一个函数来根据图标索引获取组件类型
const wchar_t* GetComponentTypeByImageIndex(int imageIndex)
{
    static const wchar_t* componentTypes[] = {
        L"文件夹",
        L"光标",        // 0
        L"编辑框",      // 1
        L"图片框",      // 2
        L"外形框",      // 3
        L"画板",        // 4
        L"分组框",      // 5
        L"标签",        // 6
        L"按钮",        // 7
        L"选择框",      // 8
        L"单选框",      // 9
        L"组合框",      // 10+2
        L"列表框",      // 11
        L"选择列表框",  // 12
        L"横向滚动条",  // 13
        L"纵向滚动条",  // 14
        L"进度条",      // 15
        L"滑块条",      // 16
        L"选择夹",      // 17
        L"影像框",      // 18
        L"日期框",      // 19
        L"月历",        // 20+2
        L"调节器",      // 21
        L"树型框",      // 22
        L"状态条",      // 23
        L"工具条",      // 24
        L"超级列表框",  // 25
        L"透明标签",    // 26
        L"超级按钮",    // 27
        L"分隔条",      // 28
        L"丰富文本框",  // 29
        L"IP编辑框",    // 30+2
        L"超文本浏览框", // 31
        L"菜单",        // 32
        L"时钟",        // 33
        L"热键框",      // 34
        L"属性框",      // 35
        L"超链接框"     // 36+2
    };

    if (imageIndex >= 0 && imageIndex < sizeof(componentTypes)/sizeof(componentTypes[0])) {
        return componentTypes[imageIndex];
    }
    return L"未知类型";
}

// 窗口过程函数定义
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static HWND hListView = NULL;
    static HWND hChildWnd = NULL;
    switch (uMsg) {
    case WM_CREATE:
            {
                // 注册子窗口类
                WNDCLASS wc = {};
                wc.lpfnWndProc = 设计子窗口过程;
                wc.hInstance = GetModuleHandle(NULL);
                wc.lpszClassName = L"ChildWindowClass";
                RegisterClass(&wc);
    
                // Create child window
                hChildWnd = CreateWindowEx(
                    0,
                    L"ChildWindowClass",
                    L"设计窗口",
                    WS_OVERLAPPEDWINDOW | WS_CHILD | WS_VISIBLE,
                    150, 50,
                    640, 560,
                    hwnd,
                    NULL,
                    GetModuleHandle(NULL),
                    NULL
                );
    
    
                // 创建第1个 ListView 控件（只读）
                HWND hListView1 = CreateWindowEx(
                    WS_EX_CLIENTEDGE,
                    WC_LISTVIEW,
                    L"",
                    WS_CHILD | WS_VISIBLE | LVS_REPORT ,
                    10, 50,
                    50, 200,
                    hwnd,
                    NULL,
                    GetModuleHandle(NULL),
                    NULL
                );
    
                // 添加第1列
                LVCOLUMN lvc1 = {0};
                lvc1.mask = LVCF_TEXT | LVCF_WIDTH;
                lvc1.pszText = const_cast<LPWSTR>(L"属性");
                lvc1.cx = 50;
                ListView_InsertColumn(hListView1, 0, &lvc1);
    
                // 添加行和初始值
                LVITEM lvi1 = {0};
                lvi1.mask = LVIF_TEXT;
                const wchar_t* properties[] = {L"名称", L"类型", L"标题", L"左边", L"顶边", L"宽度", L"高度"};
                
                for(int i = 0; i < 7; i++) {
                    lvi1.iItem = i;
                    lvi1.pszText = (LPWSTR)properties[i];
                    ListView_InsertItem(hListView1, &lvi1);
                }
    
                // 创建第2个 ListView 控件（可编辑）
                hListView2 = CreateWindowEx(
                    WS_EX_CLIENTEDGE,
                    WC_LISTVIEW,
                    L"",
                    WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_EDITLABELS,
                    60, 50,
                    80, 200,
                    hwnd,
                    NULL,
                    GetModuleHandle(NULL),
                    NULL
                );

                    // 设置扩展样式
    ListView_SetExtendedListViewStyle(hListView2, 
        LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

                // 添加第2列
                LVCOLUMN lvc2 = {0};
                lvc2.mask = LVCF_TEXT | LVCF_WIDTH;
                lvc2.pszText = const_cast<LPWSTR>(L"值");
                lvc2.cx = 80;
                ListView_InsertColumn(hListView2, 0, &lvc2);
    
                // 添加行和初始值
                LVITEM lvi2 = {0};
                lvi2.mask = LVIF_TEXT;
                const wchar_t* values[] = {L"主窗口", L"窗口", L"设计窗口", L"0", L"0", L"640", L"560"};
                
                for(int i = 0; i < 7; i++) {
                    lvi2.iItem = i;
                    lvi2.pszText = (LPWSTR)values[i];
                    ListView_InsertItem(hListView2, &lvi2);
                }
    
                // 设置样式
                ListView_SetExtendedListViewStyle(hListView1, 
                    LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_REGIONAL);
                ListView_SetExtendedListViewStyle(hListView2, 
                    LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_REGIONAL);
            }
            return 0;

    case WM_SIZING:
    case WM_MOVING:
    {
        RECT* rect = (RECT*)lParam;
        // 限制最小尺寸
        if (rect->right - rect->left < 100) rect->right = rect->left + 100;
        if (rect->bottom - rect->top < 100) rect->bottom = rect->top + 100;
        return TRUE;
    }

    case WM_DESTROY:
        ImageList_Destroy(工具条图片组句柄);
        ImageList_Destroy(组件图标组句柄);
        PostQuitMessage(0);
        return 0;
    case WM_ERASEBKGND:
    {
        HDC hdc = (HDC)wParam;
        RECT rc;
        GetClientRect(hwnd, &rc);
        HBRUSH hBrush = (HBRUSH)(COLOR_WINDOW + 1); // 使用窗口背景色
        FillRect(hdc, &rc, hBrush);
        return 1; // 返回非零值表示已处理
    }
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
        
        // 处理列表框2的双击和编辑完成消息
        if (pnmh->hwndFrom == hListView2) {
            switch (pnmh->code) {
            case NM_DBLCLK:
                {
                    // 获取双击的项目
                    int clickedItem = ((LPNMITEMACTIVATE)lParam)->iItem;
                    // 只有不是第2行时才允许编辑
                    if (clickedItem != 1) {
                        ListView_EditLabel(hListView2, clickedItem);
                    }
                }
                break;
                
            case LVN_ENDLABELEDIT:
                {
                    NMLVDISPINFO *plvdi = (NMLVDISPINFO*)lParam;
                    if (plvdi->item.pszText != NULL && plvdi->item.pszText[0] != '\0')
                    {
                        // 获取编辑的行号和新值
                        int editedItem = plvdi->item.iItem;
                        wchar_t* newValue = plvdi->item.pszText;

                        // 获取目标窗口句柄
                        HWND targetWnd = g_currentSelectedControl;
                        if (!targetWnd) {
                            targetWnd = FindWindowEx(hwnd, NULL, L"ChildWindowClass", NULL);
                        }
                        if (!targetWnd) return FALSE;

                        switch(editedItem)
                        {
                            case 2: // 标题
                                SetWindowText(targetWnd, newValue);
                                ListView_SetItemText(hListView2, editedItem, 0, newValue);
                                break;
                                
                            case 3: // 左边
                            case 4: // 顶边
                            case 5: // 宽度
                            case 6: // 高度
                                {
                                    RECT rect;
                                    GetWindowRect(targetWnd, &rect);
                                    MapWindowPoints(HWND_DESKTOP, GetParent(targetWnd), (LPPOINT)&rect, 2);
                                    
                                    int left = editedItem == 3 ? (_wtoi(newValue) + 150) : rect.left;
                                    int top = editedItem == 4 ? (_wtoi(newValue) + 50) : rect.top;
                                    int width = editedItem == 5 ? _wtoi(newValue) : (rect.right - rect.left);
                                    int height = editedItem == 6 ? _wtoi(newValue) : (rect.bottom - rect.top);
                                    
                                    g_suppressListViewUpdate = true;
                                    SetWindowPos(targetWnd, NULL, left, top, width, height, SWP_NOZORDER);
                                    g_suppressListViewUpdate = false;
                                    
                                    ListView_SetItemText(hListView2, editedItem, 0, newValue);
                                }
                                break;
                        }
                        return TRUE;
                    }
                    return FALSE;
                }
                break;
            }
        }
        if (pnmh->hwndFrom == 树型框1句柄 && pnmh->code == TVN_SELCHANGED)
        {
            NMTREEVIEW* pNMTreeView = (NMTREEVIEW*)lParam;
            TVITEM item = pNMTreeView->itemNew;
            
            // 获取选中项的文本
            WCHAR szText[256] = {0};
            item.mask = TVIF_TEXT;
            item.pszText = szText;
            item.cchTextMax = 256;
            TreeView_GetItem(树型框1句柄, &item);

            // 检查是否为根节点或第一个子节点
            HTREEITEM hRoot = TreeView_GetRoot(树型框1句柄);
            HTREEITEM hFirstChild = TreeView_GetChild(树型框1句柄, hRoot);
            
            if (item.hItem == hRoot || item.hItem == hFirstChild) {
                准备绘制 = false;  // 禁止绘制
                return 0;
            }

            // 设置控件类型和样式
            const wchar_t* className = L"";
            DWORD style = WS_CHILD | WS_VISIBLE | WS_SIZEBOX;
            DWORD exStyle = 0;

            // 根据选中项设置相应的控件类型
            if (wcscmp(szText, L"编辑框") == 0) {
                className = L"BUTTON";
                style = WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP;
            }
                            else if (wcscmp(szText, L"图片框") == 0) {
                    className = L"BUTTON";
                style = WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP;
                }
                else if (wcscmp(szText, L"外形框") == 0) {
                    className = L"BUTTON";
                style = WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP;
                }
                else if (wcscmp(szText, L"画板") == 0) {
                    className = L"BUTTON";
                style = WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP;
                }
                else if (wcscmp(szText, L"分组框") == 0) {
                    className = L"Button";
                    style |= BS_GROUPBOX| WS_THICKFRAME;
                }
                else if (wcscmp(szText, L"标签") == 0) {
                    className = L"BUTTON";
                style = WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP;
                }
                else if (wcscmp(szText, L"按钮") == 0) {
                    className = L"Button";
                }
                else if (wcscmp(szText, L"选择框") == 0) {
                    className = L"Button";
                    style |= BS_CHECKBOX;
                }
                else if (wcscmp(szText, L"单选框") == 0) {
                    className = L"Button";
                    style |= BS_RADIOBUTTON;
                }
                else if (wcscmp(szText, L"组合框") == 0) {
                    className = L"BUTTON";
                style = WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP;
                }
                else if (wcscmp(szText, L"列表框") == 0) {
                    className = L"BUTTON";
                style = WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP;
                }
                else if (wcscmp(szText, L"选择列表框") == 0) {
                    className = L"BUTTON";
                style = WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP;
                }
                else if (wcscmp(szText, L"横向滚动条") == 0) {
                    className = L"BUTTON";
                style = WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP;
                }
                else if (wcscmp(szText, L"纵向滚动条") == 0) {
                    className = L"BUTTON";
                style = WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP;
                }
                else if (wcscmp(szText, L"进度条") == 0) {
                    className = L"BUTTON";
                style = WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP;
                }
                else if (wcscmp(szText, L"滑块条") == 0) {
                    className = L"BUTTON";
                style = WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP;
                }
                else if (wcscmp(szText, L"选择夹") == 0) {
                    className = L"BUTTON";
                style = WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP;
                }
                else if (wcscmp(szText, L"影像框") == 0) {
                    className = L"BUTTON";
                style = WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP;
                }
                else if (wcscmp(szText, L"日期框") == 0) {
                    className = L"BUTTON";
                style = WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP;
                }
                else if (wcscmp(szText, L"月历") == 0) {
                    className = L"BUTTON";
                style = WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP;
                }
                else if (wcscmp(szText, L"调节器") == 0) {
                    className = L"BUTTON";
                style = WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP;
                }
                else if (wcscmp(szText, L"树型框") == 0) {
                    className = L"BUTTON";
                style = WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP;
                }
                else if (wcscmp(szText, L"状态条") == 0) {
                    className = L"msctls_statusbar32";
                }
                else if (wcscmp(szText, L"工具条") == 0) {
                    className = L"ToolbarWindow32";
                }
                else if (wcscmp(szText, L"超级列表框") == 0) {
                    className = L"BUTTON";
                style = WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP;
                }
                else if (wcscmp(szText, L"透明标签") == 0) {
                    className = L"BUTTON";
                style = WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP;
                }
                else if (wcscmp(szText, L"超级按钮") == 0) {
                    className = L"BUTTON";
                style = WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP;
                }
                else if (wcscmp(szText, L"分隔条") == 0) {
                    className = L"BUTTON";
                style = WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP;
                }
                else if (wcscmp(szText, L"丰富文本框") == 0) {
                    className = L"BUTTON";
                style = WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP;
                }
                else if (wcscmp(szText, L"IP编辑框") == 0) {
                    className = L"BUTTON";
                style = WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP;
                }
                else if (wcscmp(szText, L"超文本浏览框") == 0) {
                    className = L"BUTTON";
                style = WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP;
                }
                else if (wcscmp(szText, L"菜单") == 0) {
                    className = L"BUTTON";
                style = WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP;
                }
                else if (wcscmp(szText, L"时钟") == 0) {
                    className = L"BUTTON";
                style = WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP;
                }
                else if (wcscmp(szText, L"热键框") == 0) {
                    className = L"BUTTON";
                style = WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP;
                }
                else if (wcscmp(szText, L"属性框") == 0) {
                    className = L"BUTTON";
                style = WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP;
                }
                else if (wcscmp(szText, L"超链接框") == 0) {
                    className = L"BUTTON";
                style = WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP;
                }

            if (className[0] != L'\0') {
                g_currentClassName = className;
                g_currentStyle = style;
                g_currentExStyle = exStyle;
                准备绘制 = true;
            }
        }
        break;
    }

    case WM_NCHITTEST:
    {
        POINT pt;
        GetCursorPos(&pt);
        RECT rect;
        GetWindowRect(hChildWnd, &rect);
        ScreenToClient(hwnd, &pt);
        
        // 检查鼠标是否在设计子窗口区域内
        if (pt.x >= 180 && pt.x <= 180 + 480 &&
            pt.y >= 40 && pt.y <= 40 + 360) {
            
            // 定义边缘检测区域的大小（像素）
            const int EDGE_SIZE = 5;
            
            // 检查是否在边缘
            BOOL onRight = (pt.x >= (180 + 480 - EDGE_SIZE));
            BOOL onBottom = (pt.y >= (40 + 360 - EDGE_SIZE));
            
            if (onRight && onBottom) return HTBOTTOMRIGHT;
            if (onRight) return HTRIGHT;
            if (onBottom) return HTBOTTOM;
        }
        return HTCLIENT;
    }
    case WM_SETCURSOR:
    {
        if (准备绘制) {
            SetCursor(LoadCursor(NULL, IDC_CROSS)); // 准备绘制时显示十字光标
        } else {
            return DefWindowProc(hwnd, uMsg, wParam, lParam); // 否则使用默认光标
        }
        return TRUE;
    }
    case WM_LBUTTONDOWN:
    {
        POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        
        // 检查点击是否在设计子窗口内
        RECT rect;
        GetWindowRect(hChildWnd, &rect);
        MapWindowPoints(HWND_DESKTOP, hwnd, (LPPOINT)&rect, 2);
        
        // 检查是否在窗口右下角区域
        const int RESIZE_AREA = 10; // 调整区域大小
        BOOL inResizeArea = (pt.x >= rect.right - RESIZE_AREA && pt.x <= rect.right) &&
                           (pt.y >= rect.bottom - RESIZE_AREA && pt.y <= rect.bottom);
        
        if (inResizeArea) {
            SetCapture(hwnd);
            SetCursor(LoadCursor(NULL, IDC_SIZENWSE));
            // 保存起始位置
            SetProp(hwnd, L"ResizeStartX", (HANDLE)(INT_PTR)pt.x);
            SetProp(hwnd, L"ResizeStartY", (HANDLE)(INT_PTR)pt.y);
            SetProp(hwnd, L"WindowWidth", (HANDLE)(INT_PTR)(rect.right - rect.left));
            SetProp(hwnd, L"WindowHeight", (HANDLE)(INT_PTR)(rect.bottom - rect.top));
        }
        return 0;
    }
    case WM_MOUSEMOVE:
    {
        if (GetCapture() == hwnd) {
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            
            INT_PTR startX = (INT_PTR)GetProp(hwnd, L"ResizeStartX");
            INT_PTR startY = (INT_PTR)GetProp(hwnd, L"ResizeStartY");
            
            if (startX || startY) {
                INT_PTR origWidth = (INT_PTR)GetProp(hwnd, L"WindowWidth");
                INT_PTR origHeight = (INT_PTR)GetProp(hwnd, L"WindowHeight");
                
                int deltaX = pt.x - (int)startX;
                int deltaY = pt.y - (int)startY;
                
                // 计算新尺寸
                int newWidth = (int)origWidth + deltaX;
                int newHeight = (int)origHeight + deltaY;
                
                // 限制最小尺寸
                if (newWidth < 100) newWidth = 100;
                if (newHeight < 100) newHeight = 100;
                
                // 确保设计窗口被选中
                if (g_currentSelectedControl != hChildWnd) {
                    g_currentSelectedControl = hChildWnd;
                }
                
                // 调整窗口大小
                SetWindowPos(hChildWnd, NULL, 
                    0, 0,  // 使用相对坐标(0,0)
                    newWidth, newHeight,
                    SWP_NOZORDER | SWP_NOMOVE);  // 添加 SWP_NOMOVE 保持位置不变

                // 更新列表框2
                UpdateListView2Info(hChildWnd);
                
                // 更新鼠标指针
                SetCursor(LoadCursor(NULL, IDC_SIZENWSE));
            }
        }
        return 0;
    }
    case WM_LBUTTONUP:
    {
        if (GetCapture() == hwnd) {
            ReleaseCapture();
            // 清除属性
            RemoveProp(hwnd, L"ResizeStartX");
            RemoveProp(hwnd, L"ResizeStartY");
            RemoveProp(hwnd, L"WindowWidth");
            RemoveProp(hwnd, L"WindowHeight");
        }
        return 0;
    }
        case WM_KEYDOWN:
        if (wParam == VK_RETURN) {
            int selectedItem = ListView_GetNextItem(hListView2, -1, LVNI_SELECTED);
            if (selectedItem != -1 && selectedItem != 1) {  // 不允许编辑第2行
                ListView_EditLabel(hListView2, selectedItem);
            }
        }
        break;

    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
            case ID_TOOLBAR_BUTTON1:  // 新建按钮
            {
                HWND designWnd = FindWindowEx(hwnd, NULL, L"ChildWindowClass", NULL);
                if (designWnd) {
                    ClearDesignWindow(designWnd);
                }
                return 0;
            }

            case ID_TOOLBAR_BUTTON2:  // 打开按钮
            {
                IFileOpenDialog *pFileOpen;
                HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, 
                                            IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));
                
                if (SUCCEEDED(hr)) {
                    // 设置文件类型过滤器
                    COMDLG_FILTERSPEC fileTypes[] = {
                        { L"JSON Files", L"*.json" }
                    };
                    pFileOpen->SetFileTypes(1, fileTypes);
                    
                    // 设置初始目录为程序运行目录
                    std::wstring exePath = GetExecutablePath();
                    IShellItem *pFolder;
                    hr = SHCreateItemFromParsingName(exePath.c_str(), NULL, IID_PPV_ARGS(&pFolder));
                    if (SUCCEEDED(hr)) {
                        pFileOpen->SetFolder(pFolder);
                        pFolder->Release();
                    }

                    // 显示对话框
                    hr = pFileOpen->Show(hwnd);
                    if (SUCCEEDED(hr)) {
                        IShellItem *pItem;
                        hr = pFileOpen->GetResult(&pItem);
                        if (SUCCEEDED(hr)) {
                            PWSTR filePath;
                            hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &filePath);
                            if (SUCCEEDED(hr)) {
                                HWND designWnd = FindWindowEx(hwnd, NULL, L"ChildWindowClass", NULL);
                                if (designWnd) {
                                    ClearDesignWindow(designWnd);
                                    LoadComponentsFromJson(designWnd, filePath);
                                }
                                CoTaskMemFree(filePath);
                            }
                            pItem->Release();
                        }
                    }
                    pFileOpen->Release();
                }
                return 0;
            }

            case ID_TOOLBAR_BUTTON3:  // 保存按钮
            {
                std::wstring exePath = GetExecutablePath();
                std::wstring jsonPath = exePath + L"\\bms.json";
                HWND designWnd = FindWindowEx(hwnd, NULL, L"ChildWindowClass", NULL);
                if (designWnd) {
                    SaveComponentsToJson(designWnd, jsonPath.c_str());
                }
                return 0;
            }

            case ID_TOOLBAR_BUTTON13:  // 常用按钮
            {
                // 获取设计窗口句柄
                HWND designWnd = FindWindowEx(hwnd, NULL, L"ChildWindowClass", NULL);
                if (!designWnd) return 0;
                
                // 生成代码
                std::string generatedCode = GenerateCode(designWnd);
                
                // 保存到文件
                std::wstring exePath = GetExecutablePath();
                std::wstring filePath = exePath + L"\\test.cpp";
                std::ofstream file(filePath.c_str(), std::ios::binary);
                if (file.is_open()) {
                    // 写入 UTF-8 BOM
                    unsigned char bom[] = { 0xEF, 0xBB, 0xBF };
                    file.write(reinterpret_cast<char*>(bom), sizeof(bom));
                    
                    // 写入生成的代码
                    file.write(generatedCode.c_str(), generatedCode.length());
                    file.close();
                }

                // 注册代码窗口类（如果还没注册）
                WNDCLASSW wc = {};
                if (!GetClassInfoW(GetModuleHandle(NULL), L"CodeWindowClass", &wc)) {
                    wc.lpfnWndProc = CodeWindowProc;
                    wc.hInstance = GetModuleHandle(NULL);
                    wc.lpszClassName = L"CodeWindowClass";
                    RegisterClassW(&wc);
                }

                // 创建代码窗口
                if (!g_CodeWindow) {
                    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
                    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
                    int windowWidth = 800;
                    int windowHeight = 640;
                    int x = (screenWidth - windowWidth) / 2;
                    int y = (screenHeight - windowHeight) / 2;

                    g_CodeWindow = CreateWindowExW(
                        0,
                        L"CodeWindowClass",
                        L"代码窗口",
                        WS_OVERLAPPEDWINDOW,
                        x, y, windowWidth, windowHeight,
                        NULL,
                        NULL,
                        GetModuleHandle(NULL),
                        NULL
                    );

                    if (g_CodeWindow) {
                        ShowWindow(g_CodeWindow, SW_SHOW);
                        UpdateWindow(g_CodeWindow);
                    }
                } else {
                    // 如果窗口已存在，则显示并置前
                    ShowWindow(g_CodeWindow, SW_SHOW);
                    SetForegroundWindow(g_CodeWindow);
                }

                // 在代码窗口的编辑框中显示生成的代码
                if (g_CodeWindow) {
                    HWND hEdit = FindWindowEx(g_CodeWindow, NULL, L"EDIT", NULL);
                    if (hEdit) {
                        std::wstring wideCode = UTF8ToWide(generatedCode.c_str());
                        SetWindowTextW(hEdit, wideCode.c_str());
                    }
                }

                return 0;
            }
        }
        break;
    }
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// 添加设计子窗口的窗口过程
LRESULT CALLBACK 设计子窗口过程(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
        case WM_LBUTTONDOWN:
        {
            if (!准备绘制) {
                // 只有当点击空白区域时才选中设计窗口
                POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
                HWND hitControl = ChildWindowFromPoint(hwnd, pt);
                
                if (hitControl == hwnd) {  // 点击到设计窗口空白处
                    if (g_currentSelectedControl != hwnd) {
                        g_currentSelectedControl = hwnd;
                        UpdateListView2Info(hwnd);
                        SetFocus(hwnd);
                    }
                }
                return 0;
            } else {
                // 检查是否允许绘制
                if (!准备绘制) {
                    return 0;
                }
                正在绘制 = true;
                起始点.x = GET_X_LPARAM(lParam);
                起始点.y = GET_Y_LPARAM(lParam);
                当前方块.left = 起始点.x;
                当前方块.top = 起始点.y;
                当前方块.right = 起始点.x;
                当前方块.bottom = 起始点.y;
                SetCapture(hwnd);
            }
            return 0;
        }

    case WM_MOUSEMOVE:
    {
            // 检查是否允许绘制
            if (!准备绘制 || !正在绘制) {
                return 0;
            }
        // 获取鼠标当前位置
        int currentX = GET_X_LPARAM(lParam);
        int currentY = GET_Y_LPARAM(lParam);
    
        if (正在绘制)
        {
            // 使用无效化区域来处理重绘，而不是直接在DC上绘制
            RECT updateRect;
            // 合并前一个矩形和新矩形的区域以确保完全重绘
            updateRect.left = min1(当前方块.left, currentX);
            updateRect.top = min1(当前方块.top, currentY);
            updateRect.right = max1(当前方块.right, currentX);
            updateRect.bottom = max1(当前方块.bottom, currentY);
            
            // 扩大无效区域以确保完全覆盖
            InflateRect(&updateRect, 1, 1);
            
            // 更新当前矩形
            当前方块.right = currentX;
            当前方块.bottom = currentY;
    
            // 触发重绘
            InvalidateRect(hwnd, &updateRect, TRUE);
            UpdateWindow(hwnd);
        }
    
        // 更新位置信息到 ListView
        RECT rect;
        GetWindowRect(hwnd, &rect);
        HWND parentHwnd = GetParent(hwnd);
        MapWindowPoints(HWND_DESKTOP, parentHwnd, (LPPOINT)&rect, 2);
        
        wchar_t buffer[32];
        
        // 更新左边值（减去150基准值）
        _itow_s(rect.left - 150, buffer, 32, 10);
        ListView_SetItemText(hListView2, 3, 0, buffer);
        
        // 更新顶边值（减去50基准值）
        _itow_s(rect.top - 50, buffer, 32, 10);
        ListView_SetItemText(hListView2, 4, 0, buffer);
        
        // 更新宽度值
        _itow_s(rect.right - rect.left, buffer, 32, 10);
        ListView_SetItemText(hListView2, 5, 0, buffer);
        
        // 更新高度值
        _itow_s(rect.bottom - rect.top, buffer, 32, 10);
        ListView_SetItemText(hListView2, 6, 0, buffer);
    
        return 0;
    }

        case WM_LBUTTONUP:
        {
            if (正在绘制)
            {
                正在绘制 = false;
                ReleaseCapture();

                // 规范化最终矩形坐标
                RECT finalRect = 当前方块;
                if (finalRect.right < finalRect.left)
                {
                    int temp = finalRect.left;
                    finalRect.left = finalRect.right;
                    finalRect.right = temp;
                }
                if (finalRect.bottom < finalRect.top)
                {
                    int temp = finalRect.top;
                    finalRect.top = finalRect.bottom;
                    finalRect.bottom = temp;
                }

                TVITEM item = {0};
                WCHAR szText[256] = {0};
                item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_HANDLE;
                item.hItem = TreeView_GetSelection(树型框1句柄);
                item.pszText = szText;
                item.cchTextMax = 256;
                
                if (TreeView_GetItem(树型框1句柄, &item)) {
                    // 获取组件类型索引
                    int counterIndex = item.iImage;
                    if (counterIndex < 0 || counterIndex >= MAX_COMPONENTS) {
                        MessageBox(NULL, L"无效的组件类型", L"错误", MB_OK);
                        return 0;
                    }

                    // 创建控件前先禁用列表框更新
                    g_suppressListViewUpdate = true;

                    // 创建控件 - 使用原始坐标
                    HWND hControl = CreateWindowEx(
                        g_currentExStyle,
                        g_currentClassName,
                        L"",
                        g_currentStyle,
                        finalRect.left,  // 使用原始坐标，不添加偏移
                        finalRect.top,   // 使用原始坐标，不添加偏移
                        finalRect.right - finalRect.left,
                        finalRect.bottom - finalRect.top,
                        hwnd,
                        NULL,
                        GetModuleHandle(NULL),
                        NULL
                    );

                    if (hControl) {
                        // 为新创建的控件设置子类化处理
                        g_OldControlProc[g_ControlCount] = (WNDPROC)SetWindowLongPtr(hControl, 
                            GWLP_WNDPROC, (LONG_PTR)ControlProc);
                        g_ControlCount++;

                        // 增加该类型组件的计数并设置控件名称
                        g_componentCounters[counterIndex]++;
                        WCHAR controlName[256];
                        wsprintf(controlName, L"%s%d", szText, g_componentCounters[counterIndex]);
                        SetWindowText(hControl, controlName);

                        // 在树型框2中添加新组件节点
                        HTREEITEM hRoot = TreeView_GetRoot(树型框2句柄);
                        if (hRoot) {
                            TVINSERTSTRUCT tvInsert = {0};
                            tvInsert.hParent = hRoot;
                            tvInsert.hInsertAfter = TVI_LAST;
                            tvInsert.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
                            
                            // 使用与树型框1相同的图标索引
                            tvInsert.item.iImage = item.iImage;
                            tvInsert.item.iSelectedImage = item.iImage;
                            
                            // 设置节点文本（组件名称）
                            WCHAR controlName[256];
                            wsprintf(controlName, L"%s%d", szText, g_componentCounters[counterIndex]);
                            tvInsert.item.pszText = controlName;
                            
                            // 插入新节点
                            HTREEITEM hNewItem = TreeView_InsertItem(树型框2句柄, &tvInsert);
                            
                            // 确保父节点展开
                            TreeView_Expand(树型框2句柄, hRoot, TVE_EXPAND);
                        }

                        // 更新列表框信息
                        wchar_t buffer[32];
                        
                        // 更新组件名称和类型
                        ListView_SetItemText(hListView2, 0, 0, controlName);
                        ListView_SetItemText(hListView2, 1, 0, szText);
                        ListView_SetItemText(hListView2, 2, 0, controlName);
                        
                        // 使用原始绘制坐标
                        _itow_s(finalRect.left, buffer, 32, 10);
                        ListView_SetItemText(hListView2, 3, 0, buffer);
                        
                        _itow_s(finalRect.top, buffer, 32, 10);
                        ListView_SetItemText(hListView2, 4, 0, buffer);
                        
                        _itow_s(finalRect.right - finalRect.left, buffer, 32, 10);
                        ListView_SetItemText(hListView2, 5, 0, buffer);
                        
                        _itow_s(finalRect.bottom - finalRect.top, buffer, 32, 10);
                        ListView_SetItemText(hListView2, 6, 0, buffer);

                        // 保存类型到控件属性
                        SetProp(hControl, L"ControlType", (HANDLE)_wcsdup(szText));
                    }

                    // 重新启用列表框更新
                    g_suppressListViewUpdate = false;
                }

                准备绘制 = false;
            }
            return 0;
        }

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            
            // 绘制所有已保存的方块
            for(const auto& rect : 方块列表)
            {
                Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);
            }

            // 如果正在绘制，也绘制当前方块
            if (正在绘制)
            {
                RECT tempRect = 当前方块;
                if (tempRect.right < tempRect.left)
                {
                    int temp = tempRect.left;
                    tempRect.left = tempRect.right;
                    tempRect.right = temp;
                }
                if (tempRect.bottom < tempRect.top)
                {
                    int temp = tempRect.top;
                    tempRect.top = tempRect.bottom;
                    tempRect.bottom = temp;
                }
                Rectangle(hdc, tempRect.left, tempRect.top, tempRect.right, tempRect.bottom);
            }
            
            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_ERASEBKGND:
        {
            // 处理背景擦除
            HDC hdc = (HDC)wParam;
            RECT rect;
            GetClientRect(hwnd, &rect);
            FillRect(hdc, &rect, (HBRUSH)(COLOR_WINDOW+1));
            return 1;
        }

        case WM_WINDOWPOSCHANGED:
        {
            // 当窗口位置或大小改变时更新列表框
            if (hwnd == g_currentSelectedControl) {
                UpdateListView2Info(hwnd);
            }
            break;
        }

        case WM_SIZE:
        case WM_MOVE:
        {
            if (hwnd == g_currentSelectedControl) {
                // 获取窗口绝对位置
                RECT rect;
                GetWindowRect(hwnd, &rect);
                MapWindowPoints(HWND_DESKTOP, GetParent(hwnd), (LPPOINT)&rect, 2);
                
                wchar_t buffer[32];
                // 计算并显示相对位置（绝对位置 - 基准值）
                _itow_s(rect.left - 150, buffer, 32, 10);
                ListView_SetItemText(hListView2, 3, 0, buffer);
                
                _itow_s(rect.top - 50, buffer, 32, 10);
                ListView_SetItemText(hListView2, 4, 0, buffer);
                
                // 更新宽度和高度
                _itow_s(rect.right - rect.left, buffer, 32, 10);
                ListView_SetItemText(hListView2, 5, 0, buffer);
                
                _itow_s(rect.bottom - rect.top, buffer, 32, 10);
                ListView_SetItemText(hListView2, 6, 0, buffer);
            }
            break;
        }

        // 处理列表框编辑完成后的窗口位置更新
        case WM_WINDOWPOSCHANGING:
        {
            if (hwnd == g_currentSelectedControl && !g_suppressListViewUpdate) {
                WINDOWPOS* wp = (WINDOWPOS*)lParam;
                if (!(wp->flags & SWP_NOMOVE)) {
                    // 获取列表框中的相对位置
                    wchar_t buffer[32];
                    ListView_GetItemText(hListView2, 3, 0, buffer, 32);
                    int relativeX = _wtoi(buffer);
                    ListView_GetItemText(hListView2, 4, 0, buffer, 32);
                    int relativeY = _wtoi(buffer);
                    
                    // 转换为绝对位置（相对位置 + 基准值）
                    wp->x = relativeX + 150;
                    wp->y = relativeY + 50;
                }
            }
            break;
        }
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// 添加子类化窗口过程
LRESULT CALLBACK ControlProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_LBUTTONDOWN:
        {
            // 点击组件时更新选中状态
            g_currentSelectedControl = hwnd;
            UpdateListView2Info(hwnd);
            SetFocus(hwnd);
            return 0;  // 返回0以确保消息被处理
        }

        case WM_WINDOWPOSCHANGED:
        {
            // 当控件位置或大小改变时更新列表框
            if (hwnd == g_currentSelectedControl && !g_suppressListViewUpdate) {
                UpdateListView2Info(hwnd);
            }
            break;
        }

        case WM_NCHITTEST:
        {
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            RECT rect;
            GetWindowRect(hwnd, &rect);
            pt.x -= rect.left;
            pt.y -= rect.top;
            
            int width = rect.right - rect.left;
            int height = rect.bottom - rect.top;
            
            const int BORDER = 5;
            
            if (pt.x <= BORDER)
            {
                if (pt.y <= BORDER) return HTTOPLEFT;
                if (pt.y >= height - BORDER) return HTBOTTOMLEFT;
                return HTLEFT;
            }
            if (pt.x >= width - BORDER)
            {
                if (pt.y <= BORDER) return HTTOPRIGHT;
                if (pt.y >= height - BORDER) return HTBOTTOMRIGHT;
                return HTRIGHT;
            }
            if (pt.y <= BORDER) return HTTOP;
            if (pt.y >= height - BORDER) return HTBOTTOM;
            
            // 点击组件时更新选中状态
            g_currentSelectedControl = hwnd;
            UpdateListView2Info(hwnd);
            return HTCAPTION;
        }
    }
    
    WNDPROC oldProc = NULL;
    for(int i = 0; i < g_ControlCount; i++) {
        if(g_OldControlProc[i]) {
            oldProc = g_OldControlProc[i];
            break;
        }
    }
    
    return CallWindowProc(oldProc, hwnd, uMsg, wParam, lParam);
}

// 添加更新列表框信息的函数
void UpdateListView2Info(HWND hControl)
{
    if (!hControl) return;
    
    // 更新组件类型和位置信息
    HWND designWnd = FindWindowEx(GetParent(hControl), NULL, L"ChildWindowClass", NULL);
    if (hControl == designWnd) {
        // 设计窗口处理
        // 保持名称为"主窗口"不变
        ListView_SetItemText(hListView2, 0, 0, L"主窗口");
        ListView_SetItemText(hListView2, 1, 0, L"窗口");
        
        // 获取当前窗口标题（允许编辑）
        WCHAR buffer[256];
        GetWindowText(hControl, buffer, 256);
        ListView_SetItemText(hListView2, 2, 0, buffer);
        
        // 强制位置为0,0
        ListView_SetItemText(hListView2, 3, 0, L"0");
        ListView_SetItemText(hListView2, 4, 0, L"0");
        
        // 更新宽度和高度
        RECT windowRect;
        GetWindowRect(hControl, &windowRect);
        wchar_t numBuffer[32];
        _itow_s(windowRect.right - windowRect.left, numBuffer, 32, 10);
        ListView_SetItemText(hListView2, 5, 0, numBuffer);
        
        _itow_s(windowRect.bottom - windowRect.top, numBuffer, 32, 10);
        ListView_SetItemText(hListView2, 6, 0, numBuffer);
    } else {
        // 其他控件处理保持不变
        WCHAR buffer[256];
        GetWindowText(hControl, buffer, 256);
        ListView_SetItemText(hListView2, 0, 0, buffer);
        
        // 获取控件类型
        WCHAR* controlType = (WCHAR*)GetProp(hControl, L"ControlType");
        if (controlType) {
            ListView_SetItemText(hListView2, 1, 0, controlType);
            ListView_SetItemText(hListView2, 2, 0, buffer);
        }
        
        RECT controlRect;
        GetWindowRect(hControl, &controlRect);
        MapWindowPoints(HWND_DESKTOP, GetParent(hControl), (LPPOINT)&controlRect, 2);
        
        wchar_t numBuffer[32];
        _itow_s(controlRect.left, numBuffer, 32, 10);
        ListView_SetItemText(hListView2, 3, 0, numBuffer);
        
        _itow_s(controlRect.top, numBuffer, 32, 10);
        ListView_SetItemText(hListView2, 4, 0, numBuffer);
        
        _itow_s(controlRect.right - controlRect.left, numBuffer, 32, 10);
        ListView_SetItemText(hListView2, 5, 0, numBuffer);
        
        _itow_s(controlRect.bottom - controlRect.top, numBuffer, 32, 10);
        ListView_SetItemText(hListView2, 6, 0, numBuffer);
    }
}

// 添加辅助函数实现
void ClearDesignWindow(HWND hwnd)
{
    // 枚举并删除所有子窗口
    HWND child = GetWindow(hwnd, GW_CHILD);
    while (child) {
        HWND nextChild = GetWindow(child, GW_HWNDNEXT);
        DestroyWindow(child);
        child = nextChild;
    }

    // 清空树型框2的所有子节点
    HWND tree2 = GetDlgItem(GetParent(hwnd), 树型框2_ID);
    if (tree2) {
        HTREEITEM root = TreeView_GetRoot(tree2);
        if (root) {
            HTREEITEM child = TreeView_GetChild(tree2, root);
            while (child) {
                HTREEITEM next = TreeView_GetNextSibling(tree2, child);
                TreeView_DeleteItem(tree2, child);
                child = next;
            }
        }
    }

    // 重置组件计数器
    memset(g_componentCounters, 0, sizeof(g_componentCounters));
    
    // 刷新窗口
    InvalidateRect(hwnd, NULL, TRUE);
}

void LoadComponentsFromJson(HWND hwnd, const wchar_t* filename)
{
    // 使用 UTF-8 模式读取文件
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        MessageBox(hwnd, L"无法打开JSON文件", L"错误", MB_OK | MB_ICONERROR);
        return;
    }

    // 跳过 UTF-8 BOM（如果存在）
    char bom[3];
    file.read(bom, 3);
    if (!(bom[0] == (char)0xEF && bom[1] == (char)0xBB && bom[2] == (char)0xBF)) {
        file.seekg(0);
    }

    // 读取文件内容到 string
    std::string json_str((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
    file.close();

    // 解析JSON
    cJSON* root = cJSON_Parse(json_str.c_str());
    if (!root) {
        MessageBox(hwnd, L"JSON解析失败", L"错误", MB_OK | MB_ICONERROR);
        return;
    }

    // 转换键名为UTF-8
    std::string utf8Type = WideToUTF8(L"类型");
    std::string utf8Name = WideToUTF8(L"名称");
    std::string utf8Title = WideToUTF8(L"标题");
    std::string utf8Left = WideToUTF8(L"左边");
    std::string utf8Top = WideToUTF8(L"顶边");
    std::string utf8Width = WideToUTF8(L"宽度");
    std::string utf8Height = WideToUTF8(L"高度");

    // 在创建子控件之前，添加一个辅助函数来获取控件类型对应的图标索引
    auto GetImageIndexForType = [hwnd](const std::wstring& type) -> int {
        // 遍历树型框1中的所有项目来匹配类型
        HWND tree1 = GetDlgItem(GetParent(hwnd), 树型框1_ID);
        HTREEITEM hItem = TreeView_GetRoot(tree1);
        if (hItem) {
            hItem = TreeView_GetChild(tree1, hItem); // 跳过根节点
            while (hItem) {
                TVITEM item = {0};
                WCHAR text[256] = {0};
                item.mask = TVIF_TEXT | TVIF_IMAGE;
                item.hItem = hItem;
                item.pszText = text;
                item.cchTextMax = 256;
                
                if (TreeView_GetItem(tree1, &item)) {
                    if (wcscmp(text, type.c_str()) == 0) {
                        return item.iImage;
                    }
                }
                hItem = TreeView_GetNextSibling(tree1, hItem);
            }
        }
        return 0; // 默认图标索引
    };

    // 处理每个组件
    for (int i = 0; i < cJSON_GetArraySize(root); i++) {
        cJSON* componentArray = cJSON_GetArrayItem(root, i);
        if (!componentArray) continue;

        cJSON* component = cJSON_GetArrayItem(componentArray, 0);
        if (!component) continue;

        // 使用UTF-8键名获取组件属性
        cJSON* type = cJSON_GetObjectItem(component, utf8Type.c_str());
        cJSON* name = cJSON_GetObjectItem(component, utf8Name.c_str());
        cJSON* title = cJSON_GetObjectItem(component, utf8Title.c_str());
        cJSON* left = cJSON_GetObjectItem(component, utf8Left.c_str());
        cJSON* top = cJSON_GetObjectItem(component, utf8Top.c_str());
        cJSON* width = cJSON_GetObjectItem(component, utf8Width.c_str());
        cJSON* height = cJSON_GetObjectItem(component, utf8Height.c_str());

        if (!type || !name || !title || !left || !top || !width || !height) continue;

        // UTF-8转换为宽字符
        std::wstring wtype = UTF8ToWide(type->valuestring);
        std::wstring wname = UTF8ToWide(name->valuestring);
        std::wstring wtitle = UTF8ToWide(title->valuestring);

        // 创建组件
        if (wtype == L"窗口") {
            // 设置窗口属性
            SetWindowText(hwnd, wtitle.c_str());
            SetWindowPos(hwnd, NULL, 0, 0, width->valueint, height->valueint, 
                        SWP_NOMOVE | SWP_NOZORDER);
        } else {
            // 创建子控件前，更新组件计数器
            int imageIndex = GetImageIndexForType(wtype);
            if (imageIndex >= 0 && imageIndex < MAX_COMPONENTS) {
                // 从组件名称中提取数字
                int componentNumber = 0;
                std::wstring numberStr = wname.substr(wtype.length());
                if (!numberStr.empty()) {
                    componentNumber = _wtoi(numberStr.c_str());
                    // 更新计数器，确保新的组件编号会更大
                    g_componentCounters[imageIndex] = max1(g_componentCounters[imageIndex], componentNumber);
                }
            }

            // 创建子控件
            HWND hControl = CreateWindowEx(
                0,                          // 扩展样式
                L"BUTTON",                  // 默认使用按钮类
                wtitle.c_str(),            // 窗口标题
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,  // 窗口样式
                left->valueint,             // X 坐标
                top->valueint,              // Y 坐标
                width->valueint,            // 宽度
                height->valueint,           // 高度
                hwnd,                       // 父窗口句柄
                NULL,                       // 菜单句柄
                GetModuleHandle(NULL),      // 实例句柄
                NULL                        // 附加参数
            );

            if (hControl) {
                // 设置控件属性
                SetProp(hControl, L"ControlType", (HANDLE)_wcsdup(wtype.c_str()));
                
                // 子类化处理
                g_OldControlProc[g_ControlCount] = (WNDPROC)SetWindowLongPtr(hControl, 
                    GWLP_WNDPROC, (LONG_PTR)ControlProc);
                g_ControlCount++;

                // 在树型框2中添加节点
                HWND tree2 = GetDlgItem(GetParent(hwnd), 树型框2_ID);
                if (tree2) {
                    HTREEITEM hRoot = TreeView_GetRoot(tree2);
                    if (hRoot) {
                        TVINSERTSTRUCT tvInsert = {0};
                        tvInsert.hParent = hRoot;
                        tvInsert.hInsertAfter = TVI_LAST;
                        tvInsert.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
                        
                        // 获取对应的图标索引
                        int imageIndex = GetImageIndexForType(wtype);
                        
                        tvInsert.item.iImage = imageIndex;
                        tvInsert.item.iSelectedImage = imageIndex;
                        
                        // 创建缓冲区并复制控件名称
                        std::vector<wchar_t> buffer(wname.length() + 1);
                        wcscpy_s(buffer.data(), buffer.size(), wname.c_str());
                        tvInsert.item.pszText = buffer.data();
                        
                        TreeView_InsertItem(tree2, &tvInsert);
                        TreeView_Expand(tree2, hRoot, TVE_EXPAND);
                    }
                }
            }
        }
    }

    cJSON_Delete(root);
    InvalidateRect(hwnd, NULL, TRUE);
}

void SaveComponentsToJson(HWND hwnd, const wchar_t* filename)
{
    cJSON* root = cJSON_CreateArray();
    
    // 添加设计窗口信息
    cJSON* windowArray = cJSON_CreateArray();
    cJSON* windowObj = cJSON_CreateObject();
    
    // 获取窗口属性
    RECT rect;
    GetWindowRect(hwnd, &rect);
    wchar_t title[256];
    GetWindowText(hwnd, title, 256);
    
    // 转换所有键名和值为UTF-8
    std::string utf8Title = WideToUTF8(title);
    std::string utf8ID = WideToUTF8(L"序号");
    std::string utf8Type = WideToUTF8(L"类型");
    std::string utf8Name = WideToUTF8(L"名称");
    std::string utf8Title_key = WideToUTF8(L"标题");
    std::string utf8Left = WideToUTF8(L"左边");
    std::string utf8Top = WideToUTF8(L"顶边");
    std::string utf8Width = WideToUTF8(L"宽度");
    std::string utf8Height = WideToUTF8(L"高度");
    
    cJSON_AddStringToObject(windowObj, utf8ID.c_str(), "1");
    cJSON_AddStringToObject(windowObj, utf8Type.c_str(), WideToUTF8(L"窗口").c_str());
    cJSON_AddStringToObject(windowObj, utf8Name.c_str(), WideToUTF8(L"主窗口").c_str());
    cJSON_AddStringToObject(windowObj, utf8Title_key.c_str(), utf8Title.c_str());
    cJSON_AddNumberToObject(windowObj, utf8Left.c_str(), 0);
    cJSON_AddNumberToObject(windowObj, utf8Top.c_str(), 0);
    cJSON_AddNumberToObject(windowObj, utf8Width.c_str(), rect.right - rect.left);
    cJSON_AddNumberToObject(windowObj, utf8Height.c_str(), rect.bottom - rect.top);
    
    cJSON_AddItemToArray(windowArray, windowObj);
    cJSON_AddItemToArray(root, windowArray);
    
    // 枚举所有子窗口
    int componentId = 2;
    HWND child = GetWindow(hwnd, GW_CHILD);
    while (child) {
        cJSON* componentArray = cJSON_CreateArray();
        cJSON* componentObj = cJSON_CreateObject();
        
        // 获取组件属性
        RECT componentRect;
        GetWindowRect(child, &componentRect);
        MapWindowPoints(HWND_DESKTOP, hwnd, (LPPOINT)&componentRect, 2);
        
        wchar_t componentTitle[256];
        GetWindowText(child, componentTitle, 256);
        
        WCHAR* controlType = (WCHAR*)GetProp(child, L"ControlType");
        
        // 转换为UTF-8
        std::string utf8ComponentTitle = WideToUTF8(componentTitle);
        std::string utf8ControlType = controlType ? WideToUTF8(controlType) : "";
        
        char idStr[16];
        sprintf_s(idStr, sizeof(idStr), "%d", componentId++);
        
        cJSON_AddStringToObject(componentObj, utf8ID.c_str(), idStr);
        cJSON_AddStringToObject(componentObj, utf8Type.c_str(), utf8ControlType.c_str());
        cJSON_AddStringToObject(componentObj, utf8Name.c_str(), utf8ComponentTitle.c_str());
        cJSON_AddStringToObject(componentObj, utf8Title_key.c_str(), utf8ComponentTitle.c_str());
        cJSON_AddNumberToObject(componentObj, utf8Left.c_str(), componentRect.left);
        cJSON_AddNumberToObject(componentObj, utf8Top.c_str(), componentRect.top);
        cJSON_AddNumberToObject(componentObj, utf8Width.c_str(), componentRect.right - componentRect.left);
        cJSON_AddNumberToObject(componentObj, utf8Height.c_str(), componentRect.bottom - componentRect.top);
        
        cJSON_AddItemToArray(componentArray, componentObj);
        cJSON_AddItemToArray(root, componentArray);
        
        child = GetWindow(child, GW_HWNDNEXT);
    }
    
    // 保存文件
    char* jsonStr = cJSON_Print(root);
    std::ofstream file(filename, std::ios::binary);
    if (file.is_open()) {
        // 写入 UTF-8 BOM
        unsigned char bom[] = { 0xEF, 0xBB, 0xBF };
        file.write(reinterpret_cast<char*>(bom), sizeof(bom));
        
        // 写入 JSON 内容
        file.write(jsonStr, strlen(jsonStr));
        file.close();
    }
    
    free(jsonStr);
    cJSON_Delete(root);
}

// 添加UTF-8和宽字符转换辅助函数
std::wstring UTF8ToWide(const char* utf8Str) {
    if (!utf8Str) return L"";
    
    int len = MultiByteToWideChar(CP_UTF8, 0, utf8Str, -1, NULL, 0);
    if (len == 0) return L"";
    
    std::vector<wchar_t> wstr(len);
    if (MultiByteToWideChar(CP_UTF8, 0, utf8Str, -1, wstr.data(), len) == 0) {
        return L"";
    }
    return std::wstring(wstr.data());
}

std::string WideToUTF8(const wchar_t* wstr) {
    if (!wstr) return "";
    
    int len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
    if (len == 0) return "";
    
    std::vector<char> utf8(len);
    if (WideCharToMultiByte(CP_UTF8, 0, wstr, -1, utf8.data(), len, NULL, NULL) == 0) {
        return "";
    }
    return std::string(utf8.data());
}

std::wstring GetExecutablePath()
{
    wchar_t path[MAX_PATH];
    GetModuleFileName(NULL, path, MAX_PATH);
    PathRemoveFileSpec(path);
    return std::wstring(path);
}



// 添加代码窗口的窗口过程
LRESULT CALLBACK CodeWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static HWND hToolBar = NULL;
    static HWND hEdit = NULL;

    switch (uMsg)
    {
        case WM_CREATE:
        {
            // 创建工具条
            hToolBar = CreateWindowEx(0, TOOLBARCLASSNAME, NULL,
                WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS,
                0, 0, 0, 0, hwnd, (HMENU)100, GetModuleHandle(NULL), NULL);

            // 设置工具条按钮大小和图片列表
            SendMessage(hToolBar, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
            SendMessage(hToolBar, TB_SETIMAGELIST, 0, (LPARAM)g_hImageList);

            // 创建工具条按钮
            TBBUTTON tbButtons[] = {
                {8, 1001, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)L"撤销"},
                {2, 1002, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)L"保存"},
                {9, 1003, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)L"运行"},
                {10, 1004, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, (INT_PTR)L"结束"}
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
                case 1001:  // 撤销按钮
                {
                    SendMessage(hEdit, WM_UNDO, 0, 0);
                    return 0;
                }

                case 1002:  // 保存按钮
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

                case 1003:  // 运行按钮
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

                case 1004:  // 结束按钮
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

// 辅助函数：替换字符串
std::string ReplaceString(
    std::string subject,
    const std::string& search,
    const std::string& replace
) {
    size_t pos = 0;
    while ((pos = subject.find(search, pos)) != std::string::npos) {
        subject.replace(pos, search.length(), replace);
        pos += replace.length();
    }
    return subject;
}
// 添加UTF-8转换辅助函数
std::string ToUTF8Comment(const std::string& comment) {
    std::wstring wide = UTF8ToWide(comment.c_str());
    return WideToUTF8(wide.c_str());
}
// 添加 ToUTF8String 函数用于转换标题文本
std::string ToUTF8String(const std::string& input)
{
    // 将 ANSI 字符串转换为 UTF-16
    int wlen = MultiByteToWideChar(CP_ACP, 0, input.c_str(), -1, NULL, 0);
    std::vector<wchar_t> wstr(wlen);
    MultiByteToWideChar(CP_ACP, 0, input.c_str(), -1, wstr.data(), wlen);

    // 将 UTF-16 转换为 UTF-8
    int len = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), -1, NULL, 0, NULL, NULL);
    std::vector<char> str(len);
    WideCharToMultiByte(CP_UTF8, 0, wstr.data(), -1, str.data(), len, NULL, NULL);

    return std::string(str.data());
}

// 创建控件的函数
std::string CreateControl(
    const std::string& type,     // 控件类型
    const std::string& name,     // 控件名称
    const std::string& title,    // 控件标题
    const std::string& style,    // 控件风格
    const std::string& exStyle,  // 扩展风格
    int left,                    // 左边位置
    int top,                     // 顶边位置
    int width,                   // 宽度
    int height,                  // 高度
    const std::string& parentHandle = "NULL",    // 父窗口句柄
    const std::string& childHandle = "NULL",     // 子窗口句柄
    const std::string& instanceHandle = "hInstance", // 实例句柄
    const std::string& additionalData = "NULL"   // 附加数据
) {
    std::string className;
    // 定义控件模板
    std::string createCode = "   控件句柄 = CreateWindowEx(\r\n"
        "      扩展风格,L\"类名\",\r\n"
        "      L\"标题\",风格,\r\n"
        "      左边,顶边,\r\n"
        "      宽度,高度,\r\n"
        "      父窗口句柄,菜单或子窗口句柄,\r\n"
        "      实例句柄,附加数据);\r\n";

    // 根据控件类型设置对应的类名
if (type == "窗口") {
        className = "WindowClass";
    } else if (type == "编辑框") {
        className = "Edit";
    } else if (type == "图片框") {
        className = "WC_STATIC";
    } else if (type == "外形框") {
        className = "Edit Control";
    } else if (type == "画板") {
        className = "Edit Control";
    } else if (type == "分组框") {
        className = "Button";
    } else if (type == "标签") {
        className = "Static";
    } else if (type == "按钮") {
        className = "Button";
    } else if (type == "选择框") {
        className = "Button";
    } else if (type == "单选框") {
        className = "Button";
    } else if (type == "组合框") {
        className = "ComboBox";
    } else if (type == "列表框") {
        className = "ListBox";
    } else if (type == "选择列表框") {
        className = "WC_COMBOBOX";
    } else if (type == "横向滚动条") {  // 水平
        className = "ScrollBar";
    } else if (type == "纵向滚动条") {  // 竖直
        className = "ScrollBar";
    } else if (type == "进度条") {
        className = "msctls_progress32";
    } else if (type == "滑块条") {
        className = "msctls_trackbar32";
    } else if (type == "选择夹") {
        className = "SysTabControl32";
    } else if (type == "影像框") {
        className = "SysAnimate32";
    } else if (type == "日期框") {
        className = "SysDateTimePick32";
    } else if (type == "月历") {
        className = "SysMonthCal32";
    } else if (type == "调节器") {
        className = "msctls_updown32";
    } else if (type == "树型框") {
        className = "SysTreeView32";
    } else if (type == "状态栏") {
        className = "msctls_statusbar32";
    } else if (type == "工具条") {
        className = "ToolbarWindow32";
    } else if (type == "超级列表框") {
        className = "SysListView32";
    } else if (type == "透明标签") {
        className = "Static";
    } else if (type == "超级按钮") {
        className = "Button";
    } else if (type == "分隔条") {
        className = "Edit Control";
    } else if (type == "丰富文本框") {
        className = "RichEdit50w";
    } else if (type == "IP编辑框") {
        className = "SysIPAddress32";
    } else if (type == "超文本浏览框") {
        className = "Static";
    } else if (type == "菜单") {
        className = "Edit Control";
    } else if (type == "时钟") {
        className = "Edit Control";
    } else if (type == "热键框") {
        className = "msctls_hotkey32";
    } else if (type == "属性框") {
        className = "Edit Control";
    } else if (type == "超链接框") {
        className = "Edit Control";
    }

    // 替换控件句柄
    if (type == "窗口") {
        createCode = ReplaceString(createCode, "控件句柄", "hwnd");
    } else {
        createCode = ReplaceString(createCode, "控件句柄", name + ToUTF8String("句柄"));
    }

    // 替换其他参数
    createCode = ReplaceString(createCode, "扩展风格", exStyle);
    
    // 特殊处理图片框和选择列表框的类名
    if (type == "图片框" || type == "选择列表框") {
        createCode = ReplaceString(createCode, "L\"类名\"", className);
    } else {
        createCode = ReplaceString(createCode, "类名", className);
    }

    // 替换其他参数
    createCode = ReplaceString(createCode, "标题", title);
    createCode = ReplaceString(createCode, "风格", style);
    createCode = ReplaceString(createCode, "左边", std::to_string(left));
    createCode = ReplaceString(createCode, "顶边", std::to_string(top));
    createCode = ReplaceString(createCode, "宽度", std::to_string(width));
    createCode = ReplaceString(createCode, "高度", std::to_string(height));
    createCode = ReplaceString(createCode, "父窗口句柄", parentHandle);
    createCode = ReplaceString(createCode, "菜单或子窗口句柄", childHandle);
    createCode = ReplaceString(createCode, "实例句柄", instanceHandle);
    createCode = ReplaceString(createCode, "附加数据", additionalData);

    return createCode;
}
int menuCount=0;
// 添加生成代码的函数实现
std::string GenerateCode(HWND designWnd)
{
        // 局部变量声明
    std::vector<HWND> controls;
    std::vector<HWND> tempControls;
    std::string globalVars;
    std::string createCode;
    std::string codeHeader;
    std::string mainWindowCode;
    std::string finalCode;
    
    // 常量定义 - 使用 ToUTF8Comment 处理中文字符串
    const std::string CREATE_MENU_GLOBAL_VARS = ToUTF8Comment(R"(    HMENU hMenu;
    HMENU hFileMenu;)");
    
    const std::string CREATE_MENU_EXAMPLE = ToUTF8Comment(
        "    hMenu = CreateMenu();\r\n"
        "    hFileMenu = CreatePopupMenu();\r\n"
        "    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hFileMenu, L\"文件(&F)\");\r\n"
        "    AppendMenu(hFileMenu, MF_STRING, 211, L\"新建(&N)\");\r\n"
        "    AppendMenu(hFileMenu, MF_STRING, 212, L\"打开(&O)\");\r\n"
        "    AppendMenu(hFileMenu, MF_SEPARATOR, 0, NULL);\r\n"
        "    AppendMenu(hFileMenu, MF_STRING, 213, L\"退出(&X)\");\r\n\r\n");

    const std::string MACRO_DEFINES = R"(#define UNICODE
#define _UNICODE)";

    const std::string HEADER_FILES = R"(#include <windows.h>
#include <commctrl.h>)";

    const char* PICTUREBOX_EXAMPLE_BEFORE_CREATE = 
        "HIMAGELIST hImageList = ImageList_Create(32,32,ILC_COLOR32,1,0);\r\n"
        "HANDLE hIcon = LoadImage(hInstance,L\"1.ico\",IMAGE_ICON,32,32,LR_LOADFROMFILE);\r\n"
        "ImageList_AddIcon(hImageList,(HICON)hIcon);\r\n";

    // 临时变量
    char title[256] ;
    RECT rect;
    int width;
    int height;
    HWND child;
    std::string controlID,codeMid,codeTail;
    std::string childHandle, parentHandle, instanceHandle, additionalData;
    
    // 获取主窗口信息
    GetWindowTextA(designWnd, title, 256);
    
    GetWindowRect(designWnd, &rect);
    width = rect.right - rect.left;
    height = rect.bottom - rect.top;
    
    // 枚举子窗口
    child = GetWindow(designWnd, GW_CHILD);
    
    while (child) {
        if (GetProp(child, L"ControlType")) {
            tempControls.push_back(child);
        }
        child = GetWindow(child, GW_HWNDNEXT);
    }
    
    // 反转控件顺序以保持原始创建顺序
    for (int i = tempControls.size() - 1; i >= 0; i--) {
        controls.push_back(tempControls[i]);
    }
    
    // 生成头部代码，使用UTF-8字符串
     codeHeader = 
        "#include <windows.h>\r\n"
        "#include <commctrl.h>\r\n"
        "#include <richedit.h>\r\n"
        "#pragma comment(lib, \"comctl32.lib\")\r\n\r\n";

    // 创建主窗口
    createCode += "\n";
    if (menuCount > 0) {
        globalVars += CREATE_MENU_GLOBAL_VARS + "\r\n";
        createCode = "\n\n" + std::string(CREATE_MENU_EXAMPLE) + createCode + "\r\n";
        mainWindowCode = CreateControl("窗口", "主窗口", ToUTF8String(title), "WS_OVERLAPPEDWINDOW", 
                                 "0", 0, 0, width, height, "NULL", "hMenu", "hInstance", "NULL");
    } else {
        mainWindowCode = CreateControl("窗口", "主窗口", ToUTF8String(title), "WS_OVERLAPPEDWINDOW", 
                                 "0", 0, 0, width, height, "NULL", "NULL", "hInstance", "NULL");
    }
    
    // 添加窗口居中代码，使用UTF-8字符串
    mainWindowCode = 
        ToUTF8String("    // 创建居中的主窗口\r\n"
        "    int x = (GetSystemMetrics(SM_CXSCREEN) - " + std::to_string(width) + ") / 2;\r\n"
        "    int y = (GetSystemMetrics(SM_CYSCREEN) - " + std::to_string(height) + ") / 2;\r\n\r\n") +
        mainWindowCode;
    
    createCode += mainWindowCode + "\r\n";
    
    // 处理每个控件
    for (size_t i = 0; i < controls.size(); i++) {
        HWND hControl = controls[i];
        
        // 获取控件信息
        char controlTitle[256];
        GetWindowTextA(hControl, controlTitle, 256);
        
        WCHAR* controlType = (WCHAR*)GetProp(hControl, L"ControlType");
        if (!controlType) continue;
        
        std::string type = WideToUTF8(controlType);
        std::string name = WideToUTF8(controlType) + std::to_string(i + 1);
        
        // 获取控件位置和大小
        RECT controlRect;
        GetWindowRect(hControl, &controlRect);
        MapWindowPoints(HWND_DESKTOP, designWnd, (LPPOINT)&controlRect, 2);
        int extendedStyle = 0;
        width = rect.right - rect.left;
        height = rect.bottom - rect.top;
        std::string parentHandle = "hwnd";
        bool hasSameClass;
        int sameClassIndex;
        
           // 处理同类索引
            hasSameClass = false;
            sameClassIndex = std::stoi(ReplaceString(name, type, ""));
            if (sameClassIndex != 1) {
                hasSameClass = true;
            }

            // 跳过特殊控件
            if (type == "代码编辑框" || type == "分隔条" || type == "菜单") {
                continue;
            }

            // 添加富文本框头文件
            if (type == "丰富文本框" && sameClassIndex == 1) {
                codeHeader += "#include <richedit.h>\r\n";
            }

            // 处理工具条ID
            if (type == "工具条") {
                childHandle = "(HMENU)100";
            } else {
                controlID += "#define " + name + "_ID " + std::to_string(i+1) + "\r\n";
                childHandle = "(HMENU)" + name + "_ID";
            }

            instanceHandle = "hInstance";
            additionalData = "NULL";
            globalVars += "HWND " + name + ToUTF8String("句柄;\r\n");

            // 设置控件风格
            std::string style;
               if (type == "编辑框") {
        style = "WS_CHILD | WS_VISIBLE | ES_MULTILINE | WS_VSCROLL | ES_NOHIDESEL";
    }
    else if (type == "图片框") {
        style = "SS_ICON | SS_CENTERIMAGE | WS_CHILD | WS_VISIBLE";
    }
    else if (type == "外形框") {
        style = "WS_CHILD | WS_VISIBLE";
    }
    else if (type == "分组框") {
        style = "WS_CHILD | WS_VISIBLE | BS_GROUPBOX";
    }
    else if (type == "标签") {
        style = "WS_CHILD | WS_VISIBLE";
    }
    else if (type == "按钮") {
        style = "WS_CHILD | WS_VISIBLE";
    }
    else if (type == "单选框") {
        style = "WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON";
    }
    else if (type == "选择框") {
        style = "WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX";
    }
    else if (type == "组合框") {
        style = "WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST";
        height = 200;
    }
    else if (type == "列表框") {
        style = "WS_CHILD | WS_VISIBLE | WS_BORDER | LBS_NOTIFY";
    }
    else if (type == "选择列表框") {
        style = "WS_CHILD | WS_VISIBLE | WS_BORDER | CBS_DROPDOWNLIST";
        height = 200;
    }
    else if (type == "横向滚动条") {
        style = "WS_CHILD | WS_VISIBLE | SBS_HORZ";
    }
    else if (type == "纵向滚动条") {
        style = "WS_CHILD | WS_VISIBLE | SBS_VERT";
    }
    else if (type == "进度条") {
        style = "WS_CHILD | WS_VISIBLE";
    }
    else if (type == "滑块条") {
        style = "WS_CHILD | WS_VISIBLE | TBS_HORZ";
    }
    else if (type == "选择夹") {
        style = "WS_CHILD | WS_VISIBLE";
    }
    else if (type == "影像框") {
        style = "WS_CHILD | WS_VISIBLE | ACS_CENTER";
    }
    else if (type == "日期框") {
        style = "WS_CHILD | WS_VISIBLE | WS_BORDER";
    }
    else if (type == "月历") {
        style = "WS_CHILD | WS_VISIBLE | WS_BORDER";
    }
    else if (type == "调节器") {
        style = "WS_CHILD | WS_VISIBLE | UDS_SETBUDDYINT";
    }
    else if (type == "树型框") {
        style = "WS_VISIBLE | WS_CHILD | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS";
    }
    else if (type == "状态栏") {
        style = "WS_VISIBLE | WS_CHILD | SBARS_SIZEGRIP";
        rect.left = 0;
        rect.top = 0;
        width = 0;
        height = 0;
    }
    else if (type == "工具条") {
        style = "WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS";
        rect.left = 0;
        rect.top = 0;
        width = 0;
        height = 0;
    }
    else if (type == "超级列表框") {
        style = "WS_CHILD | WS_VISIBLE | LVS_REPORT";
    }
    else if (type == "透明标签") {
        style = "WS_CHILD | WS_VISIBLE | SS_NOTIFY";
    }
    else if (type == "超级按钮") {
        style = "WS_CHILD | WS_VISIBLE | BS_OWNERDRAW";
    }
    else if (type == "丰富文本框") {
        style = "WS_CHILD | WS_VISIBLE | ES_MULTILINE | WS_VSCROLL | ES_NOHIDESEL";
    }
    else if (type == "IP编辑框") {
        style = "WS_CHILD | WS_VISIBLE";
    }
    else if (type == "超文本浏览框") {
        style = "WS_CHILD | WS_VISIBLE | WS_BORDER";
    }
    else if (type == "热键框") {
        style = "WS_CHILD | WS_VISIBLE";
    }

    // 处理无实例句柄的控件
    if (type == "单选框" || type == "选择框") {
        instanceHandle = "NULL";
    }

            // 创建控件代码
            if (type == "调节器" && sameClassIndex == 1) {
                createCode += ToUTF8Comment("    //创建" + name + "默认竖直，改水平方向加| UDS_HORZ，关联编辑框等加| UDS_ALIGNRIGHT\r\n");
            } else {
                createCode += ToUTF8String("    //创建") + name + "\r\n";
            }

            if (type == "图片框" && sameClassIndex == 1) {
                createCode += "\r\n" + std::string(PICTUREBOX_EXAMPLE_BEFORE_CREATE) + "\r\n";
            }

            if (type != "菜单" && type != "画板" && type != "外形框" && type != "时钟") {
                createCode += CreateControl(type, name, ToUTF8String(controlTitle), style, std::to_string(extendedStyle),
                                         rect.left, rect.top, width, height,
                                         parentHandle, childHandle, instanceHandle, additionalData) + "\r\n\r\n";
            }
        
        }

    // 组合最终代码，使用UTF-8字符串
    finalCode = 
        codeHeader + "\r\n" + 
        controlID + "\r\n" + 
        globalVars + "\r\n" + 
        ToUTF8String(
            "int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpCmdLine,int nCmdShow)\n"
            "{\r\n"
            "    WNDCLASSEX wc = {0};\r\n"
            "    wc.cbSize = sizeof(WNDCLASSEX);\r\n"
            "    wc.lpfnWndProc = DefWindowProc;\r\n"
            "    wc.hInstance = hInstance;\r\n"
            "    wc.lpszClassName = L\"WindowClass\";\r\n"
            "    RegisterClassEx(&wc);\r\n") +
        createCode +
        ToUTF8String("\r\n"
            "    ShowWindow(hwnd, nCmdShow);\r\n"
            "    UpdateWindow(hwnd);\r\n\r\n"
            "    MSG msg = {0};\r\n"
            "    while (GetMessage(&msg, NULL, 0, 0))\r\n"
            "    {\r\n"
            "        TranslateMessage(&msg);\r\n"
            "        DispatchMessage(&msg);\r\n"
            "    }\r\n\r\n"
            "    return msg.wParam;\r\n"
            "}\r\n");
    
    return finalCode;
}






