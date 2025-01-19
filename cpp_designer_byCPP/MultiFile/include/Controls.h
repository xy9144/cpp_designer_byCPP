#pragma once

#include "MainWindow.h"

// 控件相关函数声明
const wchar_t* GetComponentTypeByImageIndex(int imageIndex);
void UpdateListView2Info(HWND hControl);
void ClearDesignWindow(HWND hwnd);
std::string CreateControl(
    const std::string& type,
    const std::string& name,
    const std::string& title,
    const std::string& style,
    const std::string& exStyle,
    int left,
    int top,
    int width,
    int height,
    const std::string& parentHandle = "NULL",
    const std::string& childHandle = "NULL",
    const std::string& instanceHandle = "hInstance",
    const std::string& additionalData = "NULL"
);