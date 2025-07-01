#pragma once

#include <windows.h>
#include <shobjidl.h> 
#include <wrl/client.h>
#include <wrl/implements.h>
#include <wrl/module.h>


using namespace Microsoft::WRL;

class __declspec(uuid("{EA9FBF19-BD53-4167-947E-32682DB80C83}"))
 PikiniExplorerCommand :public RuntimeClass<RuntimeClassFlags<ClassicCom>, IExplorerCommand, IObjectWithSite>
{
public:
    // *** IExplorerCommand 方法 ***
    IFACEMETHODIMP GetTitle(_In_opt_ IShellItemArray* psiItemArray, _Outptr_ LPWSTR* ppszName);
    IFACEMETHODIMP GetIcon(_In_opt_ IShellItemArray* psiItemArray, _Outptr_ LPWSTR* ppszIcon);
    IFACEMETHODIMP GetToolTip(_In_opt_ IShellItemArray* psiItemArray, _Outptr_ LPWSTR* ppszInfotip);
    IFACEMETHODIMP GetCanonicalName(_Out_ GUID* pguidCommandName);
    IFACEMETHODIMP GetState(_In_opt_ IShellItemArray* psiItemArray, _In_ BOOL fOkToBeSlow, _Out_ EXPCMDSTATE* pCmdState);
    IFACEMETHODIMP Invoke(_In_opt_ IShellItemArray* psiItemArray, _In_opt_ IBindCtx* pbc);
    IFACEMETHODIMP GetFlags(_Out_ EXPCMDFLAGS* pFlags);
    IFACEMETHODIMP EnumSubCommands(_Outptr_ IEnumExplorerCommand** ppEnum);

    // *** IObjectWithSite 方法 ***
    IFACEMETHODIMP SetSite(_In_ IUnknown* pUnkSite);
    IFACEMETHODIMP GetSite(_In_ REFIID riid, _Outptr_ void** ppvSite);

private:
    ComPtr<IUnknown> m_site;
};
CoCreatableClass(PikiniExplorerCommand);
