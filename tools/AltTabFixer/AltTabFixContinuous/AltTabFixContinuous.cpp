/*
Copyright 2013 Cygnus Software

This code exists to correct a Windows 7 problem with alt+tab that has been
documented at:
http://randomascii.wordpress.com/2011/10/16/alttab-depth-inversion-bug/

This program is distributed for free, with no warranty.
*/

#include "stdafx.h"
#include "AltTabFixContinuous.h"
#include <assert.h>

LRESULT CALLBACK LowLevelKeyboardHook(int nCode, WPARAM wParam, LPARAM lParam)
{
	assert(nCode == HC_ACTION);
	// wParam is WM_KEYDOWN, WM_KEYUP, WM_SYSKEYDOWN, or WM_SYSKEYUP

	KBDLLHOOKSTRUCT* pKbdLLHook = (KBDLLHOOKSTRUCT*)lParam;

	if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)
	{
		if (pKbdLLHook->vkCode == VK_TAB)
		{
			RunFixer();
		}
	}
	
	return CallNextHookEx(0, nCode, wParam, lParam);
}

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	const auto* keyName = L"Software\\CygnusSoftware\\AltTabFixContinuous";
	const auto* valueName = L"eulaaccepted";
	bool accepted = false;
	if (RegGetValue(HKEY_CURRENT_USER, keyName, valueName, RRF_RT_ANY,
				NULL, NULL, NULL) == ERROR_SUCCESS)
		accepted = true;

	if (!accepted)
	{
		int answer = MessageBox(NULL, L"This program will sit in the background and attempt to fix the alt+tab problems documented "
					L"at http://randomascii.wordpress.com/2011/10/16/alttab-depth-inversion-bug/. It does this by monitoring the keyboard "
					L"and on each press of the tab key hiding any zero-by-zero always-on-top windows "
					L"that are visible. This is believed to be safe and helpful, but no warranty is provided. "
					L"This program is free and is Copyright 2013 Cygnus Software.\r\n"
					L"\r\n"
					L"Would you like this program to run? If you say yes you will not be asked again.",
					L"Alt Tab Fix Continuous", MB_YESNO);
		if (answer != IDYES)
			return 0;

		DWORD value = 1;
		RegSetKeyValue(HKEY_CURRENT_USER, keyName, valueName, REG_DWORD, &value, sizeof(value));
	}

	// Install a hook so that this thread will receive all keyboard messages. They must be
	// processed in a timely manner or else bad things will happen. Doing this on a
	// separate thread is a good idea, but even then bad things will happen to your system
	// if you halt in a debugger. Even simple things like calling printf() from the hook
	// can easily cause system deadlocks which render the mouse unable to move!
	HHOOK keyHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardHook, NULL, 0);

	if (!keyHook)
		return 0;

	// Run a message pump -- necessary so that the hooks will be processed
	BOOL bRet;
	MSG msg;
	// Keeping pumping messages until WM_QUIT is received. If this is opened
	// in a child thread then you can terminate it by using PostThreadMessage
	// to send WM_QUIT.
	while( (bRet = GetMessage( &msg, NULL, 0, 0 )) != 0)
	{ 
		if (bRet == -1)
		{
			// handle the error and possibly exit
			break;
		}
		else
		{
			TranslateMessage(&msg); 
			DispatchMessage(&msg); 
		}
	}

	// Unhook and exit.
	if (keyHook)
		UnhookWindowsHookEx(keyHook);

	return 0;
}
