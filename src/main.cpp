#include "main.h"
#include <filesystem>
#include <iostream>

using namespace std;
namespace fs = std::filesystem;

//

void arrangeDesktopIcons(HWND hListView,
                         const std::vector<std::vector<POINT>>& PointList,
                         const FileClassification& classification) {
	int rows = PointList.size();
	int cols = PointList[0].size();

	// Create a matrix to store icon indices (or -1 if empty)
	std::vector<std::vector<int>> iconMatrix(rows, std::vector<int>(cols, -1));

	// Get icon lists from classification
	std::list<std::wstring> aClass = classification.files; // 'A' = Files
	std::list<std::wstring> bClass =
	    classification.directories; // 'B' = Directories
	std::list<std::wstring> cClass =
	    classification.Icondirectories; // 'C' = Icon directories

	// Step 1: Fill 'A' and 'B' classes from left to right, column-by-column
	auto itA = aClass.begin();
	auto itB = bClass.begin();
	int totalPlaced = 0;
	int aCount = aClass.size();
	int bCount = bClass.size();

	for (int col = 0; col < cols && totalPlaced < aCount + bCount; ++col) {
		for (int row = 0; row < rows && totalPlaced < aCount + bCount; ++row) {
			std::wstring filePath;
			if (totalPlaced < aCount && itA != aClass.end()) {
				filePath = *itA;
				++itA;
			} else if (itB != bClass.end()) {
				filePath = *itB;
				++itB;
			}
			totalPlaced++;

			// Find the icon index by name (assuming name matches filePath's
			// filename)
			std::wstring fileName = fs::path(filePath).filename().wstring();
			int index = -1;
			auto iconList = GetIconPositionWithName();
			for (const auto& item : iconList) {
				if (item.name == fileName) {
					index = item.index;
					break;
				}
			}
			if (index != -1) {
				iconMatrix[row][col] = index;
			}
		}
	}

	// Step 2: Fill 'C' class from right to left, column-by-column
	auto itC = cClass.begin();
	int cCount = cClass.size();
	for (int col = cols - 1; col >= 0 && cCount > 0; --col) {
		for (int row = 0; row < rows && cCount > 0; ++row) {
			if (iconMatrix[row][col] != -1) {
				continue; // Skip occupied slots
			}
			if (itC != cClass.end()) {
				std::wstring filePath = *itC;
				std::wstring fileName = fs::path(filePath).filename().wstring();
				int index = -1;
				auto iconList = GetIconPositionWithName();
				for (const auto& item : iconList) {
					if (item.name == fileName) {
						index = item.index;
						break;
					}
				}
				if (index != -1) {
					iconMatrix[row][col] = index;
					--cCount;
					++itC;
				}
			}
		}
	}

	// Step 3: Apply positions to the desktop
	for (int row = 0; row < rows; ++row) {
		for (int col = 0; col < cols; ++col) {
			int index = iconMatrix[row][col];
			if (index != -1) {
				SetIconPosition(hListView, index, PointList[row][col]);
			}
		}
	}
}

int main() {
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

	// Classify files
	FileClassification classification = classifyFiles(paths);

	// Get desktop grid
	auto iconList = GetIconPositionWithName();
	if (iconList.empty()) {
		std::wcerr << L"No icons found on desktop." << std::endl;
		return 1;
	}
	std::vector<std::vector<POINT>> PointList =
	    GetListviewItemPosition(iconList.front().pint, hListView);
	if (PointList.empty() || PointList[0].empty()) {
		std::wcerr << L"Failed to get desktop grid." << std::endl;
		return 1;
	}

	// Arrange icons according to the fillMatrix rule
	arrangeDesktopIcons(hListView, PointList, classification);

	// Optional: Output the final layout for debugging
	std::wcout << L"Desktop layout updated (" << PointList.size() << L" rows, "
	           << PointList[0].size() << L" columns):\n";
	auto updatedIcons = GetIconPositionWithName();
	for (const auto& item : updatedIcons) {
		std::wcout << L"[name: " << item.name << L", Point: (" << item.pint.x
		           << L", " << item.pint.y << L")]\n";
	}

	return 0;
}

// Ensure operator== for POINT is defined
inline bool operator==(const POINT& lhs, const POINT& rhs) {
	return lhs.x == rhs.x && lhs.y == rhs.y;
}
//

std::list<std::wstring> getFilesInDirectory(const std::wstring& directoryPath) {
	std::list<std::wstring> result;

	for (const auto& entry : fs::directory_iterator(directoryPath)) {
		result.push_back(entry.path().wstring());
	}

	return result;
}
// int main() {
// 	HWND hListView = NULL;
// 	HWND hDesktop = GetShellWindow();
// 	if (hDesktop) {
// 		HWND hDefView =
// 		    FindWindowExW(hDesktop, NULL, L"SHELLDLL_DefView", NULL);
// 		if (hDefView) {
// 			hListView =
// 			    FindWindowExW(hDefView, NULL, L"SysListView32", L"FolderView");
// 		}
// 	}

// 	_setmode(_fileno(stdout), _O_U16TEXT);
// 	std::list<std::wstring> paths =
// 	    getFilesInDirectory(L"C:\\Users\\123\\Desktop\\");

// 	// 存储合并后的图标信息列表
// 	std::list<IconitemWithIndex> DesktopPointList;

// 	// 获取桌面图标的位置和名称映射关系
// 	auto IconList = GetIconPositionWithName();
// 	if (!IconList.empty()) {
// 		// 获取列表视图中的图标位置信息（现在是二维数组）
// 		std::vector<std::vector<POINT>> PointList =
// 		    GetListviewItemPosition(IconList.front().pint, hListView);
// 		if (PointList.empty() || PointList[0].empty() || IconList.empty()) {
// 			return 1;
// 		}

// 		// 输出图标列表大小信息
// 		wcout << L"iconList.size() " << PointList.size() << L" rows, "
// 		      << PointList[0].size() << L" columns\n";

// 		// 将位置信息与图标名称进行匹配合并
// 		int ExceedIndex = IconList.size();
// 		for (size_t r = 0; r < PointList.size(); ++r) {
// 			for (size_t c = 0; c < PointList[r].size(); ++c) {
// 				const POINT& PointListItem = PointList[r][c];
// 				bool found = false;
// 				for (const auto& IconListItem : IconList) {
// 					if (PointListItem == IconListItem.pint) {
// 						DesktopPointList.push_back(
// 						    IconitemWithIndex{IconListItem.index, PointListItem,
// 						                      IconListItem.name});
// 						found = true;
// 						break;
// 					}
// 				}
// 				if (!found) {
// 					ExceedIndex += 1;
// 					DesktopPointList.push_back(IconitemWithIndex{
// 					    ExceedIndex, PointListItem, L"Unknow"});
// 				}
// 			}
// 		}
// 	}

// 	// 输出最终合并后的图标信息
// 	for (const auto& DesktopListItem : DesktopPointList) {
// 		SetIconPosition(hListView, DesktopListItem.index, DesktopListItem.pint);
// 		std::wcout << L"[name: " << DesktopListItem.name << L",Point: ("
// 		           << DesktopListItem.pint.x << L" " << DesktopListItem.pint.y
// 		           << L")]\n";
// 	}

// 	return 0;
// }

FileClassification classifyFiles(const std::list<std::wstring>& paths) {
	FileClassification result;

	for (const auto& path : paths) {
		if (!fs::exists(path)) {
			std::wprintf(L"Path does not exist:  %ls\n", path.c_str());
			continue;
		}
		if (fs::is_regular_file(path)) {
			result.files.push_back(path);
		} else if (fs::is_directory(path) && is_have_icon(path)) {
			result.Icondirectories.push_back(path);
		} else if (fs::is_directory(path)) {
			result.directories.push_back(path);
		} else if (fs::is_symlink(path)) {
			result.shortcuts.push_back(path);
		} else {
			std::wprintf(L"Unknown file type:  %ls\n", path.c_str());
		}
	}
	std::wprintf(L"Files:\n");
	for (const auto& file : result.files) {
		std::wprintf(L"%s\n", file.c_str());
	}

	std::wprintf(L"Directories:\n");
	for (const auto& dir : result.directories) {
		std::wprintf(L"%s\n", dir.c_str());
	}
	std::wprintf(L"IconDirectories:\n");
	for (const auto& icondir : result.Icondirectories) {
		std::wprintf(L"%s\n", icondir.c_str());
	}

	std::wprintf(L"Shortcuts:\n");
	for (const auto& shortcut : result.shortcuts) {
		std::wprintf(L"%s\n", shortcut.c_str());
	}
	return result;
}

bool is_have_icon(const std::wstring& path) {
	try {
		fs::path dirPath(path);
		if (!fs::exists(dirPath) || !fs::is_directory(dirPath)) {
			return false;
		}

		// 遍历目录，查找是否有 .ini 文件
		for (const auto& entry : fs::directory_iterator(dirPath)) {
			if (entry.is_regular_file() &&
			    entry.path().extension() == L".ini") {
				return true;
			}
		}
		return false;
	} catch (const fs::filesystem_error& e) {
		std::wcerr << L"Error accessing directory: " << path.c_str()
		           << L", error: " << e.what() << std::endl;
		return false;
	}
}

// 获取桌面排列视图
std::vector<std::vector<POINT>> GetListviewItemPosition(POINT itmePoint,
                                                        HWND hListView) {
	std::vector<std::vector<POINT>> IconGrid;
	// 1. 设置DPI感知 (在任何UI或COM操作前完成)
	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

	// 2. 初始化COM库
	HRESULT hr = CoInitialize(NULL);
	if (FAILED(hr)) {
		std::cerr << "Failed to initialize COM library. Error code: " << hr
		          << std::endl;
		return IconGrid;
	}

	// 使用 CComPtr 智能指针可以自动管理 COM 对象的生命周期 (自动调用 Release)
	CComPtr<IShellView> pShellView;
	CComPtr<IFolderView> pFolderView;

	// 我们将COM操作放在一个块中,以便CComPtr在CoUninitialize之前析构
	{
		CComPtr<IShellFolder> pDesktopFolder;
		// 3. 获取代表桌面的 IShellFolder 接口
		hr = SHGetDesktopFolder(&pDesktopFolder);
		if (FAILED(hr)) {
			std::cerr << "Failed to get desktop folder. Error code: " << hr
			          << std::endl;
			CoUninitialize();
			return IconGrid;
		}

		// 4. 获取桌面的 IShellView 接口
		HWND hDesktop = GetShellWindow();
		hr = pDesktopFolder->CreateViewObject(hDesktop,
		                                      IID_PPV_ARGS(&pShellView));
		if (FAILED(hr)) {
			std::cerr << "Failed to create view object. Error code: " << hr
			          << std::endl;
			CoUninitialize();
			return IconGrid;
		}

		// 5. 从 IShellView 查询更现代、更易用的 IFolderView 接口
		// IFolderView 提供了更简单的API来获取图标位置和数量
		hr = pShellView->QueryInterface(IID_PPV_ARGS(&pFolderView));
		if (FAILED(hr)) {
			std::cerr << "Failed to query IFolderView interface. Error code: "
			          << hr << std::endl;
			CoUninitialize();
			return IconGrid;
		}
	} // pDesktopFolder 在这里超出作用域，CComPtr 会自动调用
	  // pDesktopFolder->Release()

	// --- 至此，我们已经安全地获取了桌面视图对象，不会导致闪烁 ---

	// 6. 获取桌面 ListView 的客户端矩形
	RECT rc = {0};

	CComPtr<IOleWindow> pOleWindow;
	hr = pFolderView->QueryInterface(IID_PPV_ARGS(&pOleWindow));
	// 检查是否最终成功获取了句柄
	if (hListView) {
		GetClientRect(hListView, &rc);
	} else {
		std::cerr << "Failed to get ListView handle using both methods."
		          << std::endl;
		CoUninitialize();
		return IconGrid;
	}

	// pOleWindow 超出作用域，自动释放
	// 7. 获取图标间距 (网格大小)
	POINT spacing = {0, 0};
	DWORD itemSpacing = ListView_GetItemSpacing(hListView, FALSE);
	// 表示获取小图标间距，通常桌面是大图标，但此API返回的是完整的网格大小
	if (itemSpacing != 0) {
		spacing.x = LOWORD(itemSpacing); // 网格宽度
		spacing.y = HIWORD(itemSpacing); // 网格高度
		hr = S_OK; // 手动将 hr 设为成功状态，以便后续代码继续执行
	}

	if (FAILED(hr)) {
		std::cerr << "Failed to get item spacing using both methods."
		          << std::endl;
		CoUninitialize();
		return IconGrid;
	}

	int colWidth = spacing.x;
	int rowHeight = spacing.y;

	// 8. 计算网格的行数和列数 (已修正，考虑工作区)

	// 8a. 获取主显示器的工作区矩形 (排除了任务栏等)
	RECT rcWorkArea;
	if (!SystemParametersInfo(SPI_GETWORKAREA, 0, &rcWorkArea, 0)) {
		// 如果失败，就退回到使用整个屏幕矩形，虽然不完美但能工作
		// 这种情况很少发生
		rcWorkArea.left = 0;
		rcWorkArea.top = 0;
		rcWorkArea.right = GetSystemMetrics(SM_CXSCREEN);
		rcWorkArea.bottom = GetSystemMetrics(SM_CYSCREEN);
	}

	// 8b. 计算 ListView 客户区和工作区的交集，得到真正的有效区域
	RECT rcEffective;
	// ClientRect 返回的 rc 是相对于 ListView 窗口左上角的 (即 rc.left=0,
	// rc.top=0) 我们需要先将它转换为屏幕坐标
	MapWindowPoints(hListView, NULL, (POINT*)&rc, 2);

	if (!IntersectRect(&rcEffective, &rc, &rcWorkArea)) {
		// 如果没有交集 (几乎不可能)，则说明桌面完全不可见
		std::cerr << "Desktop view area has no intersection with the work area."
		          << std::endl;
		CoUninitialize();
		return IconGrid;
	}

	// 8c. 使用有效区域的尺寸来计算行列数
	int effectiveWidth = rcEffective.right - rcEffective.left;
	int effectiveHeight = rcEffective.bottom - rcEffective.top;

	int cols = effectiveWidth / colWidth;
	int rows = effectiveHeight / rowHeight;

	if (cols <= 0 || rows <= 0) {
		std::cerr << "Invalid grid dimensions calculated." << std::endl;
		CoUninitialize();
		return IconGrid;
	}

	// 9. 创建二维数组
	IconGrid.resize(rows, std::vector<POINT>(cols));

	// 10. 填充二维数组
	for (int r = 0; r < rows; ++r) {
		for (int c = 0; c < cols; ++c) {
			POINT freePt = {c * colWidth + itmePoint.x,
			                r * rowHeight + itmePoint.y};
			IconGrid[r][c] = freePt;
		}
	}

	// 11. 清理COM库
	CoUninitialize();

	return IconGrid;
}
// 获取图标位置和名字
std::list<IconitemWithIndex> GetIconPositionWithName() {
	HWND hListView = ::FindWindowW(L"progman", NULL);
	hListView = ::FindWindowExW(hListView, 0, L"shelldll_defview", NULL);
	hListView = ::FindWindowExW(hListView, 0, L"syslistview32", NULL);

	std::list<IconitemWithIndex> IconList;
	DWORD dwProcessId;

	GetWindowThreadProcessId(hListView, &dwProcessId);
	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwProcessId);

	tagLVITEM64W *_lv, lvi;
	LPVOID lpvPt = VirtualAllocEx(hProcess, NULL, sizeof(POINT), MEM_COMMIT,
	                              PAGE_READWRITE);

	_lv = (tagLVITEM64W*)VirtualAllocEx(hProcess, NULL, sizeof(tagLVITEM64W),
	                                    MEM_COMMIT, PAGE_READWRITE);

	wchar_t item[512] = {0};
	char* _item;
	POINT pt;

	int m_iconCount = ListView_GetItemCount(hListView);
	_item =
	    (char*)VirtualAllocEx(hProcess, NULL, 512, MEM_COMMIT, PAGE_READWRITE);

	ZeroMemory(&lvi, sizeof(LVITEM));
	lvi.mask = LVIF_TEXT;
	lvi.cchTextMax = 512;
	// 设置index为0的icon到最顶部,方便获取正确的偏移量
	SetIconPosition(hListView, 0, POINT{0, 0});
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
		IconList.push_back(IconitemWithIndex{i, pt, item});
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

// 监听文件夹
void WatchDirectoryChanges(LPCWSTR lpDirectoryPath) {
	// 打开目录句柄
	HANDLE hDir =
	    CreateFileW(lpDirectoryPath, FILE_LIST_DIRECTORY,
	                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
	                NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
	if (hDir == INVALID_HANDLE_VALUE) {
		std::wcout << L"无法打开目录。错误代码: " << GetLastError()
		           << std::endl;
		return;
	}
	const DWORD dwBufferSize = 65536;
	BYTE buffer[dwBufferSize];
	while (true) {
		DWORD dwBytesReturned = 0;
		// 读取目录变化
		if (ReadDirectoryChangesW(
		        hDir, buffer, dwBufferSize, FALSE,
		        FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME |
		            FILE_NOTIFY_CHANGE_ATTRIBUTES | FILE_NOTIFY_CHANGE_SIZE |
		            FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_SECURITY,
		        &dwBytesReturned, NULL, NULL)) {
			if (dwBytesReturned == 0) {
				continue;
			}
			PFILE_NOTIFY_INFORMATION pNotifyInfo =
			    reinterpret_cast<PFILE_NOTIFY_INFORMATION>(buffer);
			do {
				// 获取文件名（宽字符）
				std::wstring fileName(pNotifyInfo->FileName,
				                      pNotifyInfo->FileNameLength /
				                          sizeof(WCHAR));
				// 查找事件类型的描述
				const wchar_t* typeDescription = L"未知类型";
				for (const auto& type : changeTypes) {
					if (type.type == pNotifyInfo->Action) {
						typeDescription = type.description;
						break;
					}
				}
				// 打印事件类型和文件名
				std::wcout << L"[变更类型: " << std::hex << pNotifyInfo->Action
				           << L"] " << typeDescription << L" 文件: " << fileName
				           << std::endl;
				// 移动到下一个记录
				if (pNotifyInfo->NextEntryOffset == 0)
					break;
				pNotifyInfo = reinterpret_cast<PFILE_NOTIFY_INFORMATION>(
				    reinterpret_cast<BYTE*>(pNotifyInfo) +
				    pNotifyInfo->NextEntryOffset);
			} while (true);
		} else {
			std::wcerr << L"ReadDirectoryChangesW 失败，错误代码: "
			           << GetLastError() << std::endl;
			break;
		}
	}
	CloseHandle(hDir);
}