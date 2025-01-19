#define UNICODE
#define _UNICODE
#include "Controls.h"
#include "Utils.h"
#include "Resource.h"

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
    const std::string& parentHandle,    // 父窗口句柄
    const std::string& childHandle,     // 子窗口句柄
    const std::string& instanceHandle,  // 实例句柄
    const std::string& additionalData   // 附加数据
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