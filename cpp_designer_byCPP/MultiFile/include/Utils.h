#pragma once

#include <string>
#include <windows.h>

// 工具函数声明
std::wstring UTF8ToWide(const char* utf8Str);
std::string WideToUTF8(const wchar_t* wstr);
std::wstring GetExecutablePath();
std::string ReplaceString(std::string subject, const std::string& search, const std::string& replace);
std::string ToUTF8Comment(const std::string& comment);
std::string ToUTF8String(const std::string& input);
int min1(int a, int b);
int max1(int a, int b);