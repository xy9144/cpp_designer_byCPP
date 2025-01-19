#define UNICODE
#define _UNICODE
#include "MainWindow.h"
#include "Controls.h"
#include "JsonHandler.h"
#include "Utils.h"
#include "CodeGenerator.h"
#include "Resource.h"


// 全局变量定义
HWND hListView2 = NULL;
HWND 树型框1句柄 = NULL;
HWND 树型框2句柄 = NULL;
HIMAGELIST g_hImageList = NULL;
HWND g_currentSelectedControl = NULL;
WNDPROC g_OldControlProc[MAX_COMPONENTS] = {0};
int g_ControlCount = 0;
bool g_suppressListViewUpdate = false;
const wchar_t* g_currentClassName = L"";
DWORD g_currentStyle = 0;
DWORD g_currentExStyle = 0;
bool 准备绘制 = false;
bool 正在绘制 = false;
POINT 起始点 = {0};
RECT 当前方块 = {0};
std::vector<RECT> 方块列表;
int g_componentCounters[MAX_COMPONENTS] = {0};
HWND g_CodeWindow = NULL;
HANDLE g_hProcess = NULL;

LRESULT CALLBACK MainWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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

        // 创建子窗口
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
            WS_CHILD | WS_VISIBLE | LVS_REPORT,
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
        break;
    }

    case WM_NOTIFY:
    {
        LPNMHDR pnmh = (LPNMHDR)lParam;
        if (pnmh->code == TTN_GETDISPINFO)
        {
            LPTOOLTIPTEXT lpttt = (LPTOOLTIPTEXT)lParam;
            switch (lpttt->hdr.idFrom)
            {
                case ID_TOOLBAR_BUTTON1: lpttt->lpszText = L"新建"; break;
                case ID_TOOLBAR_BUTTON2: lpttt->lpszText = L"打开"; break;
                case ID_TOOLBAR_BUTTON3: lpttt->lpszText = L"保存"; break;
                case ID_TOOLBAR_BUTTON4: lpttt->lpszText = L"属性"; break;
                case ID_TOOLBAR_BUTTON5: lpttt->lpszText = L"剪切"; break;
                case ID_TOOLBAR_BUTTON6: lpttt->lpszText = L"复制"; break;
                case ID_TOOLBAR_BUTTON7: lpttt->lpszText = L"粘贴"; break;
                case ID_TOOLBAR_BUTTON8: lpttt->lpszText = L"恢复"; break;
                case ID_TOOLBAR_BUTTON9: lpttt->lpszText = L"撤销"; break;
                case ID_TOOLBAR_BUTTON10: lpttt->lpszText = L"运行"; break;
                case ID_TOOLBAR_BUTTON11: lpttt->lpszText = L"结束"; break;
                case ID_TOOLBAR_BUTTON12: lpttt->lpszText = L"纯净"; break;
                case ID_TOOLBAR_BUTTON13: lpttt->lpszText = L"常用"; break;
            }
            return 0;
        }
        
        // 处理树型框1的选择变更
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
                准备绘制 = false;
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

        // 处理列表框2的编辑完成消息
        if (pnmh->hwndFrom == hListView2) {
            switch (pnmh->code) {
            case NM_DBLCLK:
                {
                    int clickedItem = ((LPNMITEMACTIVATE)lParam)->iItem;
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
        break;
    }

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

    case WM_SIZING:
    case WM_MOVING:
    {
        RECT* rect = (RECT*)lParam;
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
        HBRUSH hBrush = (HBRUSH)(COLOR_WINDOW + 1);
        FillRect(hdc, &rc, hBrush);
        return 1;
    }

    case WM_SETCURSOR:
    {
        if (准备绘制) {
            SetCursor(LoadCursor(NULL, IDC_CROSS));
            return TRUE;
        }
        break;
    }

    case WM_LBUTTONDOWN:
    case WM_MOUSEMOVE:
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
