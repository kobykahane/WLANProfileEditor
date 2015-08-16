#include <WinSock2.h>
#include <Windows.h>
#include <windowsx.h>
#include <objbase.h>
#include <wlanapi.h>

#include <vector>

#include "resource.h"

#pragma comment(lib, "wlanapi")
#pragma comment(lib, "wlanui")

INT_PTR CALLBACK MainDialogProc(_In_ HWND hwndDlg, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam);

_Use_decl_annotations_ int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr)) {
        abort();
    }

    INT_PTR result = DialogBox(hInstance, MAKEINTRESOURCE(IDD_MAIN_DIALOG), nullptr, MainDialogProc);

    if (result <= 0) {
        abort();
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
                abort();
            }
            
            rc = WlanEnumInterfaces(hClientHandle, nullptr, &interfaceList);
            if (rc != ERROR_SUCCESS) {
                abort();
            }

            // Populate list with interfaces
            HWND hwndInterfaceList = GetDlgItem(hwndDlg, IDC_INTERFACE_LIST);
            if (hwndInterfaceList == nullptr) {
                abort();
            }
                        
            for (DWORD i = 0; i < interfaceList->dwNumberOfItems; ++i) {                           
                int interfaceIndex = ListBox_AddString(hwndInterfaceList, interfaceList->InterfaceInfo[i].strInterfaceDescription);
                if (interfaceIndex == LB_ERR || interfaceIndex == LB_ERRSPACE) {
                    abort();
                }
                int rci = ListBox_SetItemData(hwndInterfaceList, interfaceIndex, &interfaceList->InterfaceInfo[i].InterfaceGuid);
                if (rci == LB_ERR) {
                    abort();
                }
            }            
        }
        break;
    case WM_CLOSE:
        if (interfaceList) {
            WlanFreeMemory(interfaceList);
        }
        if (hClientHandle) {
            DWORD rc = WlanCloseHandle(hClientHandle, nullptr);
            if (rc != ERROR_SUCCESS) {
                abort();
            }
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
                    if (hwndInterfaceList == nullptr) {
                        abort();
                    }
                    HWND hwndProfileList = GetDlgItem(hwndDlg, IDC_PROFILE_LIST);
                    if (hwndProfileList == nullptr) {
                        abort();
                    }

                    int interfaceIndex = ListBox_GetCurSel(hwndInterfaceList);
                    if (interfaceIndex == LB_ERR) {
                        abort();
                    }
                    LRESULT itemData = ListBox_GetItemData(hwndInterfaceList, interfaceIndex);
                    if (itemData == LB_ERR) {
                        abort();
                    }
                    LPGUID interfaceGuid = reinterpret_cast<LPGUID>(itemData);
                    
                    PWLAN_PROFILE_INFO_LIST profileList = nullptr;
                    DWORD rc = WlanGetProfileList(hClientHandle, interfaceGuid, nullptr, &profileList);
                    if (rc != ERROR_SUCCESS) {
                        abort();
                    }

                    ListBox_ResetContent(hwndProfileList);
                    LRESULT rcl;
                    for (DWORD i = 0; i < profileList->dwNumberOfItems; ++i) {
                        rcl = ListBox_AddString(hwndProfileList, profileList->ProfileInfo[i].strProfileName);
                        if (rcl == LB_ERR || rcl == LB_ERRSPACE) {
                            abort();
                        }

                    }

                    WlanFreeMemory(profileList);
                }
                break;
            }            
            break;
        case IDC_PROFILE_LIST:            
            break;
        case IDOK:
            {
                HWND hwndInterfaceList = GetDlgItem(hwndDlg, IDC_INTERFACE_LIST);
                if (hwndInterfaceList == nullptr) {
                    abort();
                }
                HWND hwndProfileList = GetDlgItem(hwndDlg, IDC_PROFILE_LIST);
                if (hwndProfileList == nullptr) {
                    abort();
                }

                int interfaceIndex = ListBox_GetCurSel(hwndInterfaceList);
                if (interfaceIndex == LB_ERR) {
                    abort();
                }

                LRESULT itemData = ListBox_GetItemData(hwndInterfaceList, interfaceIndex);
                if (itemData == LB_ERR) {
                    abort();
                }

                LPGUID interfaceGuid = reinterpret_cast<LPGUID>(itemData);

                int profileIndex = ListBox_GetCurSel(hwndProfileList);
                if (profileIndex == LB_ERR) {
                    abort();
                }

                int len = ListBox_GetTextLen(hwndProfileList, profileIndex) + 1;
                if (len == LB_ERR) {
                    abort();
                }
                std::vector<WCHAR> profileName(len);

                LRESULT rcl = ListBox_GetText(hwndProfileList, profileIndex, profileName.data());
                if (rcl == LB_ERR) {
                    abort();
                }

                WLAN_REASON_CODE reasonCode = 0;
                DWORD rc = WlanUIEditProfile(WLAN_UI_API_VERSION, profileName.data(), interfaceGuid, hwndDlg, WLSecurityPage, nullptr, &reasonCode);
                if (rc != ERROR_SUCCESS) {
                    abort();
                }
            }
            
            break;
        case IDCANCEL:
            {
                BOOL rc = EndDialog(hwndDlg, wParam);
                if (rc == FALSE) {
                    abort();
                }
            }
            
            return TRUE;
        }
        break;
    }

    return FALSE;
}