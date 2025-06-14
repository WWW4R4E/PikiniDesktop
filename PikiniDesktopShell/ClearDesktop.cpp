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

	// Classify files
	FileClassification classification = ClassifyFiles(paths);

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

	arrangeDesktopIcons(hListView, PointList, classification);
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
	RECT rc = { 0 };

	CComPtr<IOleWindow> pOleWindow;
	hr = pFolderView->QueryInterface(IID_PPV_ARGS(&pOleWindow));
	// 检查是否最终成功获取了句柄
	if (hListView) {
		GetClientRect(hListView, &rc);
	}
	else {
		std::cerr << "Failed to get ListView handle using both methods."
			<< std::endl;
		CoUninitialize();
		return IconGrid;
	}

	// pOleWindow 超出作用域，自动释放
	// 7. 获取图标间距 (网格大小)
	POINT spacing = { 0, 0 };
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
			POINT freePt = { c * colWidth + itmePoint.x,
							r * rowHeight + itmePoint.y };
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
	const FileClassification& classification) {
	// 获取行数和列数
	int rows = PointList.size();
	int cols = PointList[0].size();

	// 初始化图标矩阵，所有元素设为-1
	std::vector<std::vector<int>> iconMatrix(rows, std::vector<int>(cols, -1));

	// 分别获取目录、文件和特殊目录的列表
	std::list<std::wstring> aClass = classification.directories;
	std::list<std::wstring> bClass = classification.files;
	std::list<std::wstring> cClass = classification.Icondirectories;

	// 获取带文件名的图标列表
	auto iconList = GetIconPositionWithName();

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
			for (const auto& item : iconList) {
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
				for (const auto& item : iconList) {
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
