#include "stdafx.h"
#include "XperfUI.h"
#include "XperfUIDlg.h"

#include "About.h"
#include "afxdialogex.h"
#include "ChildProcess.h"
#include <ETWProviders\etwprof.h>
#include "Utility.h"
#include <direct.h>
#include <vector>
#include <algorithm>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// Send this when the list of traces needs to be updated.
const int WM_UPDATETRACELIST = WM_USER + 10;

const int kRecordTraceHotKey = 1234;


// CXperfUIDlg dialog


static CXperfUIDlg* pMainWindow;

void outputPrintf(_Printf_format_string_ const char* pFormat, ...)
{
	va_list args;
	va_start(args, pFormat);
	pMainWindow->vprintf(pFormat, args);
	va_end(args);
}

void CXperfUIDlg::vprintf(const char* pFormat, va_list args)
{
	char buffer[5000];
	vsnprintf_s(buffer, _TRUNCATE, pFormat, args);

	for (const char* pBuf = buffer; *pBuf; ++pBuf)
	{
		// Need \r\n as a line separator.
		if (pBuf[0] == '\n')
		{
			// Don't add a line separator at the very beginning.
			if (!output_.empty())
				output_ += "\r\n";
		}
		else
			output_ += pBuf[0];
	}

	SetDlgItemText(IDC_OUTPUT, output_.c_str());

	// Make sure the end of the data is visible.
	btOutput_.SetSel(0, -1);
	btOutput_.SetSel(-1, -1);

	// Display the results immediately.
	UpdateWindow();
}

CXperfUIDlg::CXperfUIDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CXperfUIDlg::IDD, pParent)
{
	pMainWindow = this;
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CXperfUIDlg::~CXperfUIDlg()
{
}

void CXperfUIDlg::ShutdownTasks()
{
	if (bShutdownCompleted_)
		return;
	bShutdownCompleted_ = true;
	SaveNotesIfNeeded();
	if (bIsTracing_)
	{
		StopTracing(false);
	}

	SetHeapTracing(true);
}

void CXperfUIDlg::OnCancel()
{
	ShutdownTasks();

	CDialog::OnCancel();
}

void CXperfUIDlg::OnClose()
{
	ShutdownTasks();

	CDialog::OnClose();
}

void CXperfUIDlg::OnOK()
{
	ShutdownTasks();
	CDialog::OnOK();
}

void CXperfUIDlg::DoDataExchange(CDataExchange* pDX)
{
	DDX_Control(pDX, IDC_STARTTRACING, btStartTracing_);
	DDX_Control(pDX, IDC_SAVETRACEBUFFERS, btSaveTraceBuffers_);
	DDX_Control(pDX, IDC_STOPTRACING, btStopTracing_);

	DDX_Control(pDX, IDC_COMPRESSTRACE, btCompress_);
	DDX_Control(pDX, IDC_CPUSAMPLINGCALLSTACKS, btSampledStacks_);
	DDX_Control(pDX, IDC_CONTEXTSWITCHCALLSTACKS, btCswitchStacks_);
	DDX_Control(pDX, IDC_FASTSAMPLING, btFastSampling_);
	DDX_Control(pDX, IDC_SHOWCOMMANDS, btShowCommands_);

	DDX_Control(pDX, IDC_INPUTTRACING, btInputTracing_);
	DDX_Control(pDX, IDC_INPUTTRACING_LABEL, btInputTracingLabel_);
	DDX_Control(pDX, IDC_TRACINGMODE, btTracingMode_);
	DDX_Control(pDX, IDC_TRACELIST, btTraces_);
	DDX_Control(pDX, IDC_TRACENOTES, btTraceNotes_);
	DDX_Control(pDX, IDC_OUTPUT, btOutput_);

	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CXperfUIDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_STARTTRACING, &CXperfUIDlg::OnBnClickedStarttracing)
	ON_BN_CLICKED(IDC_STOPTRACING, &CXperfUIDlg::OnBnClickedStoptracing)
	ON_BN_CLICKED(IDC_COMPRESSTRACE, &CXperfUIDlg::OnBnClickedCompresstrace)
	ON_BN_CLICKED(IDC_CPUSAMPLINGCALLSTACKS, &CXperfUIDlg::OnBnClickedCpusamplingcallstacks)
	ON_BN_CLICKED(IDC_CONTEXTSWITCHCALLSTACKS, &CXperfUIDlg::OnBnClickedContextswitchcallstacks)
	ON_BN_CLICKED(IDC_SHOWCOMMANDS, &CXperfUIDlg::OnBnClickedShowcommands)
	ON_BN_CLICKED(IDC_FASTSAMPLING, &CXperfUIDlg::OnBnClickedFastsampling)
	ON_CBN_SELCHANGE(IDC_INPUTTRACING, &CXperfUIDlg::OnCbnSelchangeInputtracing)
	ON_MESSAGE(WM_UPDATETRACELIST, UpdateTraceListHandler)
	ON_LBN_DBLCLK(IDC_TRACELIST, &CXperfUIDlg::OnLbnDblclkTracelist)
	ON_WM_GETMINMAXINFO()
	ON_WM_SIZE()
	ON_LBN_SELCHANGE(IDC_TRACELIST, &CXperfUIDlg::OnLbnSelchangeTracelist)
	ON_BN_CLICKED(IDC_ABOUT, &CXperfUIDlg::OnBnClickedAbout)
	ON_BN_CLICKED(IDC_SAVETRACEBUFFERS, &CXperfUIDlg::OnBnClickedSavetracebuffers)
	ON_MESSAGE(WM_HOTKEY, OnHotKey)
	ON_WM_CLOSE()
	ON_CBN_SELCHANGE(IDC_TRACINGMODE, &CXperfUIDlg::OnCbnSelchangeTracingmode)
END_MESSAGE_MAP()


// CXperfUIDlg message handlers

void CXperfUIDlg::SetSymbolPath()
{
	// Make sure that the symbol paths are set.

#pragma warning(suppress : 4996)
	const char* symPath = getenv("_NT_SYMBOL_PATH");
	if (!symPath)
		(void)_putenv("_NT_SYMBOL_PATH=SRV*c:\\symbols*\\\\symsrv\\symbols;SRV*c:\\symbols*http://msdl.microsoft.com/download/symbols");
#pragma warning(suppress : 4996)
	const char* symCachePath = getenv("_NT_SYMCACHE_PATH");
	if (!symCachePath)
		(void)_putenv("_NT_SYMCACHE_PATH=c:\\symcache");
}


// This function monitors the traceDir_ directory and sends a message to the main thread
// whenever anything changes. That's it. All UI work is done in the main thread.
DWORD __stdcall DirectoryMonitorThread(LPVOID voidTraceDir)
{
	const char* traceDir = (const char*)voidTraceDir;

	HANDLE hChangeHandle = FindFirstChangeNotification(traceDir, FALSE, FILE_NOTIFY_CHANGE_FILE_NAME);

	if (hChangeHandle == INVALID_HANDLE_VALUE)
	{
		assert(0);
		return 0;
	}

	for (;;)
	{
		DWORD dwWaitStatus = WaitForSingleObject(hChangeHandle, INFINITE);

		switch (dwWaitStatus)
		{
		case WAIT_OBJECT_0:
			pMainWindow->PostMessage(WM_UPDATETRACELIST, 0, 0);
			if (FindNextChangeNotification(hChangeHandle) == FALSE)
			{
				assert(0);
				return 0;
			}
			break;

		default:
			assert(0);
			return 0;
		}
	}

	assert(0);

	return 0;
}


BOOL CXperfUIDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	CRect windowRect;
	GetWindowRect(&windowRect);
	initialWidth_ = lastWidth_ = windowRect.Width();
	initialHeight_ = lastHeight_ = windowRect.Height();

	// 0x41 is 'C', compatible with wprui
	RegisterHotKey(*this, kRecordTraceHotKey, MOD_WIN + MOD_CONTROL, 0x43);

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	if (!PathFileExists(GetXperfPath().c_str()))
	{
		AfxMessageBox((GetXperfPath() + " does not exist. Please install WPT 8.1. Exiting.").c_str());
		exit(10);
	}

	char documents[MAX_PATH];
	if (!SHGetSpecialFolderPath(*this, documents, CSIDL_MYDOCUMENTS, TRUE))
	{
		assert(!"Failed to find My Documents directory.\n");
		exit(10);
	}
	std::string defaultTraceDir = documents + std::string("\\xperftraces\\");
	traceDir_ = GetDirectory("xperftracedir", defaultTraceDir);

	tempTraceDir_ = GetDirectory("xperftemptracedir", traceDir_);

	SetSymbolPath();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	CheckDlgButton(IDC_COMPRESSTRACE, bCompress_);
	CheckDlgButton(IDC_CPUSAMPLINGCALLSTACKS, bSampledStacks_);
	CheckDlgButton(IDC_CONTEXTSWITCHCALLSTACKS, bCswitchStacks_);



	btInputTracing_.AddString(_T("Off"));
	btInputTracing_.AddString(_T("Private"));
	btInputTracing_.AddString(_T("Full"));
	btInputTracing_.SetCurSel(InputTracing_);

	btTracingMode_.AddString(_T("Tracing to file"));
	btTracingMode_.AddString(_T("Circular buffer tracing"));
	btTracingMode_.AddString(_T("Heap tracing to file"));
	btTracingMode_.SetCurSel(tracingMode_);

	UpdateEnabling();
	btTraceNotes_.EnableWindow(false); // This window always starts out disabled.

	// Don't change traceDir_ - the DirectoryMonitorThread has a pointer to it.
	(void)CreateThread(nullptr, 0, DirectoryMonitorThread, const_cast<char*>(traceDir_.c_str()), 0, 0);

	RegisterProviders();
	DisablePagingExecutive();

	UpdateTraceList();

	if (toolTip_.Create(this))
	{
		toolTip_.AddTool(&btStartTracing_, _T("Start ETW tracing."));

		toolTip_.AddTool(&btCompress_, _T("Only uncheck this if you record traces on Windows 8 and above and want to analyze "
					"them on Windows 7 and below.\n"
					"Enable ETW trace compression. On Windows 8 and above this compresses traces "
					"as they are saved, making them 5-10x smaller. However compressed traces cannot be loaded on "
					"Windows 7 or earlier. On Windows 7 this setting has no effect."));
		toolTip_.AddTool(&btCswitchStacks_, _T("This enables recording of call stacks on context switches, from both "
					"the thread being switched in and the readying thread. This should only be disabled if the performance "
					"of functions like WaitForSingleObject and SetEvent appears to be distorted, which can happen when the "
					"context-switch rate is very high."));
		toolTip_.AddTool(&btSampledStacks_, _T("This enables recording of call stacks on CPU sampling events, which "
			"by default happen at 1 KHz. This should rarely be disabled."));
		toolTip_.AddTool(&btFastSampling_, _T("Checking this changes the CPU sampling frequency from the default of "
					"~1 KHz to the maximum speed of ~8 KHz. This increases the data rate and thus the size of traces "
					"but can make investigating brief CPU-bound performance problems (such as a single long frame) "
					"more practical."));
		toolTip_.AddTool(&btShowCommands_, _T("This tells XperfUI to display the xperf.exe and other commands being "
			"executed. This can be helpful for diagnostic purposes but is not normally needed."));

		const TCHAR* pInputTip = _T("Input tracing inserts custom ETW events into traces which can be helpful when "
					"investigating performance problems that are correlated with user input. The default setting of "
					"'private' records alphabetic keys as 'A' and numeric keys as '0'. The 'full' setting records "
					"alphanumeric details. Both 'private' and 'full' record mouse movement and button clicks. The "
					"'off' setting records no input.");
		toolTip_.AddTool(&btInputTracingLabel_, pInputTip);
		toolTip_.AddTool(&btInputTracing_, pInputTip);

		toolTip_.AddTool(&btTracingMode_, _T("Select whether to trace straight to disk or to in-memory circular buffers."));

		toolTip_.AddTool(&btTraces_, _T("This is a list of all traces found in %xperftracedir%, which defaults to "
					"documents\\xperftraces."));
		toolTip_.AddTool(&btTraceNotes_, _T("Trace notes are intended for recording information about ETW traces, such "
					"as an analysis of what was discovered in the trace. Trace notes are auto-saved to a parallel text "
					"file - just type your analysis."));

		toolTip_.SetMaxTipWidth(400);
		toolTip_.Activate(TRUE);
	}

	SetHeapTracing(false);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

std::string CXperfUIDlg::GetDirectory(const char* env, const std::string& default)
{
	// Get a directory (from an environment variable, if set) and make sure it exists.
	std::string result = default;
#pragma warning(suppress : 4996)
	const char* traceDir = getenv(env);
	if (traceDir)
	{
		result = traceDir;
	}
	// Make sure the name ends with a backslash.
	if (!result.empty() && result[result.size() - 1] != '\\')
		result += '\\';
	if (!PathFileExists(result.c_str()))
	{
		(void)_mkdir(result.c_str());
	}
	if (!PathIsDirectory(result.c_str()))
	{
		AfxMessageBox((result + " is not a directory. Exiting.").c_str());
		exit(10);
	}
	return result;
}

void CXperfUIDlg::RegisterProviders()
{
	std::string dllSource = GetExeDir() + "ETWProviders.dll";
#pragma warning(suppress:4996)
	const char* temp = getenv("temp");
	if (!temp)
		return;
	std::string dllDest = temp;
	dllDest += "\\ETWProviders.dll";
	if (!CopyFile(dllSource.c_str(), dllDest.c_str(), FALSE))
	{
		outputPrintf("Registering of ETW providers failed due to copy error.\n");
		return;
	}
	char systemDir[MAX_PATH];
	systemDir[0] = 0;
	GetSystemDirectory(systemDir, ARRAYSIZE(systemDir));
	std::string wevtPath = systemDir + std::string("\\wevtutil.exe");

	for (int pass = 0; pass < 2; ++pass)
	{
		ChildProcess child(wevtPath);
		std::string args = pass ? " im" : " um";
		args += " \"" + GetExeDir() + "etwproviders.man\"";
		child.Run(bShowCommands_, "wevtutil.exe" + args);
	}
}


// Tell Windows to keep 64-bit kernel metadata in memory so that
// stack walking will work. Just do it -- don't ask.
void CXperfUIDlg::DisablePagingExecutive()
{
	// http://blogs.msdn.com/b/oldnewthing/archive/2005/02/01/364563.aspx
	BOOL f64 = FALSE;
	bool bIsWin64 = IsWow64Process(GetCurrentProcess(), &f64) && f64;

	if (bIsWin64)
	{
		const char* keyName = "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management";
		SetRegistryDWORD(HKEY_LOCAL_MACHINE, keyName, "DisablePagingExecutive", 1);
	}
}

void CXperfUIDlg::UpdateEnabling()
{
	btStartTracing_.EnableWindow(!bIsTracing_);
	btSaveTraceBuffers_.EnableWindow(bIsTracing_);
	btStopTracing_.EnableWindow(bIsTracing_);
	btTracingMode_.EnableWindow(!bIsTracing_);

	btSampledStacks_.EnableWindow(!bIsTracing_);
	btCswitchStacks_.EnableWindow(!bIsTracing_);
}

void CXperfUIDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CXperfUIDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CXperfUIDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


std::string CXperfUIDlg::GetWPTDir()
{
	return "C:\\Program Files (x86)\\Windows Kits\\8.1\\Windows Performance Toolkit\\";
}

std::string CXperfUIDlg::GetXperfPath()
{
	return GetWPTDir() + "xperf.exe";
}

std::string CXperfUIDlg::GetTraceDir()
{
	return traceDir_;
}

std::string CXperfUIDlg::GetExeDir()
{
	char exePath[MAX_PATH];
	if (GetModuleFileName(0, exePath, sizeof(exePath)))
	{
		char* lastSlash = strrchr(exePath, '\\');
		if (lastSlash)
		{
			lastSlash[1] = 0;
			return exePath;
		}
	}

	exit(10);
}

std::string CXperfUIDlg::GetResultFile()
{
	std::string traceDir = GetTraceDir();

	char time[9];
	_strtime_s(time);
	char date[9];
	_strdate_s(date);
	int hour, min, sec;
	int year, month, day;
#pragma warning(suppress : 4996)
	const char* username = getenv("USERNAME");
	if (!username)
		username = "";
	char fileName[MAX_PATH];
	// Hilarious /analyze warning on this line from bug in _strtime_s annotation!
	if (3 == sscanf_s(time, "%d:%d:%d", &hour, &min, &sec) &&
		3 == sscanf_s(date, "%d/%d/%d", &month, &day, &year))
	{
		// The filenames are chosen to sort by date, with username as the LSB.
		sprintf_s(fileName, "%04d-%02d-%02d_%02d-%02d-%02d_%s", year + 2000, month, day, hour, min, sec, username);
	}
	else
	{
		strcpy_s(fileName, "xperfui");
	}

	std::string filePart = fileName;

	if (tracingMode_ == kHeapTracingToFile)
	{
		filePart += "_" + heapTracingExe_.substr(0, heapTracingExe_.size() - 4);
		filePart += "_heap";
	}

	return GetTraceDir() + filePart + ".etl";
}

std::string CXperfUIDlg::GetTempTraceDir()
{
	return tempTraceDir_;
}

std::string CXperfUIDlg::GetKernelFile()
{
	return CXperfUIDlg::GetTempTraceDir() + "kernel.etl";
}

std::string CXperfUIDlg::GetUserFile()
{
	return GetTempTraceDir() + "user.etl";
}

std::string CXperfUIDlg::GetHeapFile()
{
	return GetTempTraceDir() + "heap.etl";
}

void CXperfUIDlg::OnBnClickedStarttracing()
{
	if (tracingMode_ == kTracingToFile)
		outputPrintf("\nStarting tracing to disk...\n");
	else if (tracingMode_ == kTracingToMemory)
		outputPrintf("\nStarting tracing to in-memory circular buffers...\n");
	else if (tracingMode_ == kHeapTracingToFile)
		outputPrintf("\nStarting heap tracing to disk of %s...\n", heapTracingExe_.c_str());
	else
		assert(0);
	ChildProcess child(GetXperfPath());
	std::string kernelProviders = " Latency+POWER+DISPATCHER+FILE_IO+FILE_IO_INIT+VIRT_ALLOC";
	std::string kernelStackWalk = "";
	if (bSampledStacks_ && bCswitchStacks_)
		kernelStackWalk = " -stackwalk PROFILE+CSWITCH+READYTHREAD";
	else if (bSampledStacks_)
		kernelStackWalk = " -stackwalk PROFILE";
	else if (bCswitchStacks_)
		kernelStackWalk = " -stackwalk CSWITCH+READYTHREAD";
	// Buffer sizes are in KB, so 1024 is actually 1 MB
	// Make this configurable.
	std::string kernelBuffers = " -buffersize 1024 -minbuffers 600 -maxbuffers 600";
	std::string kernelFile = " -f \"" + GetKernelFile() + "\"";
	if (tracingMode_ == kTracingToMemory)
		kernelFile = " -buffering";
	std::string kernelArgs = " -on" + kernelProviders + kernelStackWalk + kernelBuffers + kernelFile;

	std::string userProviders = " -on Microsoft-Windows-Win32k+Multi-MAIN+Multi-FrameRate+Multi-Input+Multi-Worker";
	std::string userBuffers = " -buffersize 1024 -minbuffers 100 -maxbuffers 100";
	std::string userFile = " -f \"" + GetUserFile() + "\"";
	if (tracingMode_ == kTracingToMemory)
		userFile = " -buffering";
	std::string userArgs = " -start xperfuiSession" + userProviders + userBuffers + userFile;

	// Heap tracing settings -- only used for heap tracing.
	// Could also record stacks on HeapFree
	std::string heapBuffers = " -buffersize 1024 -minbuffers 200";
	std::string heapFile = " -f \"" + GetHeapFile() + "\"";
	std::string heapStackWalk = " -stackwalk HeapCreate+HeapDestroy+HeapAlloc+HeapRealloc";
	std::string heapArgs = " -start xperfHeapSession -heap -Pids 0" + heapStackWalk + heapBuffers + heapFile;

	if (tracingMode_ == kHeapTracingToFile)
		child.Run(bShowCommands_, "xperf.exe" + kernelArgs + userArgs + heapArgs);
	else
		child.Run(bShowCommands_, "xperf.exe" + kernelArgs + userArgs);

	bIsTracing_ = true;
	UpdateEnabling();
	outputPrintf("Tracing is started.\n");
}

void CXperfUIDlg::StopTracing(bool bSaveTrace)
{
	std::string traceFilename = GetResultFile();
	if (bSaveTrace)
		outputPrintf("\nSaving trace to disk...\n");
	else
		outputPrintf("\nStopping tracing...\n");

	// Rename Amcache.hve to work around a merge hang that can last up to six
	// minutes.
	// https://randomascii.wordpress.com/2015/03/02/profiling-the-profiler-working-around-a-six-minute-xperf-hang/
	const char* const compatFile = "c:\\Windows\\AppCompat\\Programs\\Amcache.hve";
	const char* const compatFileTemp = "c:\\Windows\\AppCompat\\Programs\\Amcache_temp.hve";
	BOOL moveSuccess = MoveFile(compatFile, compatFileTemp);

	{
		// Stop the kernel and user sessions.
		ChildProcess child(GetXperfPath());
		if (bSaveTrace && tracingMode_ == kTracingToMemory)
		{
			// If we are in memory tracing mode then don't actually stop tracing,
			// just flush the buffers to disk.
			std::string args = " -flush \"NT Kernel Logger\" -f \"" + GetKernelFile() + "\" -flush xperfuisession -f \"" + GetUserFile() + "\"";
			child.Run(bShowCommands_, "xperf.exe" + args);
		}
		else
		{
			if (tracingMode_ == kHeapTracingToFile)
				child.Run(bShowCommands_, "xperf.exe -stop xperfHeapSession -stop xperfuiSession -stop");
			else
				child.Run(bShowCommands_, "xperf.exe -stop xperfuiSession -stop");
		}
	}

	if (bSaveTrace)
	{
		outputPrintf("Merging trace...\n");
		{
			// Separate merge step to allow compression on Windows 8+
			// https://randomascii.wordpress.com/2015/03/02/etw-trace-compression-and-xperf-syntax-refresher/
			ChildProcess merge(GetXperfPath());
			std::string args = " -merge \"" + GetKernelFile() + "\" \"" + GetUserFile() + "\"";
			if (tracingMode_ == kHeapTracingToFile)
				args += " \"" + GetHeapFile() + "\"";
			args += " \"" + traceFilename + "\"";
			if (bCompress_)
				args += " -compress";
			merge.Run(bShowCommands_, "xperf.exe" + args);
		}
	}

	if (moveSuccess)
		MoveFile(compatFileTemp, compatFile);

	// Delete the temporary files.
	DeleteFile(GetKernelFile().c_str());
	DeleteFile(GetUserFile().c_str());
	if (tracingMode_ == kHeapTracingToFile)
		DeleteFile(GetHeapFile().c_str());

	if (!bSaveTrace || tracingMode_ != kTracingToMemory)
	{
		bIsTracing_ = false;
		UpdateEnabling();
	}

	if (bSaveTrace)
	{
		// Some private symbols, particularly Chrome's, must be stripped and
		// then converted to .symcache files in order to avoid ~25 minute
		// conversion times for the full private symbols.
		// https://randomascii.wordpress.com/2014/11/04/slow-symbol-loading-in-microsofts-profiler-take-two/
		// Call Python script here, or recreate it in C++.
#pragma warning(suppress:4996)
		const char* path = getenv("path");
		if (path)
		{
			std::vector<std::string> pathParts = split(path, ';');
			for (auto part : pathParts)
			{
				std::string pythonPath = part + '\\' + "python.exe";
				if (PathFileExists(pythonPath.c_str()))
				{
					outputPrintf("Stripping chrome symbols...\n");
					//ChildProcess child("\"" + pythonPath + "\"");
					ChildProcess child(pythonPath);
					std::string args = " \"" + GetExeDir() + "StripChromeSymbols.py\" \"" + traceFilename + "\"";
					child.Run(bShowCommands_, "python.exe" + args);
					break;
				}
			}
		}

		LaunchTraceViewer(traceFilename);
	}
	else
		outputPrintf("Tracing stopped.\n");
}


void CXperfUIDlg::OnBnClickedSavetracebuffers()
{
	StopTracing(true);
}

void CXperfUIDlg::OnBnClickedStoptracing()
{
	StopTracing(false);
}

void CXperfUIDlg::LaunchTraceViewer(const std::string traceFilename)
{
	std::string wpaPath = GetWPTDir() + "wpa.exe";

	const std::string args = std::string("wpa.exe \"") + traceFilename.c_str() + "\"";

	// Wacky CreateProcess rules say args has to be writable!
	std::vector<char> argsCopy(args.size() + 1);
	strcpy_s(&argsCopy[0], argsCopy.size(), args.c_str());
	STARTUPINFO startupInfo = {};
	PROCESS_INFORMATION processInfo = {};
	BOOL result = CreateProcessA(wpaPath.c_str(), &argsCopy[0], nullptr, nullptr, FALSE, 0, nullptr, nullptr, &startupInfo, &processInfo);
	if (result)
	{
		// Close the handles to avoid leaks.
		CloseHandle(processInfo.hProcess);
		CloseHandle(processInfo.hThread);
	}
	else
	{
		AfxMessageBox("Failed to start trace viewer.");
	}
}

void CXperfUIDlg::OnBnClickedCompresstrace()
{
	bCompress_ = !bCompress_;
}


void CXperfUIDlg::OnBnClickedCpusamplingcallstacks()
{
	bSampledStacks_ = !bSampledStacks_;
}


void CXperfUIDlg::OnBnClickedContextswitchcallstacks()
{
	bCswitchStacks_ = !bCswitchStacks_;
}


void CXperfUIDlg::OnBnClickedShowcommands()
{
	bShowCommands_ = !bShowCommands_;
}


void CXperfUIDlg::OnBnClickedFastsampling()
{
	bFastSampling_ = !bFastSampling_;
	const char* message = nullptr;
	if (bFastSampling_)
		message = "Setting CPU sampling speed to 8 KHz, for finer resolution.";
	else
		message = "Setting CPU sampling speed to 1 KHz, for lower overhead.";
	outputPrintf("%s\n", message);
	// Record the CPU sampling frequency change in the trace, if one is being recorded.
	ETWMark(message);
	ChildProcess child(GetXperfPath());
	std::string profInt = bFastSampling_ ? "1221" : "9001";
	std::string args = " -setprofint " + profInt + " cached";
	child.Run(bShowCommands_, "xperf.exe" + args);
}


void CXperfUIDlg::OnCbnSelchangeInputtracing()
{
	InputTracing_ = (KeyLoggerState)btInputTracing_.GetCurSel();
	switch (InputTracing_)
	{
	case kKeyLoggerOff:
		outputPrintf("Key logging disabled.\n");
		break;
	case kKeyLoggerAnonymized:
		outputPrintf("Key logging enabled. Number and letter keys will be recorded generically.\n");
		break;
	case kKeyLoggerFull:
		outputPrintf("Key logging enabled. Full keyboard information recorded - beware of private information being recorded.\n");
		break;
	default:
		assert(0);
		InputTracing_ = kKeyLoggerOff;
		break;
	}
	SetKeyloggingState(InputTracing_);
}

void CXperfUIDlg::UpdateTraceList()
{
	const std::string tracePath = GetTraceDir() + "\\*.etl";

	auto tempTraces = GetFileList(tracePath);
	std::sort(tempTraces.begin(), tempTraces.end());
	// Function to stop the temporary traces from showing up.
	auto ifInvalid = [](const std::string& name) { return name == "kernel.etl" || name == "user.etl" || name == "heap.etl"; };
	tempTraces.erase(std::remove_if(tempTraces.begin(), tempTraces.end(), ifInvalid), tempTraces.end());
	// If nothing has changed, do nothing. This avoids flicker and other ugliness.
	if (tempTraces == traces_)
		return;
	traces_ = tempTraces;

	// Erase all entries and replace them.
	// Todo: retain the current selection index.
	while (btTraces_.GetCount())
		btTraces_.DeleteString(0);
	for (auto name : traces_)
	{
		btTraces_.AddString(name.c_str());
	}
}

LRESULT CXperfUIDlg::UpdateTraceListHandler(WPARAM wParam, LPARAM lParam)
{
	UpdateTraceList();

	return 0;
}


void CXperfUIDlg::OnLbnDblclkTracelist()
{
	int selIndex = btTraces_.GetCurSel();
	CString cStringTraceName;
	btTraces_.GetText(selIndex, cStringTraceName);
	std::string tracename = GetTraceDir() + static_cast<const char*>(cStringTraceName);
	LaunchTraceViewer(tracename);
}

void CXperfUIDlg::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
	if (!initialWidth_)
		return;

	// Don't let the dialog be smaller than its initial size.
	lpMMI->ptMinTrackSize.x = initialWidth_;
	lpMMI->ptMinTrackSize.y = initialHeight_;
}


void CXperfUIDlg::OnSize(UINT nType, int cx, int cy)
{
	if (nType == SIZE_RESTORED && initialWidth_)
	{
		// Calculate xDelta and yDelta -- the change in the window's size.
		CRect windowRect;
		GetWindowRect(&windowRect);
		int xDelta = windowRect.Width() - lastWidth_;
		lastWidth_ += xDelta;
		int yDelta = windowRect.Height() - lastHeight_;
		lastHeight_ += yDelta;

		UINT flags = SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_NOACTIVATE;

		// Resize the trace list and notes control.
		CRect listRect;
		btTraces_.GetWindowRect(&listRect);
		btTraces_.SetWindowPos(nullptr, 0, 0, listRect.Width(), listRect.Height() + yDelta, flags);
		int curSel = btTraces_.GetCurSel();
		if (curSel != LB_ERR)
		{
			// Make the selected line visible.
			btTraces_.SetTopIndex(curSel);
		}

		CRect editRect;
		btTraceNotes_.GetWindowRect(&editRect);
		btTraceNotes_.SetWindowPos(nullptr, 0, 0, editRect.Width() + xDelta, editRect.Height() + yDelta, flags);
	}
}

void CXperfUIDlg::SaveNotesIfNeeded()
{
	// Get the currently selected text, which might have been edited.
	CString editedNotesCString;
	GetDlgItemText(IDC_TRACENOTES, editedNotesCString);
	std::string editedNotes = static_cast<const char*>(editedNotesCString);
	if (editedNotes != traceNotes_)
	{
		if (!traceNoteFilename_.empty())
		{
			WriteTextAsFile(traceNoteFilename_, editedNotes);
		}
	}
}

void CXperfUIDlg::OnLbnSelchangeTracelist()
{
	int curSel = btTraces_.GetCurSel();
	if (curSel >= 0 && curSel < (int)traces_.size())
	{
		SaveNotesIfNeeded();

		btTraceNotes_.EnableWindow(true);
		std::string traceName = traces_[curSel];
		std::string notesFilename = GetTraceDir() + traceName.substr(0, traceName.size() - 4) + ".txt";
		std::string notes = LoadFileAsText(notesFilename);
		SetDlgItemText(IDC_TRACENOTES, notes.c_str());
		traceNotes_ = notes;
		traceNoteFilename_ = notesFilename;
	}
}


void CXperfUIDlg::OnBnClickedAbout()
{
	CAboutDlg dlgAbout;
	dlgAbout.DoModal();
}

LRESULT CXperfUIDlg::OnHotKey(WPARAM wParam, LPARAM lParam)
{
	switch (wParam)
	{
	case kRecordTraceHotKey:
		StopTracing(true);
		break;
	}

	return 0;
}


// Magic sauce to make tooltips work.
BOOL CXperfUIDlg::PreTranslateMessage(MSG* pMsg)
{
	toolTip_.RelayEvent(pMsg);

	return CDialog::PreTranslateMessage(pMsg);
}


void CXperfUIDlg::SetHeapTracing(bool forceOff)
{
	std::string targetKey = "Software\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options";
	DWORD tracingFlags = tracingMode_ == kHeapTracingToFile ? 1 : 0;
	if (forceOff)
		tracingFlags = 0;
	CreateRegistryKey(HKEY_LOCAL_MACHINE, targetKey, heapTracingExe_);
	targetKey += "\\" + heapTracingExe_;
	SetRegistryDWORD(HKEY_LOCAL_MACHINE, targetKey, "TracingFlags", tracingFlags);
}

void CXperfUIDlg::OnCbnSelchangeTracingmode()
{
	tracingMode_ = (TracingMode)btTracingMode_.GetCurSel();
	switch (tracingMode_)
	{
	case kTracingToFile:
		outputPrintf("Traces will be recorded to disk to allow arbitrarily long recordings.\n");
		break;
	case kTracingToMemory:
		outputPrintf("Traces will be recorded to in-memory circular buffers. Tracing can be enabled "
			"indefinitely long, and will record the last ~10-60 seconds.\n");
		break;
	case kHeapTracingToFile:
		outputPrintf("Heap traces will be recorded to disk for %s. Note that only %s processes "
			"started after this is selected will be traced. Note that %s processes started now "
			"will run slightly slower even if not being traced.\n", heapTracingExe_.c_str(),
			heapTracingExe_.c_str(), heapTracingExe_.c_str());
		break;
	}
	SetHeapTracing(false);
}
