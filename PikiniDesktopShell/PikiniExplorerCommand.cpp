#include "pch.h"
#include "PikiniExplorerCommand.h"
#include <shobjidl.h> 
#include <string>
#include "ClearDesktop.h"

constexpr GUID CLSID_PikiniExplorerCommand = __uuidof(PikiniExplorerCommand);
HRESULT S_OK_AllocAndCopy(const std::wstring& str, LPWSTR* ppszOut) {
    if (!ppszOut) return E_POINTER;
    size_t len = str.length();
    *ppszOut = static_cast<LPWSTR>(CoTaskMemAlloc((len + 1) * sizeof(WCHAR)));
    if (!*ppszOut) return E_OUTOFMEMORY;
    wcscpy_s(*ppszOut, len + 1, str.c_str());
    return S_OK;
}

// IExplorerCommand 方法实现
// 获取命令标题
IFACEMETHODIMP PikiniExplorerCommand::GetTitle(_In_opt_ IShellItemArray* psiItemArray, _Outptr_ LPWSTR* ppszName) {
    return S_OK_AllocAndCopy(L"Pikini Explorer Command", ppszName);
}

// 获取命令图标
IFACEMETHODIMP PikiniExplorerCommand::GetIcon(_In_opt_ IShellItemArray* psiItemArray, _Outptr_ LPWSTR* ppszIcon) {
    WCHAR szModule[MAX_PATH];
    GetModuleFileNameW((HMODULE)&__ImageBase, szModule, MAX_PATH); 
    std::wstring dllPath = szModule;
    std::wstring iconPath = dllPath + L",-105";
    return S_OK_AllocAndCopy(iconPath, ppszIcon);
}

// 获取命令工具提示
IFACEMETHODIMP PikiniExplorerCommand::GetToolTip(_In_opt_ IShellItemArray* psiItemArray, _Outptr_ LPWSTR* ppszInfotip) {
    return S_OK_AllocAndCopy(L"这是一个示例的 Pikini Explorer 命令", ppszInfotip);
}

// 获取命令的唯一标识符
IFACEMETHODIMP PikiniExplorerCommand::GetCanonicalName(_Out_ GUID* pguidCommandName) {
    if (!pguidCommandName) return E_POINTER;
    *pguidCommandName = CLSID_PikiniExplorerCommand; // 使用定义的 CLSID
    return S_OK;
}

IFACEMETHODIMP PikiniExplorerCommand::GetState(_In_opt_ IShellItemArray* psiItemArray, _In_ BOOL fOkToBeSlow, _Out_ EXPCMDSTATE* pCmdState) {
    if (!pCmdState) return E_POINTER;
    *pCmdState = ECS_ENABLED; // 设置命令为启用状态
    return S_OK;
}

// 点击执行命令时调用
IFACEMETHODIMP PikiniExplorerCommand::Invoke(_In_opt_ IShellItemArray* psiItemArray, _In_opt_ IBindCtx* pbc) {
    // 在这里实现命令的具体逻辑
    ClearDesktopIcons();
    return S_OK;
}

IFACEMETHODIMP PikiniExplorerCommand::GetFlags(_Out_ EXPCMDFLAGS* pFlags) {
    if (!pFlags) return E_POINTER;
    *pFlags = ECF_DEFAULT;
    return S_OK;
}

IFACEMETHODIMP PikiniExplorerCommand::EnumSubCommands(_Outptr_ IEnumExplorerCommand** ppEnum) {
    if (!ppEnum) return E_POINTER;
    *ppEnum = nullptr; // 目前没有子命令
    return E_NOTIMPL;
}

// IObjectWithSite 方法实现
IFACEMETHODIMP PikiniExplorerCommand::SetSite(_In_ IUnknown* pUnkSite) {
    m_site = pUnkSite; // ComPtr 会自动处理 AddRef 和 Release
    return S_OK;
}

IFACEMETHODIMP PikiniExplorerCommand::GetSite(_In_ REFIID riid, _Outptr_ void** ppvSite) {
    if (!m_site)
    {
        *ppvSite = nullptr;
        return E_FAIL;
    }
    return m_site.CopyTo(riid, ppvSite); // ComPtr::CopyTo 完成 QueryInterface 的工作
}