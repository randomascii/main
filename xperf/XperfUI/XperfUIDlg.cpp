
// XperfUIDlg.cpp : implementation file
//

#include "stdafx.h"
#include "XperfUI.h"
#include "XperfUIDlg.h"
#include "afxdialogex.h"
#include "ChildProcess.h"
#include <direct.h>
#include <vector>
#include <ETWProviders\etwprof.h>
#include "KeyLoggerThread.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


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
	CEdit* pOutput = (CEdit*)GetDlgItem(IDC_OUTPUT);
	pOutput->SetSel(0, -1);
	pOutput->SetSel(-1, -1);

	// Display the results immediately.
	UpdateWindow();
}

CXperfUIDlg::CXperfUIDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CXperfUIDlg::IDD, pParent)
{
	pMainWindow = this;
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
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
	ON_BN_CLICKED(IDC_LOGINPUT, &CXperfUIDlg::OnBnClickedLoginput)
END_MESSAGE_MAP()


// CXperfUIDlg message handlers

BOOL CXperfUIDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
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

	// Select the trace directory and make sure it exists.
#pragma warning(suppress : 4996)
	const char* traceDir = getenv("tracedir");
	if (traceDir)
	{
		traceDir_ = traceDir;
		if (!traceDir_.empty() && traceDir_[traceDir_.size() - 1] != '\\')
			traceDir_ += '\\';
	}
	else
	{
		char documents[MAX_PATH];
		if (!SHGetSpecialFolderPath(*this, documents, CSIDL_MYDOCUMENTS, TRUE))
		{
			assert(!"Failed to find My Documents directory.\n");
			exit(10);
		}
		traceDir_ = documents + std::string("\\xperftraces\\");
	}
	if (!PathFileExists(GetTraceDir().c_str()))
	{
		(void)_mkdir(GetTraceDir().c_str());
	}
	if (!PathIsDirectory(GetTraceDir().c_str()))
	{
		AfxMessageBox((GetTraceDir() + " is not a directory. Exiting.").c_str());
		exit(10);
	}

#pragma warning(suppress : 4996)
	const char* tempTraceDir = getenv("temptracedir");
	if (tempTraceDir)
	{
		tempTraceDir_ = tempTraceDir;
		if (!tempTraceDir_.empty() && tempTraceDir_[tempTraceDir_.size() - 1] != '\\')
			tempTraceDir_ += '\\';
		if (!PathFileExists(GetTempTraceDir().c_str()))
		{
			(void)_mkdir(GetTempTraceDir().c_str());
		}
		if (!PathIsDirectory(GetTempTraceDir().c_str()))
		{
			AfxMessageBox((GetTempTraceDir() + " is not a directory. Exiting.").c_str());
			exit(10);
		}
	}
	else
	{
		tempTraceDir_ = traceDir_;
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	CheckDlgButton(IDC_COMPRESSTRACE, bCompress_);
	CheckDlgButton(IDC_CPUSAMPLINGCALLSTACKS, bSampledStacks_);
	CheckDlgButton(IDC_CONTEXTSWITCHCALLSTACKS, bCswitchStacks_);

	UpdateEnabling();

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CXperfUIDlg::UpdateEnabling()
{
	btStartTracing_.EnableWindow(!bIsTracing_);
	btSaveTraceBuffers_.EnableWindow(FALSE);
	btStopTracing_.EnableWindow(bIsTracing_);

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
		sprintf_s(fileName, "%s_%04d-%02d-%02d_%02d-%02d-%02d.etl", username, year + 2000, month, day, hour, min, sec);
	}
	else
	{
		strcpy_s(fileName, "xperfui.etl");
	}
	return GetTraceDir() + fileName;
}

std::string CXperfUIDlg::GetTempTraceDir()
{
	return tempTraceDir_;
}

std::string CXperfUIDlg::GetKernelFile()
{
	return CXperfUIDlg::GetTraceDir() + "kernel.etl";
}

std::string CXperfUIDlg::GetUserFile()
{
	return GetTempTraceDir() + "user.etl";
}

void CXperfUIDlg::OnBnClickedStarttracing()
{
	outputPrintf("\nStarting tracing...\n");
	ChildProcess child(GetXperfPath());
	std::string kernelProviders = " Latency+POWER+DISPATCHER+FILE_IO+FILE_IO_INIT+VIRT_ALLOC";
	std::string kernelStackWalk = "";
	if (bSampledStacks_ && bCswitchStacks_)
		kernelStackWalk = " -stackwalk PROFILE+CSWITCH+READYTHREAD";
	else if (bSampledStacks_)
		kernelStackWalk = " -stackwalk PROFILE";
	else if (bCswitchStacks_)
		kernelStackWalk = " -stackwalk CSWITCH+READYTHREAD";
	// Make this configurable.
	std::string kernelBuffers = " -buffersize 1024 -minbuffers 1200";
	std::string kernelFile = " -f " + GetKernelFile();
	std::string kernelArgs = " -on" + kernelProviders + kernelStackWalk + kernelBuffers + kernelFile;

	std::string userProviders = " -on Microsoft-Windows-Win32k+Multi-MAIN+Multi-FrameRate+Multi-Input+Multi-Worker";
	std::string userFile = " -f " + GetUserFile();
	std::string userArgs = " -start xperfuiSession" + userProviders + userFile;

	std::string args = "xperf.exe" + kernelArgs + userArgs;

	child.Run(bShowCommands_, args);

	bIsTracing_ = true;
	UpdateEnabling();
	outputPrintf("Tracing is started.\n");
}

void CXperfUIDlg::OnBnClickedStoptracing()
{
	std::string traceFilename = GetResultFile();
	outputPrintf("Saving trace to disk.\n");
	{
		ChildProcess child(GetXperfPath());
		child.Run(bShowCommands_, "xperf.exe -stop xperfuiSession -stop");
	}
	// Rename Amcache.hve to work around a merge hang that can last up to six
	// minutes.
	// https://randomascii.wordpress.com/2015/03/02/profiling-the-profiler-working-around-a-six-minute-xperf-hang/
	const char* const compatFile = "c:\\Windows\\AppCompat\\Programs\\Amcache.hve";
	const char* const compatFileTemp = "c:\\Windows\\AppCompat\\Programs\\Amcache_temp.hve";
	BOOL moveSuccess = MoveFile(compatFile, compatFileTemp);
	{
		// Separate merge step to allow compression on Windows 8+
		// https://randomascii.wordpress.com/2015/03/02/etw-trace-compression-and-xperf-syntax-refresher/
		ChildProcess merge(GetXperfPath());
		std::string args = " -merge " + GetKernelFile() + " " + GetUserFile() + " " + traceFilename;
		if (bCompress_)
			args += " -compress";
		merge.Run(bShowCommands_, "xperf.exe" + args);
	}
	if (moveSuccess)
		MoveFile(compatFileTemp, compatFile);

	// Delete the temporary files.
	DeleteFile(GetKernelFile().c_str());
	DeleteFile(GetUserFile().c_str());

	bIsTracing_ = false;
	UpdateEnabling();

	// Some private symbols, particularly Chrome's, must be stripped and
	// then converted to .symcache files in order to avoid ~25 minute
	// conversion times for the full private symbols.
	// https://randomascii.wordpress.com/2014/11/04/slow-symbol-loading-in-microsofts-profiler-take-two/
	// Call Python script here, or recreate it in C++.
	// python StripChromeSymbols.py %FileName%

	LaunchTraceViewer(traceFilename);
}

void CXperfUIDlg::LaunchTraceViewer(const std::string traceFilename)
{
	// Before launching trace analysis make sure that the symbol paths are set.

#pragma warning(suppress : 4996)
	const char* symPath = getenv("_NT_SYMBOL_PATH");
	if (!symPath)
		(void)_putenv("_NT_SYMBOL_PATH=SRV*c:\\symbols*\\\\symsrv\\symbols;SRV*c:\\symbols*http://msdl.microsoft.com/download/symbols");
#pragma warning(suppress : 4996)
	const char* symCachePath = getenv("_NT_SYMCACHE_PATH");
	if (!symCachePath)
		(void)_putenv("_NT_SYMCACHE_PATH=c:\\symcache");

	std::string wpaPath = GetWPTDir() + "wpa.exe";

	const std::string args = std::string("wpa.exe \"") + traceFilename.c_str() + "\"";

	// Wacky CreateProcess rules say args has to be writable!
	std::vector<char> argsCopy(args.size() + 1);
	strcpy_s(&argsCopy[0], argsCopy.size(), args.c_str());
	STARTUPINFO startupInfo = {};
	PROCESS_INFORMATION processInfo = {};
	BOOL result = CreateProcess(wpaPath.c_str(), &argsCopy[0], NULL, NULL, FALSE, 0, NULL, NULL, &startupInfo, &processInfo);
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
	std::string args = "xperf.exe -setprofint " + profInt + " cached";
	child.Run(bShowCommands_, args);
}


void CXperfUIDlg::OnBnClickedLoginput()
{
	bRecordInput_ = !bRecordInput_;
	// Should have a way to set it to kKeyLoggerAnonymized. Oh well. kKeyLoggerFull makes for better demos!
	SetKeyloggingState(bRecordInput_ ? kKeyLoggerFull : kKeyLoggerOff);
	ETWMarkPrintf("%s input logger", bRecordInput_ ? "Enabling" : "Disabling");
}
