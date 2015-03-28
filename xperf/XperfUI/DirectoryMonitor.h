#pragma once

#include <string>

class DirectoryMonitor
{
public:
	DirectoryMonitor(CWnd* pMainWindow);
	~DirectoryMonitor();

	void StartThread(const std::wstring* traceDir);

private:
	static DWORD WINAPI DirectoryMonitorThreadStatic(LPVOID);
	DWORD DirectoryMonitorThread();

	HANDLE hThread_ = 0;
	HANDLE hShutdownRequest_ = 0;

	CWnd* mainWindow_ = nullptr;
	const std::wstring* traceDir_ = nullptr;
};
