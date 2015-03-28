#pragma once

#include <string>
#include <vector>
#include <memory>
#include "KeyLoggerThread.h"
#include "DirectoryMonitor.h"

enum TracingMode
{
	kTracingToFile,
	kTracingToMemory,
	kHeapTracingToFile
};

class CXperfUIDlg : public CDialogEx
{
public:
	CXperfUIDlg(CWnd* pParent = NULL);	// standard constructor
	~CXperfUIDlg();

// Dialog Data
	enum { IDD = IDD_XPERFUI_DIALOG };

	void vprintf(const wchar_t* pFormat, va_list marker);

private:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

	HICON m_hIcon;

	bool bIsTracing_ = false;

	CButton btStartTracing_;
	CButton btSaveTraceBuffers_;
	CButton btStopTracing_;

	bool bCompress_ = true;
	bool bCswitchStacks_ = true;
	bool bSampledStacks_ = true;
	bool bFastSampling_ = false;
	bool bDirectXTracing_ = false;
	bool bShowCommands_ = false;
	CButton btCompress_;
	CButton btCswitchStacks_;
	CButton btSampledStacks_;
	CButton btFastSampling_;
	CButton btDirectXTracing_;
	CButton btShowCommands_;

	CEdit btTraceNameEdit_;
	std::wstring preRenameTraceName_;
	// Typical trace names look like this:
	// 2015-03-21_08-52-11_Bruce
	// The first 19 characters are the date and time.
	// The remainder are eligible for editin.
	const size_t kPrefixLength = 19;
	void StartRenameTrace();

	bool useChromeProviders_ = false;

	KeyLoggerState InputTracing_ = kKeyLoggerAnonymized;
	CComboBox btInputTracing_;
	CStatic btInputTracingLabel_;

	TracingMode tracingMode_ = kTracingToMemory;
	CComboBox btTracingMode_;
	// Hardcoded to chrome.exe for now.
	std::wstring heapTracingExe_ = L"chrome.exe";
	void SetHeapTracing(bool forceOff);

	std::vector<std::wstring> traces_;
	CListBox btTraces_;

	DirectoryMonitor monitorThread_;

	// This contains the notes for the selected trace, as loaded from disk.
	std::wstring traceNotes_;
	std::wstring traceNoteFilename_;
	CEdit btTraceNotes_;

	// Note that the DirectoryMonitorThread has a pointer to the contents of
	// this string object, so don't change it without adding synchronization.
	std::wstring traceDir_;
	std::wstring tempTraceDir_;

	std::wstring output_;
	CEdit btOutput_;

	HACCEL hAccelTable_ = NULL;
	HACCEL hRenameAccelTable_ = NULL;

	void StopTracing(bool bSaveTrace);

	std::wstring GetWPTDir() const;
	std::wstring GetXperfPath() const { return GetWPTDir() + L"xperf.exe"; }
	std::wstring GetTraceDir() const { return traceDir_; }
	std::wstring GetExeDir() const;
	// Note that GetResultFile() gives a time-based name, so don't expect
	// the same result across multiple calls!
	std::wstring GetResultFile() const;
	std::wstring GetTempTraceDir() const;
	std::wstring GetKernelFile() const;
	std::wstring GetUserFile() const;
	std::wstring GetHeapFile() const;

	// Get session name for kernel logger
	const std::wstring logger_ = L"\"NT Kernel Logger\"";
	//const std::wstring logger_ = L"\"Circular Kernel Context Logger\"";
	std::wstring GetKernelLogger() const { return logger_; }

	int initialWidth_ = 0;
	int initialHeight_ = 0;
	int lastWidth_ = 0;
	int lastHeight_ = 0;

	void SetSymbolPath();
	// Call this to retrieve a directory from an environment variable, or use
	// a default, and make sure it exists.
	std::wstring GetDirectory(const wchar_t* env, const std::wstring& default);
	void CXperfUIDlg::UpdateTraceList();
	void RegisterProviders();
	void DisablePagingExecutive();

	CToolTipCtrl toolTip_;

	void CompressTrace(const std::wstring& tracePath);
	// Update the enabled/disabled states of buttons.
	void UpdateEnabling();
	void UpdateNotesState();
	void StripChromeSymbols(const std::wstring& traceFilename);
	void LaunchTraceViewer(const std::wstring traceFilename, const std::wstring viewer = L"wpa.exe");
	void SaveNotesIfNeeded();
	void ShutdownTasks();
	bool bShutdownCompleted_ = false;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
	afx_msg void OnBnClickedStarttracing();
	afx_msg void OnBnClickedStoptracing();
	afx_msg void OnBnClickedCompresstrace();
	afx_msg void OnBnClickedCpusamplingcallstacks();
	afx_msg void OnBnClickedContextswitchcallstacks();
	afx_msg void OnBnClickedShowcommands();
	afx_msg void OnBnClickedFastsampling();
	afx_msg void OnCbnSelchangeInputtracing();
	afx_msg LRESULT UpdateTraceListHandler(WPARAM wParam, LPARAM lParam);
	afx_msg void OnLbnDblclkTracelist();
	afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
	afx_msg void OnSize(UINT, int, int);
	afx_msg void OnLbnSelchangeTracelist();
	afx_msg void OnBnClickedAbout();
	afx_msg void OnBnClickedSavetracebuffers();
	afx_msg LRESULT OnHotKey(WPARAM wParam, LPARAM lParam);
	afx_msg BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnClose(); 
	afx_msg void OnCancel();
	afx_msg void OnOK();
	afx_msg void OnCbnSelchangeTracingmode();
	afx_msg void OnBnClickedSettings();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnRenameKey();
	afx_msg void FinishTraceRename();
	afx_msg void OnEscKey();
	afx_msg void OnOpenTraceWPA();
	afx_msg void OnOpenTraceGPUView();
public:
	afx_msg void OnBnClickedDirectxtracing();
};
