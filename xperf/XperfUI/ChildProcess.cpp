#include "stdafx.h"
#include "ChildProcess.h"
#include <assert.h>
#include <vector>

const char* kPipeName = "\\\\.\\PIPE\\xperfUIPipe";

ChildProcess::ChildProcess(std::string exePath)
	: exePath_(std::move(exePath))
	, hProcess_(0)
	, hStdOutput_(INVALID_HANDLE_VALUE)
	, hStdError_(INVALID_HANDLE_VALUE)
{
	// Create the pipe here so that it is guaranteed to be created before
	// we try starting the process.
	hPipe_ = CreateNamedPipe(kPipeName,
		PIPE_ACCESS_DUPLEX | PIPE_TYPE_BYTE | PIPE_READMODE_BYTE,
		PIPE_WAIT,
		1,
		1024 * 16,
		1024 * 16,
		NMPWAIT_USE_DEFAULT_WAIT,
		NULL);
	hChildThread_ = CreateThread(0, 0, ListenerThreadStatic, this, 0, 0);
}

ChildProcess::~ChildProcess()
{
	DWORD exitCode = 0;
	if (hProcess_)
	{
		WaitForCompletion();
		exitCode = GetExitCode();
		CloseHandle(hProcess_);
	}

	// Once the process is finished we have to close the stderr/stdout
	// handles so that the listener thread will exit. We also have to
	// close these if the process never started.
	if (hStdError_ != INVALID_HANDLE_VALUE)
		CloseHandle(hStdError_);
	if (hStdOutput_ != INVALID_HANDLE_VALUE)
		CloseHandle(hStdOutput_);

	// Wait for the listener thread to exit.
	if (hChildThread_)
	{
		WaitForSingleObject(hChildThread_, INFINITE);
		CloseHandle(hChildThread_);
	}

	// Clean up.
	if (hPipe_)
		CloseHandle(hPipe_);

	// Now that the child thread has exited we can safely
	// read from and print the process output.
	outputPrintf("%s", processOutput_.c_str());
	if (exitCode)
		outputPrintf("Process exit code was %08x (%d)\n", exitCode, exitCode);
}

DWORD WINAPI ChildProcess::ListenerThreadStatic(LPVOID pVoidThis)
{
	ChildProcess* pThis = static_cast<ChildProcess*>(pVoidThis);
	return pThis->ListenerThread();
}

DWORD ChildProcess::ListenerThread()
{
	if (ConnectNamedPipe(hPipe_, NULL) || GetLastError() == ERROR_PIPE_CONNECTED)   // wait for someone to connect to the pipe
	{
		char buffer[1024];
		DWORD dwRead;
		while (ReadFile(hPipe_, buffer, sizeof(buffer) - 1, &dwRead, NULL) != FALSE)
		{
			buffer[dwRead] = 0;
			OutputDebugString(buffer);
			processOutput_ += buffer;
		}
	}
	else
	{
		OutputDebugString("Connect failed.\n");
	}

	DisconnectNamedPipe(hPipe_);

	return 0;
}


bool ChildProcess::Run(bool showCommand, std::string args)
{
	assert(!hProcess_);

	if (showCommand)
		outputPrintf("%s\n", args.c_str());

	SECURITY_ATTRIBUTES security = { sizeof(security), 0, TRUE };

	hStdOutput_ = CreateFile(kPipeName, GENERIC_WRITE, 0, &security,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, INVALID_HANDLE_VALUE);
	if (hStdOutput_ == INVALID_HANDLE_VALUE)
		return false;
	if (!DuplicateHandle(GetCurrentProcess(), hStdOutput_, GetCurrentProcess(),
		&hStdError_, 0, TRUE, DUPLICATE_SAME_ACCESS))
		return false;

	STARTUPINFO startupInfo = {};
	startupInfo.hStdOutput = hStdOutput_;
	startupInfo.hStdError = hStdError_;
	startupInfo.hStdInput = INVALID_HANDLE_VALUE;
	startupInfo.dwFlags = STARTF_USESTDHANDLES;

	PROCESS_INFORMATION processInfo = {};
	DWORD flags = CREATE_NO_WINDOW;
	// Wacky CreateProcess rules say args has to be writable!
	std::vector<char> argsCopy(args.size() + 1);
	strcpy_s(&argsCopy[0], argsCopy.size(), args.c_str());
	BOOL success = CreateProcess(exePath_.c_str(), &argsCopy[0], NULL, NULL,
		TRUE, flags, NULL, NULL, &startupInfo, &processInfo);
	if (success)
	{
		CloseHandle(processInfo.hThread);
		hProcess_ = processInfo.hProcess;
		return true;
	}

	return false;
}

DWORD ChildProcess::GetExitCode()
{
	if (!hProcess_)
		return 0;
	DWORD result;
	(void)GetExitCodeProcess(hProcess_, &result);
	return result;
}

void ChildProcess::WaitForCompletion()
{
	if (!hProcess_)
		return;

	WaitForSingleObject(hProcess_, INFINITE);
}
