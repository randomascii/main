
// XperfUIDlg.h : header file
//

#pragma once

#include <string>
#include <vector>
#include "KeyLoggerThread.h"

// CXperfUIDlg dialog
class CXperfUIDlg : public CDialogEx
{
// Construction
public:
	CXperfUIDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_XPERFUI_DIALOG };

	void vprintf(const char* pFormat, va_list marker);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

// Implementation
protected:
	HICON m_hIcon;

	bool bIsTracing_ = false;

	CButton btStartTracing_;
	CButton btSaveTraceBuffers_;
	CButton btStopTracing_;

	bool bCompress_ = true;
	bool bSampledStacks_ = true;
	bool bCswitchStacks_ = true;
	bool bShowCommands_ = false;
	bool bFastSampling_ = false;
	CButton btCompress_;
	CButton btSampledStacks_;
	CButton btCswitchStacks_;
	CButton btShowCommands_;
	CButton btFastSampling_;

	KeyLoggerState InputTracing_ = kKeyLoggerAnonymized;
	CComboBox btInputTracing_;

	std::vector<std::string> traces_;
	CListBox btTraces_;

	std::string traceDir_;
	std::string tempTraceDir_;

	std::string output_;

	std::string GetWPTDir();
	std::string GetXperfPath();
	std::string GetTraceDir();
	std::string GetExeDir();
	// Note that GetResultFile() gives a time-based name, so don't expect
	// the same result across multiple calls!
	std::string GetResultFile();
	std::string GetTempTraceDir();
	std::string GetKernelFile();
	std::string GetUserFile();

	void SetSymbolPath();
	std::string GetDirectory(const char* env, const std::string& default);
	void CXperfUIDlg::UpdateTraceList();
	void RegisterProviders();
	void DisablePagingExecutive();

	// Update the enabled/disabled states of buttons.
	void UpdateEnabling();
	void LaunchTraceViewer(const std::string traceFilename);

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
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
};
