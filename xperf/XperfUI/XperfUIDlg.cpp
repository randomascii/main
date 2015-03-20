#include "stdafx.h"
#include "XperfUI.h"
#include "XperfUIDlg.h"

#include "About.h"
#include "afxdialogex.h"
#include "ChildProcess.h"
#include "Settings.h"
#include "Utility.h"

#include <algorithm>
#include <direct.h>
#include <ETWProviders\etwprof.h>
#include <vector>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// Send this when the list of traces needs to be updated.
const int WM_UPDATETRACELIST = WM_USER + 10;

const int kRecordTraceHotKey = 1234;


// CXperfUIDlg dialog


static CXperfUIDlg* pMainWindow;

void outputPrintf(_Printf_format_string_ const wchar_t* pFormat, ...)
{
	va_list args;
	va_start(args, pFormat);
	pMainWindow->vprintf(pFormat, args);
	va_end(args);
}

void CXperfUIDlg::vprintf(const wchar_t* pFormat, va_list args)
{
	wchar_t buffer[5000];
	_vsnwprintf_s(buffer, _TRUNCATE, pFormat, args);

	for (const wchar_t* pBuf = buffer; *pBuf; ++pBuf)
	{
		// Need \r\n as a line separator.
		if (pBuf[0] == '\n')
		{
			// Don't add a line separator at the very beginning.
			if (!output_.empty())
				output_ += L"\r\n";
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

	// Fake out the Windows hang detection since otherwise on long-running
	// child-processes such as processing Chrome symbols we will get
	// frosted, a ghost window will be displayed, and none of our updates
	// will be visible.
	MSG msg;
	PeekMessage(&msg, *this, 0, 0, PM_NOREMOVE);
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
	ON_BN_CLICKED(IDC_SETTINGS, &CXperfUIDlg::OnBnClickedSettings)
	ON_WM_CONTEXTMENU()
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
	const wchar_t* traceDir = reinterpret_cast<const wchar_t*>(voidTraceDir);

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
		AfxMessageBox((GetXperfPath() + L" does not exist. Please install WPT 8.1. Exiting.").c_str());
		exit(10);
	}

	wchar_t documents[MAX_PATH];
	if (!SHGetSpecialFolderPath(*this, documents, CSIDL_MYDOCUMENTS, TRUE))
	{
		assert(!"Failed to find My Documents directory.\n");
		exit(10);
	}
	std::wstring defaultTraceDir = documents + std::wstring(L"\\xperftraces\\");
	traceDir_ = GetDirectory(L"xperftracedir", defaultTraceDir);

	tempTraceDir_ = GetDirectory(L"temp", traceDir_);

	SetSymbolPath();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	CheckDlgButton(IDC_COMPRESSTRACE, bCompress_);
	CheckDlgButton(IDC_CPUSAMPLINGCALLSTACKS, bSampledStacks_);
	CheckDlgButton(IDC_CONTEXTSWITCHCALLSTACKS, bCswitchStacks_);



	btInputTracing_.AddString(L"Off");
	btInputTracing_.AddString(L"Private");
	btInputTracing_.AddString(L"Full");
	btInputTracing_.SetCurSel(InputTracing_);

	btTracingMode_.AddString(L"Tracing to file");
	btTracingMode_.AddString(L"Circular buffer tracing");
	btTracingMode_.AddString(L"Heap tracing to file");
	btTracingMode_.SetCurSel(tracingMode_);

	UpdateEnabling();
	btTraceNotes_.EnableWindow(false); // This window always starts out disabled.

	// Don't change traceDir_ - the DirectoryMonitorThread has a pointer to it.
	(void)CreateThread(nullptr, 0, DirectoryMonitorThread, const_cast<wchar_t*>(traceDir_.c_str()), 0, 0);

	RegisterProviders();
	DisablePagingExecutive();

	UpdateTraceList();

	if (toolTip_.Create(this))
	{
		toolTip_.AddTool(&btStartTracing_, L"Start ETW tracing.");

		toolTip_.AddTool(&btCompress_, L"Only uncheck this if you record traces on Windows 8 and above and want to analyze "
					L"them on Windows 7 and below.\n"
					L"Enable ETW trace compression. On Windows 8 and above this compresses traces "
					L"as they are saved, making them 5-10x smaller. However compressed traces cannot be loaded on "
					L"Windows 7 or earlier. On Windows 7 this setting has no effect.");
		toolTip_.AddTool(&btCswitchStacks_, L"This enables recording of call stacks on context switches, from both "
					L"the thread being switched in and the readying thread. This should only be disabled if the performance "
					L"of functions like WaitForSingleObject and SetEvent appears to be distorted, which can happen when the "
					L"context-switch rate is very high.");
		toolTip_.AddTool(&btSampledStacks_, L"This enables recording of call stacks on CPU sampling events, which "
					L"by default happen at 1 KHz. This should rarely be disabled.");
		toolTip_.AddTool(&btFastSampling_, L"Checking this changes the CPU sampling frequency from the default of "
					L"~1 KHz to the maximum speed of ~8 KHz. This increases the data rate and thus the size of traces "
					L"but can make investigating brief CPU-bound performance problems (such as a single long frame) "
					L"more practical.");
		toolTip_.AddTool(&btShowCommands_, L"This tells XperfUI to display the xperf.exe and other commands being "
					L"executed. This can be helpful for diagnostic purposes but is not normally needed.");

		const TCHAR* pInputTip = L"Input tracing inserts custom ETW events into traces which can be helpful when "
					L"investigating performance problems that are correlated with user input. The default setting of "
					L"'private' records alphabetic keys as 'A' and numeric keys as '0'. The 'full' setting records "
					L"alphanumeric details. Both 'private' and 'full' record mouse movement and button clicks. The "
					L"'off' setting records no input.";
		toolTip_.AddTool(&btInputTracingLabel_, pInputTip);
		toolTip_.AddTool(&btInputTracing_, pInputTip);

		toolTip_.AddTool(&btTracingMode_, L"Select whether to trace straight to disk or to in-memory circular buffers.");

		toolTip_.AddTool(&btTraces_, L"This is a list of all traces found in %xperftracedir%, which defaults to "
					L"documents\\xperftraces.");
		toolTip_.AddTool(&btTraceNotes_, L"Trace notes are intended for recording information about ETW traces, such "
					L"as an analysis of what was discovered in the trace. Trace notes are auto-saved to a parallel text "
					L"file - just type your analysis.");

		toolTip_.SetMaxTipWidth(400);
		toolTip_.Activate(TRUE);
	}

	SetHeapTracing(false);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

std::wstring CXperfUIDlg::GetDirectory(const wchar_t* env, const std::wstring& default)
{
	// Get a directory (from an environment variable, if set) and make sure it exists.
	std::wstring result = default;
#pragma warning(suppress : 4996)
	const wchar_t* traceDir = _wgetenv(env);
	if (traceDir)
	{
		result = traceDir;
	}
	// Make sure the name ends with a backslash.
	if (!result.empty() && result[result.size() - 1] != '\\')
		result += '\\';
	if (!PathFileExists(result.c_str()))
	{
		(void)_wmkdir(result.c_str());
	}
	if (!PathIsDirectory(result.c_str()))
	{
		AfxMessageBox((result + L" is not a directory. Exiting.").c_str());
		exit(10);
	}
	return result;
}

void CXperfUIDlg::RegisterProviders()
{
	std::wstring dllSource = GetExeDir() + L"ETWProviders.dll";
#pragma warning(suppress:4996)
	const wchar_t* temp = _wgetenv(L"temp");
	if (!temp)
		return;
	std::wstring dllDest = temp;
	dllDest += L"\\ETWProviders.dll";
	if (!CopyFile(dllSource.c_str(), dllDest.c_str(), FALSE))
	{
		outputPrintf(L"Registering of ETW providers failed due to copy error.\n");
		return;
	}
	wchar_t systemDir[MAX_PATH];
	systemDir[0] = 0;
	GetSystemDirectory(systemDir, ARRAYSIZE(systemDir));
	std::wstring wevtPath = systemDir + std::wstring(L"\\wevtutil.exe");

	for (int pass = 0; pass < 2; ++pass)
	{
		ChildProcess child(wevtPath);
		std::wstring args = pass ? L" im" : L" um";
		args += L" \"" + GetExeDir() + L"etwproviders.man\"";
		child.Run(bShowCommands_, L"wevtutil.exe" + args);
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
		const wchar_t* keyName = L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management";
		SetRegistryDWORD(HKEY_LOCAL_MACHINE, keyName, L"DisablePagingExecutive", 1);
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


std::wstring CXperfUIDlg::GetWPTDir() const
{
	return L"C:\\Program Files (x86)\\Windows Kits\\8.1\\Windows Performance Toolkit\\";
}

std::wstring CXperfUIDlg::GetXperfPath() const
{
	return GetWPTDir() + L"xperf.exe";
}

std::wstring CXperfUIDlg::GetTraceDir() const
{
	return traceDir_;
}

std::wstring CXperfUIDlg::GetExeDir() const
{
	wchar_t exePath[MAX_PATH];
	if (GetModuleFileName(0, exePath, ARRAYSIZE(exePath)))
	{
		wchar_t* lastSlash = wcsrchr(exePath, '\\');
		if (lastSlash)
		{
			lastSlash[1] = 0;
			return exePath;
		}
	}

	exit(10);
}

std::wstring CXperfUIDlg::GetResultFile() const
{
	std::wstring traceDir = GetTraceDir();

	char time[9];
	_strtime_s(time);
	char date[9];
	_strdate_s(date);
	int hour, min, sec;
	int year, month, day;
#pragma warning(suppress : 4996)
	const wchar_t* username = _wgetenv(L"USERNAME");
	if (!username)
		username = L"";
	wchar_t fileName[MAX_PATH];
	// Hilarious /analyze warning on this line from bug in _strtime_s annotation!
	// warning C6054: String 'time' might not be zero-terminated.
#pragma warning(suppress : 6054)
	if (3 == sscanf_s(time, "%d:%d:%d", &hour, &min, &sec) &&
		3 == sscanf_s(date, "%d/%d/%d", &month, &day, &year))
	{
		// The filenames are chosen to sort by date, with username as the LSB.
		swprintf_s(fileName, L"%04d-%02d-%02d_%02d-%02d-%02d_%s", year + 2000, month, day, hour, min, sec, username);
	}
	else
	{
		wcscpy_s(fileName, L"xperfui");
	}

	std::wstring filePart = fileName;

	if (tracingMode_ == kHeapTracingToFile)
	{
		filePart += L"_" + heapTracingExe_.substr(0, heapTracingExe_.size() - 4);
		filePart += L"_heap";
	}

	return GetTraceDir() + filePart + L".etl";
}

std::wstring CXperfUIDlg::GetTempTraceDir() const
{
	return tempTraceDir_;
}

std::wstring CXperfUIDlg::GetKernelFile() const
{
	return CXperfUIDlg::GetTempTraceDir() + L"kernel.etl";
}

std::wstring CXperfUIDlg::GetUserFile() const
{
	return GetTempTraceDir() + L"user.etl";
}

std::wstring CXperfUIDlg::GetHeapFile() const
{
	return GetTempTraceDir() + L"heap.etl";
}

void CXperfUIDlg::OnBnClickedStarttracing()
{
	if (tracingMode_ == kTracingToFile)
		outputPrintf(L"\nStarting tracing to disk...\n");
	else if (tracingMode_ == kTracingToMemory)
		outputPrintf(L"\nStarting tracing to in-memory circular buffers...\n");
	else if (tracingMode_ == kHeapTracingToFile)
		outputPrintf(L"\nStarting heap tracing to disk of %s...\n", heapTracingExe_.c_str());
	else
		assert(0);
	ChildProcess child(GetXperfPath());
	std::wstring kernelProviders = L" Latency+POWER+DISPATCHER+FILE_IO+FILE_IO_INIT+VIRT_ALLOC";
	std::wstring kernelStackWalk = L"";
	if (bSampledStacks_ && bCswitchStacks_)
		kernelStackWalk = L" -stackwalk PROFILE+CSWITCH+READYTHREAD";
	else if (bSampledStacks_)
		kernelStackWalk = L" -stackwalk PROFILE";
	else if (bCswitchStacks_)
		kernelStackWalk = L" -stackwalk CSWITCH+READYTHREAD";
	// Buffer sizes are in KB, so 1024 is actually 1 MB
	// Make this configurable.
	std::wstring kernelBuffers = L" -buffersize 1024 -minbuffers 600 -maxbuffers 600";
	std::wstring kernelFile = L" -f \"" + GetKernelFile() + L"\"";
	if (tracingMode_ == kTracingToMemory)
		kernelFile = L" -buffering";
	std::wstring kernelArgs = L" -start " + GetKernelLogger() + L" -on" + kernelProviders + kernelStackWalk + kernelBuffers + kernelFile;

	std::wstring userProviders = L" -on Microsoft-Windows-Win32k+Multi-MAIN+Multi-FrameRate+Multi-Input+Multi-Worker";
	std::wstring userBuffers = L" -buffersize 1024 -minbuffers 100 -maxbuffers 100";
	std::wstring userFile = L" -f \"" + GetUserFile() + L"\"";
	if (tracingMode_ == kTracingToMemory)
		userFile = L" -buffering";
	std::wstring userArgs = L" -start xperfuiSession" + userProviders + userBuffers + userFile;

	// Heap tracing settings -- only used for heap tracing.
	// Could also record stacks on HeapFree
	std::wstring heapBuffers = L" -buffersize 1024 -minbuffers 200";
	std::wstring heapFile = L" -f \"" + GetHeapFile() + L"\"";
	std::wstring heapStackWalk = L" -stackwalk HeapCreate+HeapDestroy+HeapAlloc+HeapRealloc";
	std::wstring heapArgs = L" -start xperfHeapSession -heap -Pids 0" + heapStackWalk + heapBuffers + heapFile;

	if (tracingMode_ == kHeapTracingToFile)
		child.Run(bShowCommands_, L"xperf.exe" + kernelArgs + userArgs + heapArgs);
	else
		child.Run(bShowCommands_, L"xperf.exe" + kernelArgs + userArgs);

	bIsTracing_ = true;
	UpdateEnabling();
	outputPrintf(L"Tracing is started.\n");
}

void CXperfUIDlg::StopTracing(bool bSaveTrace)
{
	std::wstring traceFilename = GetResultFile();
	if (bSaveTrace)
		outputPrintf(L"\nSaving trace to disk...\n");
	else
		outputPrintf(L"\nStopping tracing...\n");

	// Rename Amcache.hve to work around a merge hang that can last up to six
	// minutes.
	// https://randomascii.wordpress.com/2015/03/02/profiling-the-profiler-working-around-a-six-minute-xperf-hang/
	const wchar_t* const compatFile = L"c:\\Windows\\AppCompat\\Programs\\Amcache.hve";
	const wchar_t* const compatFileTemp = L"c:\\Windows\\AppCompat\\Programs\\Amcache_temp.hve";
	BOOL moveSuccess = MoveFile(compatFile, compatFileTemp);

	{
		// Stop the kernel and user sessions.
		ChildProcess child(GetXperfPath());
		if (bSaveTrace && tracingMode_ == kTracingToMemory)
		{
			// If we are in memory tracing mode then don't actually stop tracing,
			// just flush the buffers to disk.
			std::wstring args = L" -flush " + GetKernelLogger() + L" -f \"" + GetKernelFile() + L"\" -flush xperfuisession -f \"" + GetUserFile() + L"\"";
			child.Run(bShowCommands_, L"xperf.exe" + args);
		}
		else
		{
			if (tracingMode_ == kHeapTracingToFile)
				child.Run(bShowCommands_, L"xperf.exe -stop xperfHeapSession -stop xperfuiSession -stop " + GetKernelLogger());
			else
				child.Run(bShowCommands_, L"xperf.exe -stop xperfuiSession -stop " + GetKernelLogger());
		}
	}

	if (bSaveTrace)
	{
		outputPrintf(L"Merging trace...\n");
		{
			// Separate merge step to allow compression on Windows 8+
			// https://randomascii.wordpress.com/2015/03/02/etw-trace-compression-and-xperf-syntax-refresher/
			ChildProcess merge(GetXperfPath());
			std::wstring args = L" -merge \"" + GetKernelFile() + L"\" \"" + GetUserFile() + L"\"";
			if (tracingMode_ == kHeapTracingToFile)
				args += L" \"" + GetHeapFile() + L"\"";
			args += L" \"" + traceFilename + L"\"";
			if (bCompress_)
				args += L" -compress";
			merge.Run(bShowCommands_, L"xperf.exe" + args);
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
		const wchar_t* path = _wgetenv(L"path");
		if (path)
		{
			std::vector<std::wstring> pathParts = split(path, ';');
			for (auto part : pathParts)
			{
				std::wstring pythonPath = part + L"\\python.exe";
				if (PathFileExists(pythonPath.c_str()))
				{
					outputPrintf(L"Stripping chrome symbols - this may take a while...\n");
					ChildProcess child(pythonPath);
					// Must pass -u to disable Python's output buffering when printing to
					// a pipe, in order to get timely feedback.
					std::wstring args = L" -u \"" + GetExeDir() + L"StripChromeSymbols.py\" \"" + traceFilename + L"\"";
					child.Run(bShowCommands_, L"python.exe" + args);
					break;
				}
			}
		}

		LaunchTraceViewer(traceFilename);
	}
	else
		outputPrintf(L"Tracing stopped.\n");
}


void CXperfUIDlg::OnBnClickedSavetracebuffers()
{
	StopTracing(true);
}

void CXperfUIDlg::OnBnClickedStoptracing()
{
	StopTracing(false);
}

void CXperfUIDlg::LaunchTraceViewer(const std::wstring traceFilename)
{
	std::wstring wpaPath = GetWPTDir() + L"wpa.exe";

	const std::wstring args = std::wstring(L"wpa.exe \"") + traceFilename.c_str() + L"\"";

	// Wacky CreateProcess rules say args has to be writable!
	std::vector<wchar_t> argsCopy(args.size() + 1);
	wcscpy_s(&argsCopy[0], argsCopy.size(), args.c_str());
	STARTUPINFO startupInfo = {};
	PROCESS_INFORMATION processInfo = {};
	BOOL result = CreateProcess(wpaPath.c_str(), &argsCopy[0], nullptr, nullptr, FALSE, 0, nullptr, nullptr, &startupInfo, &processInfo);
	if (result)
	{
		// Close the handles to avoid leaks.
		CloseHandle(processInfo.hProcess);
		CloseHandle(processInfo.hThread);
	}
	else
	{
		AfxMessageBox(L"Failed to start trace viewer.");
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
	const wchar_t* message = nullptr;
	const char* narrowMessage = nullptr;
	if (bFastSampling_)
	{
		message = L"Setting CPU sampling speed to 8 KHz, for finer resolution.";
		narrowMessage = "Setting CPU sampling speed to 8 KHz, for finer resolution.";
	}
	else
	{
		message = L"Setting CPU sampling speed to 1 KHz, for lower overhead.";
		narrowMessage = "Setting CPU sampling speed to 1 KHz, for lower overhead.";
	}
	outputPrintf(L"%s\n", message);
	// Record the CPU sampling frequency change in the trace, if one is being recorded.
	ETWMark(narrowMessage);
	ChildProcess child(GetXperfPath());
	std::wstring profInt = bFastSampling_ ? L"1221" : L"9001";
	std::wstring args = L" -setprofint " + profInt + L" cached";
	child.Run(bShowCommands_, L"xperf.exe" + args);
}


void CXperfUIDlg::OnCbnSelchangeInputtracing()
{
	InputTracing_ = (KeyLoggerState)btInputTracing_.GetCurSel();
	switch (InputTracing_)
	{
	case kKeyLoggerOff:
		outputPrintf(L"Key logging disabled.\n");
		break;
	case kKeyLoggerAnonymized:
		outputPrintf(L"Key logging enabled. Number and letter keys will be recorded generically.\n");
		break;
	case kKeyLoggerFull:
		outputPrintf(L"Key logging enabled. Full keyboard information recorded - beware of private information being recorded.\n");
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
	const std::wstring tracePath = GetTraceDir() + L"\\*.etl";

	auto tempTraces = GetFileList(tracePath);
	std::sort(tempTraces.begin(), tempTraces.end());
	// Function to stop the temporary traces from showing up.
	auto ifInvalid = [](const std::wstring& name) { return name == L"kernel.etl" || name == L"user.etl" || name == L"heap.etl"; };
	tempTraces.erase(std::remove_if(tempTraces.begin(), tempTraces.end(), ifInvalid), tempTraces.end());
	// If nothing has changed, do nothing. This avoids flicker and other ugliness.
	if (tempTraces == traces_)
		return;
	traces_ = tempTraces;

	// Avoid flicker by disabling redraws until the list has been rebuilt.
	btTraces_.SetRedraw(FALSE);
	// Erase all entries and replace them.
	// Todo: retain the current selection index.
	while (btTraces_.GetCount())
		btTraces_.DeleteString(0);
	for (auto name : traces_)
	{
		btTraces_.AddString(name.c_str());
	}
	btTraces_.SetRedraw(TRUE);
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
	std::wstring tracename = GetTraceDir() + GetListControlText(*this, IDC_TRACELIST, selIndex);
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
	std::wstring editedNotes = GetEditControlText(*this, IDC_TRACENOTES);
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
		std::wstring traceName = traces_[curSel];
		std::wstring notesFilename = GetTraceDir() + traceName.substr(0, traceName.size() - 4) + L".txt";
		std::wstring notes = LoadFileAsText(notesFilename);
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
	std::wstring targetKey = L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options";
	DWORD tracingFlags = tracingMode_ == kHeapTracingToFile ? 1 : 0;
	if (forceOff)
		tracingFlags = 0;
	CreateRegistryKey(HKEY_LOCAL_MACHINE, targetKey, heapTracingExe_);
	targetKey += L"\\" + heapTracingExe_;
	SetRegistryDWORD(HKEY_LOCAL_MACHINE, targetKey, L"TracingFlags", tracingFlags);
}

void CXperfUIDlg::OnCbnSelchangeTracingmode()
{
	tracingMode_ = (TracingMode)btTracingMode_.GetCurSel();
	switch (tracingMode_)
	{
	case kTracingToFile:
		outputPrintf(L"Traces will be recorded to disk to allow arbitrarily long recordings.\n");
		break;
	case kTracingToMemory:
		outputPrintf(L"Traces will be recorded to in-memory circular buffers. Tracing can be enabled "
				L"indefinitely long, and will record the last ~10-60 seconds.\n");
		break;
	case kHeapTracingToFile:
		outputPrintf(L"Heap traces will be recorded to disk for %s. Note that only %s processes "
			L"started after this is selected will be traced. Note that %s processes started now "
			L"may run slightly slower even if not being traced.\n"
			L"To keep trace sizes manageable you may want to turn off context switch and CPU "
			L"sampling call stacks.\n", heapTracingExe_.c_str(),
			heapTracingExe_.c_str(), heapTracingExe_.c_str());
		break;
	}
	SetHeapTracing(false);
}


void CXperfUIDlg::OnBnClickedSettings()
{
	CSettings dlgAbout;
	dlgAbout.heapTracingExe_ = heapTracingExe_;
	if (dlgAbout.DoModal() == IDOK)
	{
		heapTracingExe_ = dlgAbout.heapTracingExe_;
	}
}

void CXperfUIDlg::OnContextMenu(CWnd* pWnd, CPoint point)
{
	// See if we right-clicked on the trace list.
	if (pWnd == &btTraces_)
	{
		int selIndex = btTraces_.GetCurSel();

		CMenu PopupMenu;
		PopupMenu.LoadMenu(IDR_TRACESCONTEXTMENU);

		CMenu *pContextMenu = PopupMenu.GetSubMenu(0);

		std::wstring tracePath;
		if (selIndex >= 0)
		{
			pContextMenu->SetDefaultItem(ID_TRACES_OPENTRACEINWPA);
			tracePath = GetTraceDir() + GetListControlText(*this, IDC_TRACELIST, selIndex);;
		}
		else
		{
			// List of menu items that are disabled when no trace is selected.
			// Those that are always available are commented out in this list.
			int disableList[] =
			{
				ID_TRACES_OPENTRACEINWPA,
				ID_TRACES_DELETETRACE,
				ID_TRACES_COMPRESSTRACE,
				//ID_TRACES_COMPRESSTRACES,
				//ID_TRACES_BROWSEFOLDER,
				ID_TRACES_STRIPCHROMESYMBOLS,
				ID_TRACES_TRACEPATHTOCLIPBOARD,
			};

			for (auto id : disableList)
				pContextMenu->EnableMenuItem(id, MF_BYCOMMAND | MF_GRAYED);
		}

		int selection = pContextMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON |
			TPM_RIGHTBUTTON | TPM_RETURNCMD | TPM_NONOTIFY,
			point.x, point.y, pWnd, NULL);

		switch (selection)
		{
			case ID_TRACES_OPENTRACEINWPA:
				LaunchTraceViewer(tracePath);
				break;
			case ID_TRACES_DELETETRACE:
				if (AfxMessageBox(L"Are you sure you want to delete this trace?", MB_YESNO) == IDYES)
				{
					if (DeleteOneFile(*this, tracePath))
					{
						outputPrintf(L"\nFile deletion failed.\n");
					}
				}
				break;
			case ID_TRACES_COMPRESSTRACE:
				CompressTrace(tracePath);
				break;
			case ID_TRACES_COMPRESSTRACES:
				for (auto traceName : traces_)
				{
					CompressTrace(GetTraceDir() + traceName);
				}
				break;
			case ID_TRACES_BROWSEFOLDER:
				ShellExecute(NULL, L"open", GetTraceDir().c_str(), NULL, GetTraceDir().c_str(), SW_SHOW);
				break;
			case ID_TRACES_STRIPCHROMESYMBOLS:
				AfxMessageBox(L"Not implemented.");
				break;
			case ID_TRACES_TRACEPATHTOCLIPBOARD:
				SetClipboardText(tracePath);
				break;
		}
	}
	else
	{
		CDialog::OnContextMenu(pWnd, point);
	}
}

void CXperfUIDlg::CompressTrace(const std::wstring& tracePath)
{
	std::wstring compressedPath = tracePath + L".compressed";
	DWORD exitCode = 0;
	{
		ChildProcess child(GetXperfPath());
		std::wstring args = L" -merge \"" + tracePath + L"\" \"" + compressedPath + L"\" -compress";
		child.Run(bShowCommands_, L"xperf.exe" + args);
		child.WaitForCompletion();
		exitCode = child.GetExitCode();
	}

	if (exitCode)
	{
		DeleteOneFile(*this, compressedPath);
		return;
	}

	int64_t originalSize = GetFileSize(tracePath);
	int64_t compressedSize = GetFileSize(compressedPath);
	outputPrintf(L"File sizes are %lld and %lld.\n", originalSize, compressedSize);
	// Require a minimum of 1% compression
	if (compressedSize > 0 && compressedSize < (originalSize - originalSize / 100))
	{
		DeleteOneFile(*this, tracePath);
		MoveFile(compressedPath.c_str(), tracePath.c_str());
	}
	else
	{
		DeleteOneFile(*this, compressedPath);
	}
}
