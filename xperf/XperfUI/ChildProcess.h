#pragma once

#include <string>

class ChildProcess
{
public:
	ChildProcess(std::wstring exePath);
	~ChildProcess();

	// Returns true if the process started.
	bool Run(bool showCommand, std::wstring args);

	// This can be called even if the process doesn't start, but
	// it will return zero. If the process is still running it
	// will wait until the process returns and then get the exit code.
	DWORD GetExitCode();

	// This can be called even if the process doesn't start, but
	// it will just return immediately. Otherwise it will wait
	// for the process to end. This is called by the destructor
	// so calling this is strictly optional.
	void WaitForCompletion();

	// IsStillRunning returns when the child process exits or
	// when new output is available. It returns true if the
	// child is still running.
	bool IsStillRunning();
	// Remove and return the accumulated output text. Typically
	// this is called in an IsStillRunning() loop.
	std::wstring RemoveOutputText();

private:
	// Path to the executable to be run, and its process handle.
	std::wstring exePath_;
	HANDLE hProcess_;

	// This will be signaled when fresh child-process output is available.
	HANDLE hOutputAvailable_;

	// This string is written to by the listener thread.
	// Don't modify processOutput_ without acquiring the lock,
	// unless the pipe thread is known to be not running.
	CCriticalSection outputLock_;
	std::wstring processOutput_;

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
