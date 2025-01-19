#define UNICODE
#define _UNICODE
#include "Utils.h"
#include <shlwapi.h>
#include <vector>

std::wstring UTF8ToWide(const char* utf8Str)
{
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

int min1(int a, int b)
{
    return (a < b) ? a : b;
}

int max1(int a, int b)
{
    return (a > b) ? a : b;
}