#include <WinSock2.h>
#include <Windows.h>
#include <windowsx.h>
#include <objbase.h>
#include <wlanapi.h>

#include "resource.h"

#pragma comment(lib, "wlanapi")
#pragma comment(lib, "wlanui")

INT_PTR CALLBACK MainDialogProc(_In_ HWND hwndDlg, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam);

_Use_decl_annotations_ int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr)) {
        __debugbreak();        
    }

    INT_PTR result = DialogBox(hInstance, MAKEINTRESOURCE(IDD_MAIN_DIALOG), nullptr, MainDialogProc);

    if (result <= 0) {
        __debugbreak();
    }

    CoUninitialize();

    return 0;
}

_Use_decl_annotations_ INT_PTR CALLBACK MainDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static HANDLE hClientHandle = nullptr;
    static PWLAN_INTERFACE_INFO_LIST interfaceList = nullptr;

    switch (uMsg) {
    case WM_INITDIALOG:
        {
            DWORD negotiationVersion{};
            DWORD rc = WlanOpenHandle(WLAN_API_VERSION, nullptr, &negotiationVersion, &hClientHandle);
            if (rc != ERROR_SUCCESS) {
                __debugbreak();
                EndDialog(hwndDlg, IDCANCEL);
                return FALSE;
            }

            
            rc = WlanEnumInterfaces(hClientHandle, nullptr, &interfaceList);
            if (rc != ERROR_SUCCESS) {
                __debugbreak();
                EndDialog(hwndDlg, IDCANCEL);
                return FALSE;
            }

            // Populate list with interfaces
            HWND hwndInterfaceList = GetDlgItem(hwndDlg, IDC_INTERFACE_LIST);
                        
            for (DWORD i = 0; i < interfaceList->dwNumberOfItems; ++i) {                           
                int interfaceIndex = ListBox_AddString(hwndInterfaceList, interfaceList->InterfaceInfo[i].strInterfaceDescription);
                ListBox_SetItemData(hwndInterfaceList, interfaceIndex, &interfaceList->InterfaceInfo[i].InterfaceGuid);
            }            
        }
        break;
    case WM_CLOSE:
        if (interfaceList) {
            WlanFreeMemory(interfaceList);
        }
        if (hClientHandle) {
            WlanCloseHandle(hClientHandle, nullptr);
            hClientHandle = nullptr;
        }     
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_INTERFACE_LIST:
            switch (HIWORD(wParam)) {
            case LBN_SELCHANGE:
                {
                    HWND hwndInterfaceList = GetDlgItem(hwndDlg, IDC_INTERFACE_LIST);
                    HWND hwndProfileList = GetDlgItem(hwndDlg, IDC_PROFILE_LIST);

                    int interfaceIndex = ListBox_GetCurSel(hwndInterfaceList);
                    LPGUID interfaceGuid = reinterpret_cast<LPGUID>(ListBox_GetItemData(hwndInterfaceList, interfaceIndex));

                    PWLAN_PROFILE_INFO_LIST profileList = nullptr;
                    DWORD rc = WlanGetProfileList(hClientHandle, interfaceGuid, nullptr, &profileList);
                    if (rc != ERROR_SUCCESS) {
                        __debugbreak();
                    }

                    ListBox_ResetContent(hwndProfileList);
                    for (DWORD i = 0; i < profileList->dwNumberOfItems; ++i) {
                        ListBox_AddString(hwndProfileList, profileList->ProfileInfo[i].strProfileName);
                    }

                    WlanFreeMemory(profileList);
                }
                break;
            }            
            break;
        case IDC_PROFILE_LIST:            
            break;
        case IDOK:
            // TODO: trigger edit profile
            {
                HWND hwndInterfaceList = GetDlgItem(hwndDlg, IDC_INTERFACE_LIST);
                HWND hwndProfileList = GetDlgItem(hwndDlg, IDC_PROFILE_LIST);

                int interfaceIndex = ListBox_GetCurSel(hwndInterfaceList);
                LPGUID interfaceGuid = reinterpret_cast<LPGUID>(ListBox_GetItemData(hwndInterfaceList, interfaceIndex));

                int profileIndex = ListBox_GetCurSel(hwndProfileList);
                if (profileIndex == LB_ERR) {
                    __debugbreak();
                }
                WCHAR* profileName = (WCHAR*)malloc((ListBox_GetTextLen(hwndProfileList, profileIndex) + 1) * sizeof(WCHAR));
                if (!profileName) {
                    __debugbreak();
                }

                ListBox_GetText(hwndProfileList, profileIndex, profileName);

                WLAN_REASON_CODE reasonCode = 0;
                DWORD rc = WlanUIEditProfile(WLAN_UI_API_VERSION, profileName, interfaceGuid, hwndDlg, WLSecurityPage, nullptr, &reasonCode);
                if (rc != ERROR_SUCCESS) {
                    __debugbreak();
                }
            }
            
            break;
        case IDCANCEL:
            EndDialog(hwndDlg, wParam);
            return TRUE;
        }
        break;
    }

    return FALSE;
}