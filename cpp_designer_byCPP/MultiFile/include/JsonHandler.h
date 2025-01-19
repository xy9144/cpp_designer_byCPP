#pragma once

#include "MainWindow.h"
#include "cJSON/cJSON.h"

// JSON 相关函数声明
void LoadComponentsFromJson(HWND hwnd, const wchar_t* filename);
void SaveComponentsToJson(HWND hwnd, const wchar_t* filename);