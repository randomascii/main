/*
Copyright 2013 Cygnus Software

This program looks for the image file that is the current desktop background.
It opens it and shows its path in a message box. This is particularly useful
if you have Windows cycling through your Top Rated Photos. This option is
available in Windows 7 in the Change Desktop Background control panel item.

This program is distributed for free, with no warranty.
*/

#include "stdafx.h"

int __stdcall WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    bool success = false;
    HKEY hKey;
    LONG result = RegOpenKeyEx(HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Internet Explorer\\Desktop\\General",
        0,
        KEY_READ,
        &hKey);
    WCHAR buffer[MAX_PATH];
    buffer[0] = 0;
    if (!result)
    {
        DWORD size = sizeof(buffer);
        DWORD type;
        result = RegQueryValueEx(hKey,
            L"WallpaperSource",
            0,
            &type,
            (LPBYTE)buffer,
            &size);
        if (result == ERROR_SUCCESS && type == REG_SZ)
        {
            HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
            if (SUCCEEDED(hr))
            {
                HINSTANCE shellResult = ShellExecute(0, L"open", buffer, 0, 0, SW_SHOWNORMAL);
                if ((DWORD_PTR)shellResult > 32)
                {
                    success = true;
                }
            }
            CoUninitialize();
        }
        RegCloseKey(hKey);
    }

    if (success)
	{
        MessageBoxW(0, buffer, L"Wallpaper image found", MB_OK);
	}
	else
    {
		if (buffer[0])
		{
	        MessageBoxW(0, L"Oops - failure to find your desktop wallpaper image.", buffer, MB_OK);
		}
		else
		{
			MessageBoxW(0, L"Oops - failure to find your desktop wallpaper image.", L"So sad", MB_OK);
		}
    }

    return 0;
}
