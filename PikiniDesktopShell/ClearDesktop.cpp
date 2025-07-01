#include "pch.h"
#include "ClearDesktop.h"
#include <filesystem>
#include <iostream>

using namespace std;
namespace fs = std::filesystem;



int ClearDesktopIcons() {
    HWND hListView = NULL;
    HWND hDesktop = GetShellWindow();
    if (hDesktop) {
        HWND hDefView =
            FindWindowExW(hDesktop, NULL, L"SHELLDLL_DefView", NULL);
        if (hDefView) {
            hListView =
                FindWindowExW(hDefView, NULL, L"SysListView32", L"FolderView");
        }
    }

    if (!hListView) {
        std::wcerr << L"Failed to get desktop ListView handle." << std::endl;
        return 1;
    }

    _setmode(_fileno(stdout), _O_U16TEXT);
    std::list<std::wstring> paths =
        getFilesInDirectory(L"C:\\Users\\123\\Desktop\\");

    FileClassification classification = ClassifyFiles(paths);

    auto IconList = GetIconPositionWithName();
    if (IconList.empty()) {
        std::wcerr << L"No icons found on desktop." << std::endl;
        return 1;
    }
    std::vector<std::vector<POINT>> PointList =
        GetListviewItemPosition(IconList.front().pint, hListView);
    if (PointList.empty() || PointList[0].empty()) {
        std::wcerr << L"Failed to get desktop grid." << std::endl;
        return 1;
    }

    arrangeDesktopIcons(hListView, PointList, classification, IconList);
    return 0;
}

inline bool operator==(const POINT& lhs, const POINT& rhs) {
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

std::list<std::wstring> getFilesInDirectory(const std::wstring& directoryPath) {
    std::list<std::wstring> result;

    for (const auto& entry : fs::directory_iterator(directoryPath)) {
        result.push_back(entry.path().wstring());
    }

    return result;
}

FileClassification ClassifyFiles(const std::list<std::wstring>& paths) {
    FileClassification result;

    for (const auto& path : paths) {
        if (!fs::exists(path)) {
            std::wprintf(L"Path does not exist:  %ls\n", path.c_str());
            continue;
        }
        if (fs::is_regular_file(path) && !HasHiddenFile(path)) {
            result.files.push_back(path);
        }
        else if (fs::is_directory(path) && HasIcon(path)) {
            result.Icondirectories.push_back(path);
        }
        else if (fs::is_directory(path)) {
            result.directories.push_back(path);
        }
        else if (fs::is_symlink(path)) {
            result.shortcuts.push_back(path);
        }
        else {
            std::wprintf(L"Unknown file type:  %ls\n", path.c_str());
        }
    }
    return result;
}
bool HasIcon(const std::wstring& path) {
    try {
        fs::path dirPath(path);
        if (!fs::exists(dirPath) || !fs::is_directory(dirPath)) {
            return false;
        }

        for (const auto& entry : fs::directory_iterator(dirPath)) {
            if (entry.is_regular_file() &&
                entry.path().extension() == L".ini") {
                return true;
            }
        }
        return false;
    }
    catch (const fs::filesystem_error& e) {
        std::wcerr << L"Error accessing directory: " << path.c_str()
            << L", error: " << e.what() << std::endl;
        return false;
    }
}
bool HasHiddenFile(const std::wstring& path) {
    fs::path p = fs::path(path).filename();
    std::wstring filename = p.wstring();

    if (p.has_extension()) {
        const std::wstring extension = p.extension().wstring();
        if (extension == L".ini" || extension == L".tmp") {
            return true;
        }
    }

    return !filename.empty() && (filename.front() == L'.' || filename.front() == L'~');
}

// 获取桌面排列视图
std::vector<std::vector<POINT>> GetListviewItemPosition(POINT itmePoint, HWND hListView)
{
    std::vector<std::vector<POINT>> IconGrid;
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    RECT rc = {};
    if (!GetClientRect(hListView, &rc)) return IconGrid;

    POINT spacing = {};
    DWORD itemSpacing = ListView_GetItemSpacing(hListView, FALSE);
    spacing.x = LOWORD(itemSpacing);
    spacing.y = HIWORD(itemSpacing);

    RECT rcWorkArea;
    if (!SystemParametersInfo(SPI_GETWORKAREA, 0, &rcWorkArea, 0)) {
        rcWorkArea = { 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) };
    }

    MapWindowPoints(hListView, NULL, (POINT*)&rc, 2);
    RECT rcEffective;
    if (!IntersectRect(&rcEffective, &rc, &rcWorkArea)) return IconGrid;

    int effectiveWidth = rcEffective.right - rcEffective.left;
    int effectiveHeight = rcEffective.bottom - rcEffective.top;
    int cols = effectiveWidth / spacing.x;
    int rows = effectiveHeight / spacing.y;

    if (cols <= 0 || rows <= 0) return IconGrid;

    IconGrid.resize(rows, std::vector<POINT>(cols));
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            IconGrid[r][c] = {
                c * spacing.x + itmePoint.x,
                r * spacing.y + itmePoint.y
            };
        }
    }

    return IconGrid;
}

std::list<IconitemWithIndex> GetIconPositionWithName() {
    HWND hListView = ::FindWindowW(L"progman", NULL);
    hListView = ::FindWindowExW(hListView, 0, L"shelldll_defview", NULL);
    hListView = ::FindWindowExW(hListView, 0, L"syslistview32", NULL);

    std::list<IconitemWithIndex> IconList;
    DWORD dwProcessId;

    GetWindowThreadProcessId(hListView, &dwProcessId);
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwProcessId);

    tagLVITEM64W* _lv, lvi;
    LPVOID lpvPt = VirtualAllocEx(hProcess, NULL, sizeof(POINT), MEM_COMMIT,
        PAGE_READWRITE);

    _lv = (tagLVITEM64W*)VirtualAllocEx(hProcess, NULL, sizeof(tagLVITEM64W),
        MEM_COMMIT, PAGE_READWRITE);

    wchar_t item[512] = { 0 };
    char* _item;
    POINT pt;

    int m_iconCount = ListView_GetItemCount(hListView);
    _item =
        (char*)VirtualAllocEx(hProcess, NULL, 512, MEM_COMMIT, PAGE_READWRITE);

    ZeroMemory(&lvi, sizeof(tagLVITEM64W));

    lvi.mask = LVIF_TEXT;
    lvi.cchTextMax = 512;
    // 设置index为0的icon到最顶部,方便获取正确的偏移量
    SetIconPosition(hListView, 0, POINT{ 0, 0 });
    for (int i = 0; i < m_iconCount; i++) {
        ListView_GetItemPosition(hListView, i, lpvPt);
        ReadProcessMemory(hProcess, lpvPt, &pt, sizeof(POINT), NULL);
        lvi.iItem = i;
        lvi.iSubItem = 0;
        lvi.pszText = (INT64)_item;
        if (!WriteProcessMemory(hProcess, _lv, &lvi, sizeof(LVITEM64W), NULL)) {
            std::cerr << "WriteProcessMemory failed: " << GetLastError()
                << std::endl;
        }
        SendMessage(hListView, LVM_GETITEMTEXT, (WPARAM)i, (LPARAM)_lv);
        ReadProcessMemory(hProcess, _item, item, 512 * sizeof(wchar_t), NULL);
        IconList.push_back(IconitemWithIndex{ i, pt, item });
    }

    VirtualFreeEx(hProcess, lpvPt, 0, MEM_RELEASE);
    VirtualFreeEx(hProcess, _lv, 0, MEM_RELEASE);
    VirtualFreeEx(hProcess, _item, 0, MEM_RELEASE);
    CloseHandle(hProcess);
    return IconList;
}

// 设置图标位置
void SetIconPosition(HWND hListView, int iIconIndex, POINT pt) {
    SendMessage(hListView, LVM_SETITEMPOSITION, (WPARAM)iIconIndex,
        MAKELPARAM(pt.x, pt.y));
}

void arrangeDesktopIcons(HWND hListView,
    const std::vector<std::vector<POINT>>& PointList,
    const FileClassification& classification, const list<IconitemWithIndex>& IconList) {
    // 获取行数和列数
    int rows = PointList.size();
    int cols = PointList[0].size();

    // 初始化图标矩阵，所有元素设为-1
    std::vector<std::vector<int>> iconMatrix(rows, std::vector<int>(cols, -1));

    // 分别获取目录、文件和特殊目录的列表
    std::list<std::wstring> aClass = classification.directories;
    std::list<std::wstring> bClass = classification.files;
    std::list<std::wstring> cClass = classification.Icondirectories;

    // 初始化目录和文件的迭代器
    auto itA = aClass.begin();
    auto itB = bClass.begin();
    // 初始化已放置图标的总数
    int totalPlaced = 0;
    // 获取目录和文件的数量
    int aCount = aClass.size();
    int bCount = bClass.size();

    // 遍历矩阵，为目录和文件放置图标
    for (int col = 0; col < cols && totalPlaced < aCount + bCount; ++col) {
        for (int row = 0; row < rows && totalPlaced < aCount + bCount; ++row) {
            std::wstring filePath;
            // 优先放置目录图标
            if (totalPlaced < aCount && itA != aClass.end()) {
                filePath = *itA;
                ++itA;
            }
            else if (itB != bClass.end()) {
                // 如果目录图标放置完毕，则放置文件图标
                filePath = *itB;
                ++itB;
            }
            totalPlaced++;

            // 通过文件名获取图标索引
            std::wstring fileName = fs::path(filePath).filename().wstring();
            int index = -1;
            for (const auto& item : IconList) {
                if (item.name == fileName) {
                    index = item.index;
                    break;
                }
            }
            // 如果找到图标，则记录其索引到矩阵中
            if (index != -1) {
                iconMatrix[row][col] = index;
            }
        }
    }

    // 初始化特殊目录的迭代器和数量
    auto itC = cClass.begin();
    int cCount = cClass.size();
    // 遍历矩阵，为特殊目录放置图标，从右向左放置
    for (int col = cols - 1; col >= 0 && cCount > 0; --col) {
        for (int row = 0; row < rows && cCount > 0; ++row) {
            // 如果当前位置已有图标，则跳过
            if (iconMatrix[row][col] != -1) {
                continue;
            }
            // 放置特殊目录图标
            if (itC != cClass.end()) {
                std::wstring filePath = *itC;
                std::wstring fileName = fs::path(filePath).filename().wstring();
                int index = -1;
                for (const auto& item : IconList) {
                    if (item.name == fileName) {
                        index = item.index;
                        break;
                    }
                }
                // 如果找到图标，则记录其索引到矩阵中，并更新计数
                if (index != -1) {
                    iconMatrix[row][col] = index;
                    --cCount;
                    ++itC;
                }
            }
        }
    }
    // 禁止列表视图的绘制
    SendMessage(hListView, WM_SETREDRAW, FALSE, 0);
    // 根据图标矩阵更新图标的实际位置
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            int index = iconMatrix[row][col];
            // 如果当前位置有图标(索引不为-1且不为0)，则设置其位置
            if (index != -1 && index != 0) {
                SetIconPosition(hListView, index, PointList[row][col]);
            }
        }
    }
    // 重新启用ListView控件的重绘功能
    SendMessage(hListView, WM_SETREDRAW, TRUE, 0);
}
