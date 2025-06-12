#include <ShlObj.h>
#include <Windows.h>
#include <atlbase.h>
#include <commctrl.h>
#include <fcntl.h>
#include <io.h>
#include <list>
#include <string>
#include <vector>

struct FileClassification {
	std::list<std::wstring> files;           // 普通文件
	std::list<std::wstring> directories;     // 文件夹
	std::list<std::wstring> Icondirectories; // 带图标的文件夹
	std::list<std::wstring> shortcuts;       // 快捷方式（符号链接）
};
typedef struct tagLVITEM64W {
	UINT mask;
	int iItem;
	int iSubItem;
	UINT state;
	UINT stateMask;
	INT64 pszText;
	int cchTextMax;
	int iImage;
	LPARAM lParam;
#if (_WIN32_IE >= 0x0300)
	int iIndent;
#endif
#if (_WIN32_WINNT >= 0x501)
	int iGroupId;
	UINT cColumns;
	PUINT puColumns;
#endif
} LVITEM64W, *LPLVITEM64W;

// struct Iconitem {
// 	POINT pint;
// 	std::wstring name;
// };
struct IconitemWithIndex {
	int index;         // 图标的索引
	POINT pint;        // 图标的位置
	std::wstring name; // 图标的名称
};

struct FileChangeType {
	DWORD type;
	const wchar_t* description;
};

// 定义事件类型数组
const FileChangeType changeTypes[] = {
    {FILE_ACTION_ADDED, L"文件被添加"},
    {FILE_ACTION_REMOVED, L"文件被删除"},
    {FILE_ACTION_MODIFIED, L"文件被修改"},
    {FILE_ACTION_RENAMED_OLD_NAME, L"文件被重命名（旧名称）"},
    {FILE_ACTION_RENAMED_NEW_NAME, L"文件被重命名（新名称）"}};
inline bool operator==(const POINT& lhs, const POINT& rhs);
bool is_have_icon(const std::wstring& path);
FileClassification classifyFiles(const std::list<std::wstring>& paths);
std::vector<std::vector<POINT>> GetListviewItemPosition(POINT itmePoint,
                                                        HWND hListView);
std::list<IconitemWithIndex> GetIconPositionWithName();

void SetIconPosition(HWND hListView, int iIconIndex, POINT pt);
std::list<std::wstring> getFilesInDirectory(const std::wstring& directoryPath);
void WatchDirectoryChanges(LPCWSTR path);
