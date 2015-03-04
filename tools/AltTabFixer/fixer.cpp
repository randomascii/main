/*
Copyright 2013 Cygnus Software

This code exists to correct a Windows 7 problem with alt+tab that has been
documented at:
http://randomascii.wordpress.com/2011/10/16/alttab-depth-inversion-bug/

This program is distributed for free, with no warranty.
*/

#include "stdafx.h"
#include <map>
#include <string>
#include "TlHelp32.h"

static std::map<DWORD, std::wstring> g_processList;

static void EnumerateProcesses()
{
	g_processList.clear();
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
	if(hSnapshot)
	{
		PROCESSENTRY32 pe32;
		pe32.dwSize = sizeof(PROCESSENTRY32);
		if(Process32First(hSnapshot,&pe32))
		{
			do
			{
				g_processList[pe32.th32ProcessID] = pe32.szExeFile;
			} while(Process32Next(hSnapshot,&pe32));
		}
		CloseHandle(hSnapshot);
	}
}

static int totalCount = 0;
static int clearedCount = 0;

static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
	DWORD processID = 0;
	DWORD threadID = GetWindowThreadProcessId(hwnd, &processID);

	wchar_t title[1000];
	GetWindowText(hwnd, title, ARRAYSIZE(title));

	LONG style = GetWindowLong(hwnd, GWL_STYLE);
	LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
	// http://msdn.microsoft.com/en-us/library/windows/desktop/ff700543(v=vs.85).aspx
	if (exStyle & WS_EX_TOPMOST)
	{
		if (style & WS_VISIBLE)
		{
			wprintf(L"    Title: '%s' style: %08x exStyle: %08x", title, style, exStyle);
			RECT rect;
			GetWindowRect(hwnd, &rect);
			wprintf(L" %d, %d to %d, %d", rect.left, rect.top, rect.right, rect.bottom);
			std::wstring processName = g_processList[processID];
			wprintf(L", %s\n", processName.c_str());
			++totalCount;
			// ipoint.exe, iexplore.exe, and sidebar.exe should not have visible always-on-top
			// windows. When they do is is known to cause problems. The safe thing to do would
			// be to only clear the visible flag for windows owned by these processes. But,
			// there could be other programs that trigger this bug, and a zero sized, visible,
			// always-on-top window is pointless, so the safe thing to do is clear the visible
			// flag regardless of who owns the window.
			//if (processName == L"ipoint.exe" || processName == L"iexplore.exe" || processName == L"sidebar.exe")
			{
				// The problematic windows are, so far, always located at 0,0 and have zero size.
				if (rect.left == 0 && rect.top == 0 && rect.right == 0 && rect.bottom == 0)
				{
					printf("        Clearing visible flag.\n");
					SetWindowLong(hwnd, GWL_STYLE, style & ~WS_VISIBLE);
					++clearedCount;
				}
			}
		}
	}

	return TRUE;
}

void RunFixer()
{
	totalCount = 0;
	clearedCount = 0;

	EnumerateProcesses();

	printf("Listing all visible always-on-top windows.\n");
	EnumWindows(EnumWindowsProc, 0);
	printf("%d visible always-on-top windows found, %d fixed.\n", totalCount, clearedCount);
}
