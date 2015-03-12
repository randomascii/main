#pragma once

#include <string>

class ChildProcess
{
public:
	ChildProcess(std::string exePath);
	~ChildProcess();

	// Returns true if the process started.
	bool Run(bool showCommand, std::string args);

	// This can be called even if the process doesn't start, but
	// it will return zero. If the process is still running it
	// will return STILL_ACTIVE, but don't call it in a loop
	// waiting for a non-STILL_ACTIVE result!
	DWORD GetExitCode();

	// This can be called even if the process doesn't start, but
	// it will just return immediately. Otherwise it will wait
	// for the process to end. This is called by the destructor
	// so calling this is strictly optional.
	void WaitForCompletion();

private:
	// Path to the executable to be run, and its process handle.
	std::string exePath_;
	HANDLE hProcess_;

	// This string is written to by the listener thread.
	// Once the listener thread has exited the main thread can
	// display it to the user.
	std::string processOutput_;

	// Output handles for the child process -- connected to the pipe.
	HANDLE hStdOutput_;
	HANDLE hStdError_;

	// Pipe to read from, and the handle to the pipe reading thread.
	HANDLE hPipe_;
	HANDLE hChildThread_;
	// Thread functions for reading from the pipe.
	static DWORD WINAPI ListenerThreadStatic(LPVOID);
	DWORD ListenerThread();
};
