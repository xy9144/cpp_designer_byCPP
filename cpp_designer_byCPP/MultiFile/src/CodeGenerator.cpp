#define UNICODE
#define _UNICODE
#include "CodeGenerator.h"
#include "Utils.h"
#include "Controls.h"

int  menuCount=0;
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
