#define UNICODE
#define _UNICODE
#define min(a,b) (((a) < (b)) ? (a) : (b))
#define max(a,b) (((a) > (b)) ? (a) : (b))
#include <windows.h>
#include <CommCtrl.h>
#include <shlobj.h>
#include <string>
#include <stdio.h>
#include <vector>
#include <filesystem>
#include <fstream>
#include <algorithm>  // 添加这个头文件用于transform
#include <cctype>     // 添加这个头文件用于tolower

namespace fs = std::filesystem;

// 全局变量
HWND hList = NULL;
std::vector<fs::path> imageFiles;
std::wstring currentFolder;

// 窗口过程函数声明
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

// 获取文件名中的数字
int getNumberFromFilename(const std::filesystem::path& path) {
    std::wstring filename = path.stem().wstring(); // 获取不含后缀的文件名
    std::wstring numbers;
    
    // 提取数字部分
    for (wchar_t c : filename) {
        if (std::iswdigit(c)) {
            numbers += c;
        }
    }
    
    // 如果找到数字，转换为整数
    if (!numbers.empty()) {
        try {
            return std::stoi(numbers);
        } catch (...) {
            return INT_MAX; // 转换失败时返回最大值
        }
    }
    
    return INT_MAX; // 没有数字时返回最大值
}

// 字符串转换函数
std::string wstring_to_string(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string str(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &str[0], size_needed, NULL, NULL);
    return str;
}

// 检查文件是否为图片
bool isImageFile(const fs::path& path) {
    std::wstring ext = path.extension().wstring();
    return _wcsicmp(ext.c_str(), L".jpg") == 0 ||
           _wcsicmp(ext.c_str(), L".jpeg") == 0 ||
           _wcsicmp(ext.c_str(), L".png") == 0 ||
           _wcsicmp(ext.c_str(), L".bmp") == 0 ||
           _wcsicmp(ext.c_str(), L".ico") == 0 ||
           _wcsicmp(ext.c_str(), L".gif") == 0;
}

// 添加这个辅助函数来检查并创建输出文件
bool createOutputFile(const std::wstring& outputPath, std::ofstream& out) {
    try {
        // 获取文件所在目录
        fs::path filePath(outputPath);
        fs::path directory = filePath.parent_path();

        // 确保目录存在
        if (!fs::exists(directory)) {
            if (!fs::create_directories(directory)) {
                MessageBoxW(NULL, L"无法创建输出目录！", L"错误", MB_OK | MB_ICONERROR);
                return false;
            }
        }

        // 转换路径为本地编码
        std::string localPath;
        int requiredSize = WideCharToMultiByte(CP_ACP, 0, outputPath.c_str(), -1, NULL, 0, NULL, NULL);
        if (requiredSize > 0) {
            localPath.resize(requiredSize);
            WideCharToMultiByte(CP_ACP, 0, outputPath.c_str(), -1, &localPath[0], requiredSize, NULL, NULL);
            localPath.pop_back(); // 移除多余的null终止符
        } else {
            MessageBoxW(NULL, L"路径转换失败！", L"错误", MB_OK | MB_ICONERROR);
            return false;
        }

        // 尝试打开文件
        out.open(localPath, std::ios::binary);
        if (!out.is_open()) {
            std::wstring errorMsg = L"无法创建文件：" + outputPath + L"\n"
                                  L"请检查：\n"
                                  L"1. 是否有写入权限\n"
                                  L"2. 文件是否被其他程序占用\n"
                                  L"3. 路径是否包含特殊字符";
            MessageBoxW(NULL, errorMsg.c_str(), L"错误", MB_OK | MB_ICONERROR);
            return false;
        }
        return true;
    }
    catch (const std::exception& e) {
        std::string errMsg = "创建文件时发生错误: ";
        errMsg += e.what();
        MessageBoxA(NULL, errMsg.c_str(), "错误", MB_OK | MB_ICONERROR);
        return false;
    }
}

void packImages(const std::vector<fs::path>& files, const std::wstring& outputPath) {
    std::ofstream out;
    if (!createOutputFile(outputPath, out)) {
        return;
    }

    try {
        // 写入文件数量
        size_t numFiles = files.size();
        out.write(reinterpret_cast<char*>(&numFiles), sizeof(numFiles));
        
        size_t processedFiles = 0;
        std::wstring progressMsg;
        
        for (const auto& file : files) {
            try {
                // 显示处理进度
                progressMsg = L"正在处理: " + file.filename().wstring() + 
                            L"\n进度: " + std::to_wstring(processedFiles + 1) + 
                            L" / " + std::to_wstring(files.size());
                SetWindowTextW(GetActiveWindow(), progressMsg.c_str());

                // 读取源图片
                std::ifstream img(file, std::ios::binary);
                if (!img.is_open()) {
                    std::wstring errMsg = L"无法打开文件：" + file.wstring();
                    MessageBoxW(NULL, errMsg.c_str(), L"警告", MB_OK | MB_ICONWARNING);
                    continue;
                }
                
                // 获取文件名和大小
                std::string filename = file.filename().string();
                size_t nameLength = filename.length();
                
                img.seekg(0, std::ios::end);
                size_t fileSize = img.tellg();
                img.seekg(0, std::ios::beg);
                
                // 写入文件信息
                out.write(reinterpret_cast<char*>(&nameLength), sizeof(nameLength));
                out.write(filename.c_str(), nameLength);
                out.write(reinterpret_cast<char*>(&fileSize), sizeof(fileSize));
                
                // 分块读取和写入文件内容
                const size_t bufferSize = 8192; // 8KB 缓冲区
                std::vector<char> buffer(bufferSize);
                size_t remainingSize = fileSize;
                
                while (remainingSize > 0) {
                    size_t currentRead = std::min(bufferSize, remainingSize);
                    img.read(buffer.data(), currentRead);
                    out.write(buffer.data(), currentRead);
                    remainingSize -= currentRead;
                }
                
                processedFiles++;
                img.close();
            }
            catch (const std::exception& e) {
                std::string errMsg = "处理文件时发生错误: ";
                errMsg += e.what();
                MessageBoxA(NULL, errMsg.c_str(), "警告", MB_OK | MB_ICONWARNING);
            }
        }
        
        out.close();
        
        if (processedFiles > 0) {
            std::wstring successMsg = L"打包完成！\n"
                                    L"成功处理文件数：" + std::to_wstring(processedFiles) + L"\n"
                                    L"输出文件：" + outputPath;
            MessageBoxW(NULL, successMsg.c_str(), L"成功", MB_OK | MB_ICONINFORMATION);
            
            // 打开输出文件所在的文件夹
            std::wstring cmd = L"/select,\"" + outputPath + L"\"";
            ShellExecuteW(NULL, L"open", L"explorer.exe", cmd.c_str(), NULL, SW_SHOW);
        }
    }
    catch (const std::exception& e) {
        std::string errMsg = "打包过程中发生错误: ";
        errMsg += e.what();
        MessageBoxA(NULL, errMsg.c_str(), "错误", MB_OK | MB_ICONERROR);
    }
}
// 主函数
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // 注册窗口类
    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.lpszClassName = L"ImagePackerClass";
    RegisterClassEx(&wc);
    
    // 创建窗口
    HWND hwnd = CreateWindow(
        L"ImagePackerClass", L"图片打包工具",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 640,
        NULL, NULL, hInstance, NULL
    );
    
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);
    
    // 消息循环
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return (int)msg.wParam;
}

// 窗口过程函数
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            // 创建"打开"按钮
            CreateWindow(
                L"BUTTON", L"选择文件夹",
                WS_VISIBLE | WS_CHILD,
                10, 10, 100, 30,
                hwnd, (HMENU)1, NULL, NULL
            );
            
            // 创建"打包"按钮
            CreateWindow(
                L"BUTTON", L"打包",
                WS_VISIBLE | WS_CHILD,
                120, 10, 100, 30,
                hwnd, (HMENU)2, NULL, NULL
            );
            
            // 创建列表框
            hList = CreateWindow(
                L"LISTBOX", NULL,
                WS_VISIBLE | WS_CHILD | WS_BORDER | WS_VSCROLL,
                10, 50, 760, 540,
                hwnd, (HMENU)3, NULL, NULL
            );
            break;
        }
        
        case WM_COMMAND: {
            switch(LOWORD(wParam)) {
                case 1: {  // 选择文件夹按钮
                    BROWSEINFO bi = {0};
                    bi.hwndOwner = hwnd;
                    bi.lpszTitle = L"选择图片所在文件夹";
                    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
                    
                    LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
                    if (pidl) {
                        wchar_t path[MAX_PATH];
                        SHGetPathFromIDList(pidl, path);
                        currentFolder = path;
                        
                        // 清空列表和文件数组
                        SendMessage(hList, LB_RESETCONTENT, 0, 0);
                        imageFiles.clear();
                        
                        // 枚举图片文件
                        for (const auto& entry : fs::directory_iterator(path)) {
                            if (isImageFile(entry.path())) {
                                imageFiles.push_back(entry.path());
                                SendMessageW(hList, LB_ADDSTRING, 0, 
                                    (LPARAM)entry.path().filename().wstring().c_str());
                            }
                        }
                    }
                    CoTaskMemFree(pidl);
                    break;
                }
                
                case 2: {  // 打包按钮
                    if (imageFiles.empty()) {
                        MessageBoxW(hwnd, L"请先选择包含图片的文件夹！", L"提示", MB_OK | MB_ICONINFORMATION);
                        break;
                    }
                    
                    // 对文件进行排序
                    std::sort(imageFiles.begin(), imageFiles.end(),
                        [](const fs::path& a, const fs::path& b) {
                            return getNumberFromFilename(a) < getNumberFromFilename(b);
                        });
                    
                    // 更新列表显示
                    SendMessage(hList, LB_RESETCONTENT, 0, 0);
                    for (const auto& file : imageFiles) {
                        SendMessageW(hList, LB_ADDSTRING, 0, 
                            (LPARAM)file.filename().wstring().c_str());
                    }
                    
                    // 让用户选择保存位置
                    wchar_t szFile[MAX_PATH] = { 0 };
                    OPENFILENAMEW ofn = { 0 };
                    ofn.lStructSize = sizeof(ofn);
                    ofn.hwndOwner = hwnd;
                    ofn.lpstrFilter = L"数据文件(*.dat)\0*.dat\0所有文件(*.*)\0*.*\0";
                    ofn.lpstrFile = szFile;
                    ofn.nMaxFile = MAX_PATH;
                    ofn.lpstrDefExt = L"dat";
                    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
                    
                    // 设置默认文件名
                    std::wstring defaultName = fs::path(currentFolder).filename().wstring() + L".dat";
                    wcscpy(szFile, defaultName.c_str());
                    
                    if (GetSaveFileNameW(&ofn)) {
                        // 用户选择了保存位置
                        std::wstring outputPath = szFile;
                        packImages(imageFiles, outputPath);
                        
                        // 显示排序和打包结果
                        std::wstring msg = L"文件已按数字顺序排序并打包完成！\n\n排序后的顺序：\n";
                        for (const auto& file : imageFiles) {
                            msg += file.filename().wstring() + L"\n";
                        }
                        MessageBoxW(hwnd, msg.c_str(), L"成功", MB_OK | MB_ICONINFORMATION);
                    }
                    break;
                }
            }
            break;  // 添加 WM_COMMAND 的 break
        }
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
            
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}