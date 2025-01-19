#define UNICODE
#define _UNICODE
#include "JsonHandler.h"
#include "Utils.h"
#include "Resource.h"

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